#include "DataProcessor.h"
#include "Command.h"

class SendPackageMessage: public ISendPackageMessage { public: SendPackageMessage(const bool _useTCP, uint8_t* const _package):ISendPackageMessage(_useTCP,_package){} };

DataProcessor::DataProcessor(std::shared_ptr<ILogger>& _logger, IMessageSender& _sender, const IConfig& _config, std::vector<std::shared_ptr<PortWorker> >& _portWorkers):
    logger(_logger),
    sender(_sender),
    config(_config),
    portWorkers(_portWorkers),
    txBuff(std::make_unique<uint8_t[]>(static_cast<size_t>(config.GetNetPackageSz())))
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

//TODO: embed time value from ITimerMessage into the outgoing request
void DataProcessor::OnPollEvent(const ITimerMessage& message)
{
    //caller timer-thread may change if timer interval updated, so lock there as precaution
    std::lock_guard<std::mutex> pollGuard(pollLock);

    //logger->Info()<<"Poll event, counter: "<<message.counter;

    //process data from the local connections, fill-up txBuffer
    bool useTCP=false;
    bool openTriggered=false;
    for(int i=0;i<config.GetPortCount();++i)
    {
        auto request=portWorkers[static_cast<size_t>(i)]->ProcessTX(message.counter,txBuff.get()+config.GetPortBuffOffset(i));
        if(request.type==ReqType::Open || request.type==ReqType::Close || request.type==ReqType::Reset)
            useTCP=true;
        if(request.type==ReqType::Open)
            openTriggered=true;
        Request::Write(request,i,txBuff.get());
    }

    //write counter to package header
    if(openTriggered)
    {
        logger->Info()<<"Sending remote poll interval: "<<config.GetRemotePollIntervalUS();
        WriteU32Value(static_cast<uint32_t>(config.GetRemotePollIntervalUS()),txBuff.get()+PKG_CNT_OFFSET);
    }
    else
        WriteU32Value(message.counter,txBuff.get()+PKG_CNT_OFFSET);

    if(!config.GetUDPEnabled())
        useTCP=true;

    //send data
    sender.SendMessage(this,SendPackageMessage(useTCP,txBuff.get()));
}

void DataProcessor::OnIncomingPackageEvent(const IIncomingPackageMessage& message)
{
    //may be ocassionally called simultaneously from TCP and UDP transport
    std::lock_guard<std::mutex> pushGuard(pushLock);
    //logger->Info()<<"Package event: "<<message.msgType;
    for(int i=0;i<config.GetPortCount();++i)
    {
        auto response=Response::Map(i,message.package);
        portWorkers[static_cast<size_t>(i)]->ProcessRX(response,message.package+config.GetPortBuffOffset(i));
    }
}
