#include "ConnectionWorker.h"
#include "IMessage.h"

#include <cstdint>
#include <cstring>
#include <cerrno>
#include <string>

#include <unistd.h>

class ShutdownMessage: public IShutdownMessage { public: ShutdownMessage(int _ec):IShutdownMessage(_ec){} };
class PathCollapsedMessage: public IPathCollapsedMessage { public: PathCollapsedMessage(const std::shared_ptr<Connection> &_local, const std::shared_ptr<Connection> &_remote, const int _pathID):
            IPathCollapsedMessage(_local,_remote,_pathID){}; };

ConnectionWorker::ConnectionWorker(std::shared_ptr<ILogger> &_logger, IMessageSender &_sender, const IConfig &_config, const RemoteConfig &_remoteConfig, int _pathID, bool _reader):
    logger(_logger),
    sender(_sender),
    config(_config),
    remoteConfig(_remoteConfig),
    pathID(_pathID),
    isReader(_reader)
{
    shutdownPending.store(false);
    local=nullptr;
    remote=nullptr;
}

void ConnectionWorker::HandleError(const std::string &message)
{
    logger->Error()<<message<<std::endl;
    sender.SendMessage(this,ShutdownMessage(1));
}

void ConnectionWorker::HandleError(int ec, const std::string &message)
{
    logger->Error()<<message<<strerror(ec)<<std::endl;
    sender.SendMessage(this,ShutdownMessage(ec));
}

void ConnectionWorker::Worker()
{
    logger->Info()<<"Starting up connection worker";
    while(!shutdownPending)
    {
        std::unique_lock<std::mutex> lock(opLock);
        while(remote==nullptr||local==nullptr)
        {
            newPathTrigger.wait(lock);
            if(shutdownPending)
                return;
        }
        auto reader=isReader?remote:local;
        auto writer=isReader?local:remote;
        lock.unlock();
        if(reader==nullptr||writer==nullptr)
            continue;
        //perform reading/writing until shutdown or fail
        auto buffer=std::make_unique<uint8_t[]>(config.GetUARTBuffSz());
        while(!shutdownPending)
        {
            //read something
            auto dr=read(reader->fd,buffer.get(),config.GetUARTBuffSz());
            if(dr<=0)
            {
                auto error=errno;
                if(dr<0 && (error==EWOULDBLOCK))
                    continue;
                if(dr==0)
                    error=-1;
                reader->Fail(error);
                {
                    std::lock_guard<std::mutex> tmpGuard(opLock);
                    remote=nullptr;
                    local=nullptr;
                }
                sender.SendMessage(this,PathCollapsedMessage(isReader?writer:reader,isReader?reader:writer,pathID));
                break;
            }

            //write something
            auto dl=dr;
            ssize_t dw=-1;
            while(dl>0)
            {
                dw=write(writer->fd,buffer.get()+dr-dl,dl);
                if(dw<0)
                    break;
                dl-=dw;
            }
            if(dw<0)
            {
                writer->Fail(errno);
                {
                    std::lock_guard<std::mutex> tmpGuard(opLock);
                    remote=nullptr;
                    local=nullptr;
                }
                sender.SendMessage(this,PathCollapsedMessage(isReader?writer:reader,isReader?reader:writer,pathID));
                break;
            }
        }
    }
    logger->Info()<<"Shuting down connection worker";
}

bool ConnectionWorker::ReadyForMessage(const MsgType msgType)
{
    return msgType==MSG_PATH_ESTABLISHED;
}

void ConnectionWorker::OnShutdown()
{
    shutdownPending.store(true);
    newPathTrigger.notify_one();
}

void ConnectionWorker::OnMessage(const void* const, const IMessage& message)
{
    auto peMessage=static_cast<const IPathEstablishedMessage&>(message);
    if(shutdownPending||peMessage.pathID!=pathID)
        return;
    {
        std::lock_guard<std::mutex> opGuard(opLock);
        if(local!=nullptr||remote!=nullptr)
        {
            logger->Warning()<<"Not attempting to start transferring data when there is another active connection";
            sender.SendMessage(this,PathCollapsedMessage(peMessage.local,peMessage.remote,pathID));
            return;
        }
        //set new local and remote for worker
        local=peMessage.local;
        remote=peMessage.remote;
    }
    newPathTrigger.notify_one();
}
