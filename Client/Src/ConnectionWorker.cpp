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
}

void ConnectionWorker::OnShutdown()
{
    shutdownPending.store(true);
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

}

bool ConnectionWorker::ReadyForMessage(const MsgType msgType)
{
    return msgType==MSG_PATH_ESTABLISHED;
}

void ConnectionWorker::OnMessage(const void* const, const IMessage& message)
{
    auto peMessage=static_cast<const IPathEstablishedMessage&>(message);

}
