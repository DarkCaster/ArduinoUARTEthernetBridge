#ifndef TIMER_H
#define TIMER_H

#include "IConfig.h"
#include "ILogger.h"
#include "IMessageSender.h"
#include "WorkerBase.h"

#include <cstdint>
#include <atomic>

class Timer : public WorkerBase
{
    private:
        std::shared_ptr<ILogger> logger;
        IMessageSender& sender;
        const IConfig &config;
        const int64_t reqIntervalUsec;
        const bool profilingEnabled;
        std::atomic<bool> shutdownPending;
    protected:
        void WorkerPRF();
        void WorkerNoPRF();
    public:
        Timer(std::shared_ptr<ILogger>& logger, IMessageSender& sender, const IConfig& config, const int64_t intervalUsec, const bool profilingEnabled);
    protected:
        //WorkerBase
        void Worker() final;
        void OnShutdown() final;
};

#endif // TIMER_H
