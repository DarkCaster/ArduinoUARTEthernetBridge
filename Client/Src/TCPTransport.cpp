#include "TCPTransport.h"
#include "ImmutableStorage.h"
#include "TCPConnection.h"
#include "CRC8.h"

#include <cstring>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <netdb.h>

class ShutdownMessage: public IShutdownMessage { public: ShutdownMessage(int _ec):IShutdownMessage(_ec){} };
class ConnectedMessage: public IConnectedMessage { public: ConnectedMessage():IConnectedMessage(){} };
class IncomingPackageMessage: public IIncomingPackageMessage { public: IncomingPackageMessage(const uint8_t* const _package):IIncomingPackageMessage(_package){} };

TCPTransport::TCPTransport(std::shared_ptr<ILogger>& _logger, IMessageSender& _sender, const IConfig& _config, uint8_t* _rxBuff, uint8_t* _txBuff):
    logger(_logger),
    sender(_sender),
    config(_config),
    rxBuff(_rxBuff),
    txBuff(_txBuff)
{
    shutdownPending.store(false);
    remoteConn=nullptr;
}

static IPAddress Lookup(std::shared_ptr<ILogger> &logger, const std::string &target)
{
    addrinfo hints={};
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;

    addrinfo *addrResults=nullptr;
    if(getaddrinfo (target.c_str(), NULL, &hints, &addrResults)!=0)
    {
        logger->Warning()<<"Lookup of "<<target<<" is failed with: "<<strerror(errno);
        return IPAddress();
    }

    IPAddress result(addrResults->ai_addr);
    freeaddrinfo(addrResults);
    return result;
}


static void TuneSocketBaseParams(std::shared_ptr<ILogger> &logger, int fd, const IConfig& config)
{
    //set linger
    linger cLinger={0,0};
    cLinger.l_onoff=config.GetLingerSec()>=0;
    cLinger.l_linger=cLinger.l_onoff?config.GetLingerSec():0;
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &cLinger, sizeof(linger))!=0)
        logger->Warning()<<"Failed to set SO_LINGER option to socket: "<<strerror(errno);
    //set buffer size
    auto bsz=config.GetTCPBuffSz();
    if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bsz, sizeof(bsz)))
        logger->Warning()<<"Failed to set SO_SNDBUF option to socket: "<<strerror(errno);
    bsz=config.GetTCPBuffSz();
    if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bsz, sizeof(bsz)))
        logger->Warning()<<"Failed to set SO_RCVBUF option to socket: "<<strerror(errno);
    int tcpNoDelay=1;
    if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &tcpNoDelay, sizeof(tcpNoDelay)))
        logger->Warning()<<"Failed to set TCP_NODELAY option to socket: "<<strerror(errno);
    int tcpQuickAck=1;
    if(setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, &tcpQuickAck, sizeof(tcpQuickAck)))
        logger->Warning()<<"Failed to set TCP_QUICKACK option to socket: "<<strerror(errno);
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

void TCPTransport::HandleError(const std::string &message)
{
    logger->Error()<<message<<std::endl;
    sender.SendMessage(this,ShutdownMessage(1));
}

void TCPTransport::HandleError(int ec, const std::string &message)
{
    logger->Error()<<message<<strerror(ec)<<std::endl;
    sender.SendMessage(this,ShutdownMessage(ec));
}

std::shared_ptr<Connection> TCPTransport::GetConnection()
{
    std::lock_guard<std::mutex> opGuard(remoteConnLock);
    if(remoteConn!=nullptr && remoteConn->GetStatus()==0)
        return remoteConn;

    if(remoteConn!=nullptr)
        remoteConn->Dispose();

    ImmutableStorage<IPAddress> target(IPAddress(config.GetRemoteAddr()));
    if(!target.Get().isValid)
    {
        target.Set(Lookup(logger,config.GetRemoteAddr()));
        if(!target.Get().isValid)
        {
            remoteConn=nullptr;
            return nullptr;
        }
    }

    //create socket
    auto fd=socket(target.Get().isV6?AF_INET6:AF_INET,SOCK_STREAM,0);
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
        v6sa.sin6_port=htons(config.GetTCPPort());
        cr=connect(fd,reinterpret_cast<sockaddr*>(&v6sa), sizeof(v6sa));
    }
    else
    {
        sockaddr_in v4sa={};
        target.Get().ToSA(&v4sa);
        v4sa.sin_port=htons(config.GetTCPPort());
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
        /*if(error!=EINPROGRESS)
            logger->Warning()<<"Failed to connect "<<target.Get()<<" with error: "<<strerror(error);
        else
            logger->Warning()<<"Connection attempt to "<<target.Get()<<" timed out";*/
        remoteConn=nullptr;
        return nullptr;
    }

    remoteConn=std::make_shared<TCPConnection>(fd);
    logger->Info()<<"Remote connection established";
    sender.SendMessage(this, ConnectedMessage());
    return remoteConn;
}

void TCPTransport::Worker()
{
    logger->Info()<<"Trying to connect: "<<config.GetRemoteAddr()<<":"<<config.GetTCPPort();
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

        const size_t pkgSz=static_cast<size_t>(config.GetPackageSz());
        auto szLeft=pkgSz;
        while(!shutdownPending.load())
        {
            //read package
            auto dr=read(conn->fd,rxBuff+pkgSz-szLeft,szLeft);
            if(dr<=0)
            {
                auto error=errno;
                if(error==EINTR || error==EWOULDBLOCK)
                    continue;
                conn->SetStatus(error);
                //socket was closed or errored, close connection from our side and stop reading
                logger->Warning()<<"TCP read failed: "<<strerror(error);
                conn->Dispose();
                break;
            }
            szLeft-=static_cast<size_t>(dr);
            if(szLeft>0)
                continue;
            //verify CRC, disconnect on failure and drop data
            if(*(rxBuff+config.GetPackageMetaSz())!=CRC8(rxBuff,static_cast<size_t>(config.GetPackageMetaSz())))
            {
                logger->Error()<<"Package CRC mismatch! This should not happen normally, check your configuration!";
                conn->Dispose();
                break;
            }
            //logger->Info()<<"New TCP package received";
            //signal new package received
            sender.SendMessage(this, IncomingPackageMessage(rxBuff));
            szLeft=pkgSz;
        }
    }
    logger->Info()<<"TCP transport-worker shuting down";
}

void TCPTransport::ProcessTX()
{
    //try to get connection
    auto conn=GetConnection();
    if(conn==nullptr)
        return;
    //clear PKG_HEADER (not needed) and calculate CRC
    *(txBuff)=*(txBuff+1)=0;
    *(txBuff+config.GetPackageMetaSz())=CRC8(txBuff,static_cast<size_t>(config.GetPackageMetaSz()));
    //send package
    const size_t pkgSz=static_cast<size_t>(config.GetPackageSz());
    auto dataLeft=pkgSz;
    while(dataLeft>0)
    {
        auto dw=write(conn->fd,txBuff+pkgSz-dataLeft,dataLeft);
        if(dw<=0)
        {
            auto error=errno;
            if(error==EINTR || error==EWOULDBLOCK)
                continue;
            logger->Warning()<<"TCP write failed: "<<strerror(error);
            conn->SetStatus(error);
            conn->Dispose();
            return;
        }
        dataLeft-=static_cast<size_t>(dw);
    }
}

void TCPTransport::OnShutdown()
{
    shutdownPending.store(true);
}
