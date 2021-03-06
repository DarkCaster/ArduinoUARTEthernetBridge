#ifndef DATAPROCESSOR_H
#define DATAPROCESSOR_H

#include "ILogger.h"
#include "IConfig.h"
#include "IMessageSubscriber.h"
#include "IMessageSender.h"
#include "PortWorker.h"

#include <memory>
#include <mutex>
#include <vector>

class DataProcessor final : public IMessageSubscriber
{
    private:
        std::shared_ptr<ILogger> logger;
        IMessageSender& sender;
        const IConfig& config;
        std::vector<std::shared_ptr<PortWorker>> portWorkers;
        std::unique_ptr<uint8_t[]> txBuff;
        std::mutex pollLock;
        std::mutex pushLock;
    private:
        void OnPollEvent(const ITimerMessage& message);
        void OnIncomingPackageEvent(const IIncomingPackageMessage& message);
    public:
        DataProcessor(std::shared_ptr<ILogger>& logger, IMessageSender& sender, const IConfig& config, std::vector<std::shared_ptr<PortWorker>>& portWorkers);
        //methods for ISubscriber
        bool ReadyForMessage(const MsgType msgType) final;
        void OnMessage(const void* const source, const IMessage& message) final;
};

#endif // DATAPROCESSOR_H
