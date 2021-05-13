#include "TCPClient.h"
#include "ImmutableStorage.h"
#include "IPAddress.h"

#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

class ShutdownMessage: public IShutdownMessage { public: ShutdownMessage(int _ec):IShutdownMessage(_ec){} };
class PathEstablishedMessage: public IPathEstablishedMessage { public: PathEstablishedMessage(const std::shared_ptr<Connection> &_local, const std::shared_ptr<Connection> &_remote, const int _pathID): IPathEstablishedMessage(_local,_remote,_pathID){}; };

TCPClient::TCPClient(std::shared_ptr<ILogger> &_logger, IMessageSender &_sender, const bool _connectOnStart, const IConfig &_config, const RemoteConfig &_remoteConfig, const int _pathID):
    logger(_logger),
    sender(_sender),
    connectOnStart(_connectOnStart),
    config(_config),
    remoteConfig(_remoteConfig),
    pathID(_pathID)
{
    shutdownPending.store(false);
    remote=nullptr;
}

void TCPClient::HandleError(const std::string &message)
{
    logger->Error()<<message<<std::endl;
    sender.SendMessage(this,ShutdownMessage(1));
}

void TCPClient::HandleError(int ec, const std::string &message)
{
    logger->Error()<<message<<strerror(ec)<<std::endl;
    sender.SendMessage(this,ShutdownMessage(ec));
}

void TCPClient::OnShutdown()
{
    shutdownPending.store(true);
    std::lock_guard<std::mutex> opGuard(opLock);
    //dispose remote if present
}

bool TCPClient::ReadyForMessage(const MsgType msgType)
{
    return msgType==MSG_NEW_CLIENT || msgType==MSG_PATH_COLLAPSED;
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

int TCPClient::_Connect()
{
    ImmutableStorage<IPAddress> target(IPAddress(remoteConfig.serverAddr));
    if(!target.Get().isValid)
    {
        //TODO: perform host lookup
        HandleError("TODO: Host lookup is not implemented!");
        return -1;
    }

    //create socket
    auto fd=socket(target.Get().isV6?AF_INET6:AF_INET,SOCK_STREAM,0);
    if(fd<0)
    {
        HandleError(errno,"Failed to create new socket: ");
        return -1;
    }

    TuneSocketBaseParams(logger,fd,config);
    SetSocketCustomTimeouts(logger,fd,config.GetServiceIntervalTV());

    int cr=-1;
    if(target.Get().isV6)
    {
        sockaddr_in6 v6sa={};
        target.Get().ToSA(&v6sa);
        v6sa.sin6_port=htons(remoteConfig.serverPort);
        cr=connect(fd,reinterpret_cast<sockaddr*>(&v6sa), sizeof(v6sa));
    }
    else
    {
        sockaddr_in v4sa={};
        target.Get().ToSA(&v4sa);
        v4sa.sin_port=htons(remoteConfig.serverPort);
        cr=connect(fd,reinterpret_cast<sockaddr*>(&v4sa), sizeof(v4sa));
    }

    if(cr<0)
    {
        auto error=errno;
        if(close(fd)!=0)
        {
            HandleError(error,"Failed to perform proper socket close after connection failure: ");
            return -1;
        }
        if(error!=EINPROGRESS)
            logger->Warning()<<"Failed to connect "<<target.Get()<<" with error: "<<strerror(error);
        else
            logger->Warning()<<"Connection attempt to "<<target.Get()<<" timed out";
        return -1;
    }

    //send configuration
    uint8_t cfg[5];
    cfg[0]=remoteConfig.speed&0x0000FF;
    cfg[1]=(remoteConfig.speed&0x00FF00)>>8;
    cfg[2]=(remoteConfig.speed&0xFF0000)>>16;
    cfg[3]=remoteConfig.mode;
    cfg[4]=remoteConfig.flags;

    if(write(fd,cfg,5)!=5)
    {
        auto error=errno;
        if(close(fd)!=0)
            HandleError(error,"Failed to perform proper socket close after configuration failure: ");
        return -1;
    }

    return fd;
}

void TCPClient::Worker()
{
    if(!connectOnStart)
    {
        logger->Info()<<"Background remote-connection maintenance is disabled for uart port "<<pathID;
        return;
    }

    logger->Info()<<"Background remote-connection maintenance enabled for uart port "<<pathID;
    //maintain connection
    while(!shutdownPending)
    {
        {
            std::lock_guard<std::recursive_mutex> opGuard(opLock);
            if(!remoteActive)
            {
                auto fd=_Connect();
                if(fd>=0)
                {
                    remote=std::make_shared<TCPConnection>(fd);
                    remoteActive=true;
                    logger->Info()<<"Established background connection to remote for uart port "<<pathID;
                }
            }
            else
            {
                if(remote!=nullptr)
                {
                    if(remote->GetStatus()!=0)
                    {
                        remote->Dispose();
                        remote=nullptr;
                        remoteActive=false;
                    }
                }
                else
                    remoteActive=false;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(config.GetServiceIntervalMS()));
    }
}

void TCPClient::OnMessage(const void* const, const IMessage& message)
{
    std::lock_guard<std::recursive_mutex> opGuard(opLock);
    if(message.msgType==MSG_NEW_CLIENT)
    {
        auto ncMessage=static_cast<const INewClientMessage&>(message);
        if(ncMessage.pathID!=pathID)
            return;
        //establish remote connection, or reuse current connection
        if(!remoteActive)
        {
            if(remote!=nullptr)
                remote->Dispose();
            logger->Info()<<"Establishing new remote-connection for uart port "<<pathID;
            auto fd=_Connect();
            if(fd>=0)
            {
                remote=std::make_shared<TCPConnection>(fd);
                remoteActive=true;
                logger->Info()<<"Established connection to remote for uart port "<<pathID;
            }
            else
                remote=nullptr;
        }
        //send MSG_PATH_ESTABLISHED message
        if(remoteActive && remote!=nullptr)
            sender.SendMessage(this, PathEstablishedMessage(ncMessage.client,remote,pathID));
        else
        {
            logger->Warning()<<"Failed to connect to remote, disposing local connection for uart port "<<pathID;
            ncMessage.client->Dispose();
        }
    }
    else if(message.msgType==MSG_PATH_COLLAPSED)
    {
        auto pdMessage=static_cast<const IPathCollapsedMessage&>(message);
        if(pdMessage.pathID!=pathID)
            return;
        //dispose remote connection if it is not tracked at our side
        if(pdMessage.remote!=nullptr && pdMessage.remote!=remote)
            pdMessage.remote->Dispose();
        //dispose local connection as precaution, after remote
        if(pdMessage.local!=nullptr)
            pdMessage.local->Dispose();
        //stop if provided remote connection is not matched with ours
        if(!remoteActive||remote==nullptr||pdMessage.remote==nullptr||pdMessage.remote!=remote)
            return;
        //disconnect remote if needed
        if(!connectOnStart||remote->GetStatus()!=0)
        {
            logger->Info()<<"Disposing remote connection for uart port "<<pathID;
            remote->Dispose();
            remote=nullptr;
            remoteActive=false;
        }
    }
}
