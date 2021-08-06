#include "Timer.h"
#include <chrono>

class TimerMessage: public ITimerMessage { public: TimerMessage(int64_t _time):ITimerMessage(_time){} };

Timer::Timer(std::shared_ptr<ILogger>& _logger, IMessageSender& _sender, const IConfig& _config, const int64_t _intervalUsec, const bool _profilingEnabled):
    logger(_logger),
    sender(_sender),
    config(_config),
    reqIntervalUsec(_intervalUsec),
    profilingEnabled(_profilingEnabled)
{
    shutdownPending.store(false);
}

void Timer::Worker()
{
    logger->Info()<<"Starting up with interval: "<<reqIntervalUsec<<" usec";
    if(profilingEnabled)
        logger->Info()<<"Package processing-time profiling enabled";

    const auto reqInterval=std::chrono::microseconds(reqIntervalUsec);
    const auto startTime=std::chrono::steady_clock::now();
    auto prev = startTime;

    auto interval=reqInterval;
    while(!shutdownPending.load())
    {
        //wait for interval
        if(interval.count()>0)
            std::this_thread::sleep_for(interval);

        prev+=interval;
        sender.SendMessage(this,TimerMessage(std::chrono::duration_cast<std::chrono::microseconds>(prev-startTime).count()));

        auto now=std::chrono::steady_clock::now();
        if(profilingEnabled)
        {
            auto processTime=now-prev;
            if(processTime>reqInterval)
                logger->Warning()<<"Processing takes too long: "<<std::chrono::duration_cast<std::chrono::microseconds>(processTime).count()<<" usec";
        }

        //tune wait interval for next round and start over
        interval=std::chrono::duration_cast<std::chrono::microseconds>(reqInterval-(now-startTime)%reqInterval);
        prev=now;
    }

    logger->Info()<<"Timer shutdown";
}

void Timer::OnShutdown()
{
    shutdownPending.store(true);
}
