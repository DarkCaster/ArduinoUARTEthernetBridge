#include "TCPClient.h"
#include "ImmutableStorage.h"
#include "IPAddress.h"

#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

class ShutdownMessage: public IShutdownMessage { public: ShutdownMessage(int _ec):IShutdownMessage(_ec){} };

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
        return -1;
    }

    if(!target.Get().isValid)
        return -1;

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

    return fd;
}

void TCPClient::Worker()
{
    //connect on start
}

void TCPClient::OnMessage(const void* const, const IMessage& message)
{
    std::lock_guard<std::mutex> opGuard(opLock);
    if(message.msgType==MSG_NEW_CLIENT)
    {
        auto ncMessage=static_cast<const INewClientMessage&>(message);
        if(ncMessage.pathID!=pathID)
            return;
        //if current connection already claimed and not failed - dispose new client, and return
        //create remote connection if not present or failed
        //send MSG_PATH_ESTABLISHED message
    }
    else if(message.msgType==MSG_PATH_COLLAPSED)
    {
        auto pdMessage=static_cast<const IPathDisposedMessage&>(message);
        if(pdMessage.pathID!=pathID)
            return;
        //if provided "remote" part is not present locally or failed, dispose it
        //create remote connection if not present or failed
    }
}


