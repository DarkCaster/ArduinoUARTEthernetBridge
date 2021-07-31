#include "DataProcessor.h"
#include "Command.h"

class SendPackageMessage: public ISendPackageMessage { public: SendPackageMessage(const bool _useTCP, uint8_t* const _package):ISendPackageMessage(_useTCP,_package){} };

DataProcessor::DataProcessor(std::shared_ptr<ILogger>& _logger, IMessageSender& _sender, const IConfig& _config, std::vector<std::shared_ptr<PortWorker> >& _portWorkers):
    logger(_logger),
    sender(_sender),
    config(_config),
    portWorkers(_portWorkers),
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

void DataProcessor::OnPollEvent(const ITimerMessage& /*message*/)
{
    //caller timer-thread may change if timer interval updated, so lock there as precaution
    std::lock_guard<std::mutex> pollGuard(pollLock);

    //logger->Info()<<"Poll event, time: "<<message.interval;

    //process data from the local connections, fill-up txBuffer
    bool useTCP=false;
    for(int i=0;i<config.GetPortCount();++i)
    {
        auto request=portWorkers[static_cast<size_t>(i)]->ProcessTX(txBuff.get()+config.GetPortBuffOffset(i));
        if(request.type==ReqType::Open || request.type==ReqType::Reset)
            useTCP=true;
        Request::Write(request,i,txBuff.get());
    }

    if(config.GetUDPPort()<1)
        useTCP=true;

    //send data
    sender.SendMessage(this,SendPackageMessage(useTCP,txBuff.get()));
}

void DataProcessor::OnIncomingPackageEvent(const IIncomingPackageMessage& message)
{
    //may be ocassionally called simultaneously from TCP and UDP transport
    std::lock_guard<std::mutex> pushGuard(pushLock);
    //logger->Info()<<"Package event: "<<message.package;
    for(int i=0;i<config.GetPortCount();++i)
    {
        auto response=Response::Map(i,message.package);
        portWorkers[static_cast<size_t>(i)]->ProcessRX(response,message.package+config.GetPortBuffOffset(i));
    }
}
