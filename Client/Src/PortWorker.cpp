#include "PortWorker.h"

PortWorker::PortWorker(std::shared_ptr<ILogger>& _logger, IMessageSender& _sender, const IConfig& _config, const int _portId, const bool _resetOnConnect):
    logger(_logger),
    sender(_sender),
    config(_config),
    portId(_portId),
    resetOnConnect(_resetOnConnect)
{
}

bool PortWorker::ReadyForMessage(const MsgType msgType)
{
    return false;
}

void PortWorker::OnMessage(const void* const, const IMessage& message)
{

}
