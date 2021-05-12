#include "TCPClient.h"

TCPClient::TCPClient(std::shared_ptr<ILogger> &_logger, IMessageSender &_sender, const bool _connectOnStart, const IConfig &_config, const RemoteConfig &_remoteConfig, const int _pathID):
    logger(_logger),
    sender(_sender),
    connectOnStart(_connectOnStart),
    config(_config),
    remoteConfig(_remoteConfig),
    pathID(_pathID)
{
    shutdownPending.store(false);
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


