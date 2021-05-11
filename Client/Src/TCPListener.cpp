#include "TCPListener.h"

#include <cstdint>
#include <cstring>
#include <cerrno>
#include <string>

#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

class ShutdownMessage: public IShutdownMessage { public: ShutdownMessage(int _ec):IShutdownMessage(_ec){} };
class NewClientMessage: public INewClientMessage { public: NewClientMessage(int _fd, const RemoteConfig &_remoteConfig):INewClientMessage(_fd,_remoteConfig){} };

TCPServerListener::TCPServerListener(std::shared_ptr<ILogger> &_logger, IMessageSender &_sender, const IConfig &_config, const RemoteConfig &_remoteConfig):
    logger(_logger),
    sender(_sender),
    config(_config),
    remoteConfig(_remoteConfig)
{
    shutdownPending.store(false);
}

void TCPServerListener::HandleError(const std::string &message)
{
    logger->Error()<<message<<std::endl;
    sender.SendMessage(this,ShutdownMessage(1));
}

void TCPServerListener::HandleError(int ec, const std::string &message)
{
    logger->Error()<<message<<strerror(ec)<<std::endl;
    sender.SendMessage(this,ShutdownMessage(ec));
}

void TCPServerListener::Worker()
{ 
    if(!remoteConfig.listener.address.isValid)
    {
        HandleError("Listen IP address is invalid");
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

        //accept single connection
        //sockaddr_storage cAddr;
        //socklen_t cAddrSz = sizeof(cAddr);
        //auto cSockFd=accept(lSockFd,reinterpret_cast<sockaddr*>(&cAddr),&cAddrSz);
        auto cSockFd=accept(lSockFd,nullptr,nullptr);
        if(cSockFd<1)
        {
            logger->Warning()<<"Failed to accept connection: "<<strerror(errno)<<std::endl;
            continue;
        }

        logger->Info()<<"New TCP client connected, fd: "<<cSockFd;
        sender.SendMessage(this, NewClientMessage(cSockFd,remoteConfig));
    }

    if(close(lSockFd)!=0)
    {
        HandleError(errno,"Failed to close listen socket: ");
        return;
    }

    logger->Info()<<"Shuting down DNSReceiver worker thread"<<std::endl;
}

void TCPServerListener::OnShutdown()
{
    shutdownPending.store(true);
}
