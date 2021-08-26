#ifndef TIMER_H
#define TIMER_H

#include "IConfig.h"
#include "ILogger.h"
#include "IMessageSender.h"
#include "IMessageSubscriber.h"
#include "WorkerBase.h"

#include <cstdint>
#include <atomic>

class Timer : public WorkerBase, public IMessageSubscriber
{
    private:
        std::shared_ptr<ILogger> logger;
        IMessageSender& sender;
        const IConfig &config;
        const int64_t reqIntervalUsec;
        std::atomic<bool> shutdownPending;
        std::atomic<bool> connectPending;
        uint32_t eventCounter;
    public:
        Timer(std::shared_ptr<ILogger>& logger, IMessageSender& sender, const IConfig& config, const int64_t intervalUsec);
        //methods for ISubscriber
        bool ReadyForMessage(const MsgType msgType) final;
        void OnMessage(const void* const source, const IMessage& message) final;
    protected:
        //WorkerBase
        void Worker() final;
        void OnShutdown() final;
};

#endif // TIMER_H
