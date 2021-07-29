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
    else if(message.msgType==MSG_INCOMING_PACKAGE)
        OnIncomingPackageEvent(static_cast<const IIncomingPackageMessage&>(message));
}

void DataProcessor::OnPollEvent(const ITimerMessage& message)
{
    //caller timer-thread may change, and may be called in parallel ocasionally
    std::lock_guard<std::mutex> pollGuard(pollLock);

    logger->Info()<<"Poll event, time: "<<message.interval;
    //TODO: process data from the local connections, fill-up txBuffer
    //TODO: calculate new poll-time
    //TODO: send data
    //TODO: set poll-timer to the new value if needed
}

//TODO: must be processed at port-worker
/*void DataProcessor::OnPortOpenEvent(const IPortOpenMessage& message)
{
    //TODO: notify port-worker to generate "open" command on next PollEvent
    //TODO: notify PollTimer to fire PollEvent immediately
}

void DataProcessor::OnPortCloseEvent(const IPortCloseMessage& message)
{
    //TODO: notify port-worker to close
}*/

void DataProcessor::OnIncomingPackageEvent(const IIncomingPackageMessage& message)
{
    logger->Info()<<"Package event: "<<message.package;
    //TODO: decode data
    //TODO: write it to corresponding port-workers
}
