#include "PTYListener.h"

#include <cstdint>
#include <cstring>
#include <cerrno>
#include <string>

#include <pty.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>

class ShutdownMessage: public IShutdownMessage { public: ShutdownMessage(int _ec):IShutdownMessage(_ec){} };
class NewClientMessage: public INewClientMessage { public: NewClientMessage(std::shared_ptr<Connection> _client, int _pathID):INewClientMessage(_client,_pathID){} };

PTYListener::PTYListener(std::shared_ptr<ILogger> &_logger, IMessageSender &_sender, const IConfig &_config, const RemoteConfig &_remoteConfig, int _pathID):
    logger(_logger),
    sender(_sender),
    config(_config),
    remoteConfig(_remoteConfig),
    pathID(_pathID)
{
    shutdownPending.store(false);
}

void PTYListener::HandleError(const std::string &message)
{
    logger->Error()<<message<<std::endl;
    sender.SendMessage(this,ShutdownMessage(1));
}

void PTYListener::HandleError(int ec, const std::string &message)
{
    logger->Error()<<message<<strerror(ec)<<std::endl;
    sender.SendMessage(this,ShutdownMessage(ec));
}

void PTYListener::Worker()
{
    if(remoteConfig.ptsListener.empty()||remoteConfig.listener.port!=0)
    {
        HandleError("PTY symlink name is invalid or configuration mismatch");
        return;
    }

    //create new PTY with openpty and get it's dev-filename
    int ptm=-1;
    int pts=-1;
    if(openpty(&ptm,&pts,NULL,NULL,NULL)<0)
    {
        HandleError(errno,"openpty failed: ");
        return;
    }
    auto ptrPTSName=ttyname(pts);
    if(ptrPTSName==nullptr)
    {
        HandleError(errno,"ttyname failed: ");
        return;
    }
    std::string ptsName(ptrPTSName);
    //open+close slave part before starting to poll
    auto tmpFd=open(ptsName.c_str(), O_RDWR | O_NOCTTY);
    if(tmpFd<0)
    {
        HandleError(errno,"Failed to open slave pty: ");
        return;
    }
    if(close(tmpFd)<0)
    {
        HandleError(errno,"Failed to close slave pty: ");
        return;
    }

    //create symlink, close ptm on error
    if(symlink(ptsName.c_str(),remoteConfig.ptsListener.c_str())<0)
    {
        close(ptm);
        HandleError(errno,"Failed to create symlink for slave pty: ");
        return;
    }

    logger->Info()<<"Listening for incoming connection (PTY) at "<<remoteConfig.ptsListener.c_str()<<std::endl;

    pollfd lst={};
    lst.fd=ptm;
    lst.events=POLLHUP;
    bool ptsConnected=false;
    bool ptsDisconnectPending=false;
    while(!shutdownPending)
    {
        lst.revents=0;
        auto lrv=poll(&lst,1,config.GetServiceIntervalMS());
        if(lrv<0)
        {
            auto error=errno;
            if(error==EINTR)//interrupted by signal
                break;
            close(ptm);
            HandleError(error,"Error awaiting incoming connection: ");
            return;
        }
        ptsConnected=(lst.events&POLLHUP)==0;
        logger->Info()<<lst.events;
        if(ptsDisconnectPending)
        {
            if(ptsConnected)
                continue;
            logger->Info()<<"Slave PTY disconnected at "<<remoteConfig.ptsListener;
            ptsDisconnectPending=false;
            continue;
        }
        if(!ptsConnected)
            continue;
        logger->Info()<<"New PTY client connected at "<<remoteConfig.ptsListener;
        //sender.SendMessage(this, NewClientMessage(std::make_shared<TCPConnection>(cSockFd),pathID));
    }

    if(close(ptm)!=0)
    {
        HandleError(errno,"Failed to close master PTY: ");
        return;
    }

    if(unlink(remoteConfig.ptsListener.c_str())!=0)
    {
        HandleError(errno,"Failed to remove PTY symlink: ");
        return;
    }

    logger->Info()<<"Shuting down PTY listener"<<std::endl;
}

void PTYListener::OnShutdown()
{
    shutdownPending.store(true);
}
