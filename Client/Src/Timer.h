#ifndef TIMER_H
#define TIMER_H

#include "IConfig.h"
#include "ILogger.h"
#include "IMessageSender.h"

#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>

class Timer
{
    private:
        std::shared_ptr<ILogger> logger;
        IMessageSender& sender;
        const IConfig &config;
        //following fields managed from background threads
        std::mutex manageLock;
        std::shared_ptr<std::thread> activeTimer;
        int staleTimersCount;
        void Worker(std::chrono::microseconds reqInterval);
    public:
        Timer(std::shared_ptr<ILogger>& logger, IMessageSender& sender, const IConfig& config);
        void Start(int64_t intervalUsec);
        void Stop();
        void Shutdown();
        void RequestShutdown();
};

#endif // TIMER_H
