#include "DataProcessor.h"

class SendPackageMessage: public ISendPackageMessage { public: SendPackageMessage(const bool _useTCP, uint8_t* const _package):ISendPackageMessage(_useTCP,_package){} };

DataProcessor::DataProcessor(std::shared_ptr<ILogger>& _logger, IMessageSender& _sender, const IConfig& _config):
    logger(_logger),
    sender(_sender),
    config(_config),
    txBuff(std::make_unique<uint8_t[]>(static_cast<size_t>(config.GetPackageSz())))
{
}

bool DataProcessor::ReadyForMessage(const MsgType msgType)
{
    return msgType==MSG_TIMER || msgType==MSG_INCOMING_PACKAGE;
}

void DataProcessor::OnMessage(const void* const, const IMessage& message)
{
    if(message.msgType==MSG_TIMER)
        OnPollEvent(static_cast<const ITimerMessage&>(message));
    if(message.msgType==MSG_INCOMING_PACKAGE)
        OnIncomingPackageEvent(static_cast<const IIncomingPackageMessage&>(message));
}

void DataProcessor::OnPollEvent(const ITimerMessage& message)
{
    logger->Info()<<"Poll event, time: "<<message.interval;
    //process data from the

}

void DataProcessor::OnIncomingPackageEvent(const IIncomingPackageMessage& message)
{
    logger->Info()<<"Package event: "<<message.package;
}

