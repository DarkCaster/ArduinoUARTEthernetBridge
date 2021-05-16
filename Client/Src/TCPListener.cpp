#include "TCPListener.h"
#include "TCPConnection.h"

#include <cstdint>
#include <cstring>
#include <cerrno>
#include <string>

#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>

class ShutdownMessage: public IShutdownMessage { public: ShutdownMessage(int _ec):IShutdownMessage(_ec){} };
class NewClientMessage: public INewClientMessage { public: NewClientMessage(std::shared_ptr<Connection> _client, int _pathID):INewClientMessage(_client,_pathID){} };

TCPListener::TCPListener(std::shared_ptr<ILogger> &_logger, IMessageSender &_sender, const IConfig &_config, const RemoteConfig &_remoteConfig, int _pathID):
    logger(_logger),
    sender(_sender),
    config(_config),
    remoteConfig(_remoteConfig),
    pathID(_pathID)
{
    shutdownPending.store(false);
}

void TCPListener::HandleError(const std::string &message)
{
    logger->Error()<<message<<std::endl;
    sender.SendMessage(this,ShutdownMessage(1));
}

void TCPListener::HandleError(int ec, const std::string &message)
{
    logger->Error()<<message<<strerror(ec)<<std::endl;
    sender.SendMessage(this,ShutdownMessage(ec));
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
    if(config.GetTCPNoDelay())
    {
        int tcpNoDelay=1;
        if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &tcpNoDelay, sizeof(tcpNoDelay)))
            logger->Warning()<<"Failed to set TCP_NODELAY option to socket: "<<strerror(errno);
        int tcpQuickAck=1;
        if(setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, &tcpQuickAck, sizeof(tcpQuickAck)))
            logger->Warning()<<"Failed to set TCP_QUICKACK option to socket: "<<strerror(errno);
    }
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

void TCPListener::Worker()
{ 
    if(!remoteConfig.listener.address.isValid||remoteConfig.listener.port==0)
    {
        HandleError("Listen IP address or port is invalid");
        return;
    }

    //create listen socket
    auto lSockFd=socket(remoteConfig.listener.address.isV6?AF_INET6:AF_INET,SOCK_STREAM,0);
    if(lSockFd==-1)
    {
        HandleError(errno,"Failed to create listen socket: ");
        return;
    }

    //tune some some options
    int sockReuseAddrEnabled=1;
    if (setsockopt(lSockFd, SOL_SOCKET, SO_REUSEADDR, &sockReuseAddrEnabled, sizeof(int))!=0)
    {
        HandleError(errno,"Failed to set SO_REUSEADDR option: ");
        return;
    }
#ifdef SO_REUSEPORT
    int sockReusePortEnabled=1;
    if (setsockopt(lSockFd, SOL_SOCKET, SO_REUSEPORT, &sockReusePortEnabled, sizeof(int))!=0)
    {
        HandleError(errno,"Failed to set SO_REUSEPORT option: ");
        return;
    }
#endif

    linger lLinger={1,0};
    if (setsockopt(lSockFd, SOL_SOCKET, SO_LINGER, &lLinger, sizeof(linger))!=0)
    {
        HandleError(errno,"Failed to set SO_LINGER option: ");
        return;
    }

    sockaddr_in ipv4Addr = {};
    sockaddr_in6 ipv6Addr = {};
    sockaddr *target;
    socklen_t len;
    if(remoteConfig.listener.address.isV6)
    {
        ipv6Addr.sin6_family=AF_INET6;
        ipv6Addr.sin6_port=htons(static_cast<uint16_t>(remoteConfig.listener.port));
        remoteConfig.listener.address.ToSA(&ipv6Addr);
        target=reinterpret_cast<sockaddr*>(&ipv6Addr);
        len=sizeof(sockaddr_in6);
    }
    else
    {
        ipv4Addr.sin_family=AF_INET;
        ipv4Addr.sin_port=htons(static_cast<uint16_t>(remoteConfig.listener.port));
        remoteConfig.listener.address.ToSA(&ipv4Addr);
        target=reinterpret_cast<sockaddr*>(&ipv4Addr);
        len=sizeof(sockaddr_in);
    }

    if (bind(lSockFd,target,len)!=0)
    {
        HandleError(errno,"Failed to bind listen socket: ");
        return;
    }

    if (listen(lSockFd,1)!=0)
    {
        HandleError(errno,"Failed to setup listen socket: ");
        return;
    }

    logger->Info()<<"Listening for incoming connection"<<std::endl;

    pollfd lst;
    lst.fd=lSockFd;
    lst.events=POLLIN;

    while (!shutdownPending.load())
    {
        lst.revents=0;
        auto lrv=poll(&lst,1,config.GetServiceIntervalMS());
        if(lrv==0) //no incoming connection detected
            continue;

        if(lrv<0)
        {
            auto error=errno;
            if(error==EINTR)//interrupted by signal
                break;
            HandleError(error,"Error awaiting incoming connection: ");
            return;
        }

        auto cSockFd=accept(lSockFd,nullptr,nullptr);
        if(cSockFd<1)
        {
            logger->Warning()<<"Failed to accept connection: "<<strerror(errno)<<std::endl;
            continue;
        }

        TuneSocketBaseParams(logger,cSockFd,config);
        SetSocketCustomTimeouts(logger,cSockFd,config.GetServiceIntervalTV());

        logger->Info()<<"New TCP client connected, fd: "<<cSockFd;
        sender.SendMessage(this, NewClientMessage(std::make_shared<TCPConnection>(cSockFd),pathID));
    }

    if(close(lSockFd)!=0)
    {
        HandleError(errno,"Failed to close listen socket: ");
        return;
    }

    logger->Info()<<"Shuting down TCP listener"<<std::endl;
}

void TCPListener::OnShutdown()
{
    shutdownPending.store(true);
}
