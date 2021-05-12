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
}

bool TCPClient::ReadyForMessage(const MsgType msgType)
{
    return msgType==MSG_NEW_CLIENT;
}

void TCPClient::Worker()
{
    //connect on start
}

void TCPClient::OnMessage(const void* const, const IMessage& message)
{
    //check if message designated for this particular client
    //connect client if not already connected / check connection status / reconnect if failed
    //produce result-message to the workers
}


