#include "UDPTransport.h"
#include "ImmutableStorage.h"
#include "UDPConnection.h"
#include "CRC8.h"
#include "Command.h"

#include <cstring>
#include <fcntl.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <netdb.h>


class ShutdownMessage: public IShutdownMessage { public: ShutdownMessage(int _ec):IShutdownMessage(_ec){} };
class IncomingPackageMessage: public IIncomingPackageMessage { public: IncomingPackageMessage(const uint8_t* const _package):IIncomingPackageMessage(_package){} };

UDPTransport::UDPTransport(std::shared_ptr<ILogger>& _logger, IMessageSender& _sender, const IConfig& _config):
    logger(_logger),
    sender(_sender),
    config(_config),
    rxBuff(std::make_unique<uint8_t[]>(static_cast<size_t>(config.GetNetPackageSz())))
{
    shutdownPending.store(false);
    remoteConn=nullptr;
    udpPort=0;
    droppedRxSeqCnt=0;
}

static IPAddress Lookup(const std::string &target)
{
    addrinfo hints={};
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;

    addrinfo *addrResults=nullptr;
    if(getaddrinfo (target.c_str(), NULL, &hints, &addrResults)!=0)
        return IPAddress();

    IPAddress result(addrResults->ai_addr);
    freeaddrinfo(addrResults);
    return result;
}

static void TuneSocketBaseParams(std::shared_ptr<ILogger> &logger, int fd, const IConfig& config)
{
    //set buffer size
    auto sbsz=config.GetTCPBuffSz();
    if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sbsz, sizeof(sbsz)))
        logger->Warning()<<"Failed to set SO_SNDBUF option to socket: "<<strerror(errno);
    auto rbsz=config.GetTCPBuffSz();
    if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rbsz, sizeof(rbsz)))
        logger->Warning()<<"Failed to set SO_RCVBUF option to socket: "<<strerror(errno);
    int tos=IPTOS_RELIABILITY;
    if (setsockopt(fd, IPPROTO_IP, IP_TOS,&tos, sizeof(tos)) < 0)
        logger->Warning()<<"Failed to set IP_TOS option to socket: "<<strerror(errno);
}

static void SetSocketCustomTimeouts(std::shared_ptr<ILogger> &logger, int fd, const timeval &tv)
{
    timeval rtv=tv;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &rtv, sizeof(rtv))!=0)
        logger->Warning()<<"Failed to set SO_RCVTIMEO option to socket: "<<strerror(errno);
    timeval stv=tv;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &stv, sizeof(stv))!=0)
        logger->Warning()<<"Failed to set SO_SNDTIMEO option to socket: "<<strerror(errno);
}

void UDPTransport::HandleError(const std::string &message)
{
    logger->Error()<<message<<std::endl;
    sender.SendMessage(this,ShutdownMessage(1));
}

void UDPTransport::HandleError(int ec, const std::string &message)
{
    logger->Error()<<message<<strerror(ec)<<std::endl;
    sender.SendMessage(this,ShutdownMessage(ec));
}

std::shared_ptr<UDPConnection> UDPTransport::GetConnection()
{
    if(!config.GetUDPEnabled())
    {
        logger->Error()<<"Cannot create connection, because use of UDP transport is disabled";
        return nullptr;
    }

    //valid port number will when TCP connection is established
    if(udpPort<1)
        return nullptr;

    std::lock_guard<std::mutex> opGuard(remoteConnLock);
    if(remoteConn!=nullptr && remoteConn->GetStatus())
        return remoteConn;

    if(remoteConn!=nullptr)
        remoteConn->Dispose();

    ImmutableStorage<IPAddress> target(IPAddress(config.GetRemoteAddr()));
    if(!target.Get().isValid)
    {
        target.Set(Lookup(config.GetRemoteAddr()));
        if(!target.Get().isValid)
        {
            remoteConn=nullptr;
            return nullptr;
        }
    }

    //create socket
    auto fd=socket(target.Get().isV6?AF_INET6:AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(fd<0)
    {
        HandleError(errno,"Failed to create new socket: ");
        {
            remoteConn=nullptr;
            return nullptr;
        }
    }

    TuneSocketBaseParams(logger,fd,config);
    SetSocketCustomTimeouts(logger,fd,config.GetServiceIntervalTV());

    int cr=-1;
    if(target.Get().isV6)
    {
        sockaddr_in6 v6sa={};
        target.Get().ToSA(&v6sa);
        v6sa.sin6_port=htons(udpPort);
        cr=connect(fd,reinterpret_cast<sockaddr*>(&v6sa), sizeof(v6sa));
    }
    else
    {
        sockaddr_in v4sa={};
        target.Get().ToSA(&v4sa);
        v4sa.sin_port=htons(udpPort);
        cr=connect(fd,reinterpret_cast<sockaddr*>(&v4sa), sizeof(v4sa));
    }

    if(cr<0)
    {
        auto error=errno;
        if(close(fd)!=0)
        {
            HandleError(error,"Failed to perform proper socket close after connection failure: ");
            {
                remoteConn=nullptr;
                return nullptr;
            }
        }
        remoteConn=nullptr;
        return nullptr;
    }

    remoteConn=std::make_shared<UDPConnection>(fd,udpPort);
    logger->Info()<<"Remote connection created";
    return remoteConn;
}

void UDPTransport::Worker()
{
    if(!config.GetUDPEnabled())
    {
        logger->Info()<<"Use of UDP transport is disabled";
        return;
    }

    logger->Info()<<"Starting transport";
    while(!shutdownPending.load())
    {
        //get connection
        auto conn=GetConnection();
        if(conn==nullptr)
        {
            //wait for a while and try again
            std::this_thread::sleep_for(std::chrono::milliseconds(config.GetServiceIntervalMS()));
            continue;
        }

        const size_t pkgSz=static_cast<size_t>(config.GetNetPackageSz());
        iovec pkgVec={rxBuff.get(),pkgSz};
        msghdr pkgHdr={};
        pkgHdr.msg_iov=&pkgVec;
        pkgHdr.msg_iovlen=1;

        //read package
        auto dr=recvmsg(conn->fd,&pkgHdr,0);
        if(dr<=0)
        {
            auto error=errno;
            if(error==EINTR || error==EAGAIN || error==EWOULDBLOCK)
                continue;
            //socket was closed or errored, close connection from our side and stop reading
            if(!shutdownPending.load())
                logger->Warning()<<"recvmsg failed: "<<strerror(error);
            conn->Dispose();
            continue;
        }
        if(static_cast<size_t>(dr)<pkgSz)
        {
            logger->Warning()<<"Dropping package with less bytes than expected: "<<dr<<" bytes, instead of: "<<pkgSz<<" bytes";
            continue;
        }
        if((pkgHdr.msg_flags&MSG_TRUNC)>0)
        {
            logger->Warning()<<"Dropping too big package";
            continue;
        }

        //TODO: check remote port if needed

        //verify CRC, disconnect on failure and drop data
        if(*(rxBuff.get()+config.GetNetPackageMetaSz())!=CRC8(rxBuff.get(),static_cast<size_t>(config.GetNetPackageMetaSz())))
        {
            logger->Warning()<<"Dropping package with invalid control block checksum";
            continue;
        }

        //check for sequence number
        if(!conn->RXSeqCheckIncrement(static_cast<uint16_t>(*rxBuff.get()|*(rxBuff.get()+1)<<8)))
        {
            if(droppedRxSeqCnt<1)
                logger->Warning()<<"Dropping incoming packages due to invalid sequence number!";
            droppedRxSeqCnt++;
            continue;
        }

        if(droppedRxSeqCnt>0)
        {
            logger->Warning()<<"Dropped "<<droppedRxSeqCnt<<" packages with invalid sequence number";
            droppedRxSeqCnt=0;
        }

        //TODO: extra check for payloads size at metadata block, disconnect if too big

        //logger->Info()<<"New UDP package received";
        //signal new package received
        sender.SendMessage(this, IncomingPackageMessage(rxBuff.get()));
    }

    std::lock_guard<std::mutex> opGuard(remoteConnLock);
    if(remoteConn!=nullptr)
        remoteConn->Dispose();
    logger->Info()<<"Transport shutdown";
}

bool UDPTransport::ReadyForMessage(const MsgType msgType)
{
    return msgType==MSG_SEND_PACKAGE || msgType==MSG_CONNECTED;
}

void UDPTransport::OnMessage(const void* const, const IMessage& message)
{
    if(message.msgType==MSG_SEND_PACKAGE)
        OnSendPackage(static_cast<const ISendPackageMessage&>(message));
    if(message.msgType==MSG_CONNECTED)
        OnConnected(static_cast<const IConnectedMessage&>(message));
}

void UDPTransport::OnSendPackage(const ISendPackageMessage& message)
{
    if(message.useTCP)
        return;

    if(!config.GetUDPEnabled())
    {
        logger->Error()<<"Cannot send package via UDP transport, because it is disabled!";
        return;
    }

    //try to get connection
    auto txBuff=message.package;
    auto conn=GetConnection();
    if(conn==nullptr)
        return;

    //write UDP sequence
    WriteU16Value(conn->TXSeqIncrement(),txBuff);

    //generate checksum
    *(txBuff+config.GetNetPackageMetaSz())=CRC8(txBuff,static_cast<size_t>(config.GetNetPackageMetaSz()));

    //send package
    auto dw=send(conn->fd,txBuff,static_cast<size_t>(config.GetNetPackageSz()),0);
    if(dw<=0)
    {
        auto error=errno;
        if(error==EINTR)
            return;
        if(!shutdownPending.load())
            logger->Warning()<<"send failed: "<<strerror(error);
        conn->Dispose();
        return;
    }

    if(dw<config.GetNetPackageMetaSz())
        logger->Warning()<<"Partial send detected: "<<dw<<" bytes; instead of: "<<config.GetNetPackageMetaSz()<<" bytes";
}

void UDPTransport::OnConnected(const IConnectedMessage& message)
{
    //destroy current connection, it will be recreated on next send/receive operation
    std::lock_guard<std::mutex> opGuard(remoteConnLock);
    udpPort=message.udpPort;
    if(remoteConn!=nullptr)
        remoteConn->Dispose();
    if(config.GetUDPEnabled())
        logger->Info()<<"Connection reset pending";
}

void UDPTransport::OnShutdown()
{
    shutdownPending.store(true);
    std::lock_guard<std::mutex> opGuard(remoteConnLock);
    if(remoteConn!=nullptr)
        remoteConn->Dispose();
}
