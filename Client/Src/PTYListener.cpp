#include "PTYListener.h"
#include "PTYConnection.h"

#include <cstdint>
#include <cstring>
#include <cerrno>
#include <string>

#include <pty.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/inotify.h>

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
    ptm=-1;
    pts=-1;
}

bool PTYListener::Startup()
{
    if(remoteConfig.ptsListener.empty()||remoteConfig.listener.port!=0)
    {
        HandleError("PTY symlink name is invalid or configuration mismatch");
        return false;
    }

    //create new PTY with openpty and get it's dev-filename
    if(openpty(&ptm,&pts,NULL,NULL,NULL)<0)
    {
        HandleError(errno,"openpty failed: ");
        return false;
    }

    return WorkerBase::Startup();
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
    auto ptrPTSName=ttyname(pts);
    if(ptrPTSName==nullptr)
    {
        HandleError(errno,"ttyname failed: ");
        return;
    }
    std::string ptsName(ptrPTSName);

    //close slave part before starting poll
    if(close(pts)<0)
    {
        HandleError(errno,"Failed to close slave pty: ");
        return;
    }

    //create inotify events
    auto inoFd=inotify_init();
    if(inoFd<0)
    {
        HandleError(errno,"inotify_init failed: ");
        return;
    }

    if(fcntl(inoFd, F_SETFL, O_NONBLOCK) < 0)
    {
        HandleError(errno,"fcntl for inoFd failed: ");
        return;
    }

    auto inoWatchFd=inotify_add_watch(inoFd,ptsName.c_str(),IN_OPEN|IN_CLOSE);
    if(inoWatchFd<0)
    {
        HandleError(errno,"inotify_add_watch failed: ");
        return;
    }

    //create symlink, close ptm on error
    if(symlink(ptsName.c_str(),remoteConfig.ptsListener.c_str())<0)
    {
        close(ptm);
        HandleError(errno,"Failed to create symlink for slave pty: ");
        return;
    }

    logger->Info()<<"Listening for incoming connection (PTY) at "<<remoteConfig.ptsListener<<" (PTS: "<<ptsName<<")"<<std::endl;

    pollfd lst={};
    lst.fd=inoFd;
    lst.events=POLLIN|POLLHUP;
    int openCount=0;

    while(!shutdownPending)
    {
        lst.revents=0;
        auto lrv=poll(&lst,1,config.GetServiceIntervalMS());
        if(lrv<0)
        {
            auto error=errno;
            if(error==EINTR)//interrupted by signal
                break;
            HandleError(error,"Error processing inotify event: ");
            return;
        }
        //logger->Info()<<"inotify poll tick";
        if((lst.revents&(POLLHUP|POLLERR))!=0)
        {
            HandleError("Error processing inotify watch: ");
            return;
        }
        if((lst.revents&POLLIN)!=0)
        {
            //read all events awailable
            while(true)
            {
                inotify_event event={};
                if(read(inoFd,&event,sizeof(inotify_event))!=sizeof(inotify_event))
                {
                    auto error=errno;
                    if(error!=EWOULDBLOCK)
                    {
                        HandleError(error,"Error reading inotify event: ");
                        return;
                    }
                    else
                        break;
                }
                if((event.mask&IN_OPEN)!=0)
                    openCount++;
                if((event.mask&IN_CLOSE)!=0)
                    openCount--;
            }
        }
        else
            continue;

        if(openCount==1) //first client connected
        {
            logger->Info()<<"PTY client connected to "<<remoteConfig.ptsListener;
            sender.SendMessage(this, NewClientMessage(std::make_shared<PTYConnection>(ptm),pathID));
        }
        else if(openCount==0) //last client disconnected
        {
            logger->Info()<<"PTY closed at "<<remoteConfig.ptsListener;
            //nothing to do, should be processed
        }
    }

    if(unlink(remoteConfig.ptsListener.c_str())!=0)
    {
        HandleError(errno,"Failed to remove PTY symlink: ");
        return;
    }

    if(inotify_rm_watch(inoFd,inoWatchFd)!=0)
    {
        HandleError(errno,"inotify_rm_watch failed: ");
        return;
    }

    if(close(inoFd)!=0)
    {
        HandleError(errno,"Failed to close inoFd: ");
        return;
    }

    if(close(ptm)!=0)
    {
        HandleError(errno,"Failed to close master PTY: ");
        return;
    }

    logger->Info()<<"Shuting down PTY listener"<<std::endl;
}

void PTYListener::OnShutdown()
{
    shutdownPending.store(true);
}
