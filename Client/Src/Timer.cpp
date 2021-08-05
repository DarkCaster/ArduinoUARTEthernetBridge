#include "Timer.h"
#include <chrono>

class TimerMessage: public ITimerMessage { public: TimerMessage(int64_t _interval):ITimerMessage(_interval){} };

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
    if(profilingEnabled)
        WorkerPRF();
    else
        WorkerNoPRF();
}

void Timer::WorkerPRF()
{
    logger->Info()<<"Starting up with interval: "<<reqIntervalUsec<<" usec";
    logger->Info()<<"Processing-time warnings enanled";

    const auto reqInterval=std::chrono::microseconds(reqIntervalUsec);
    auto interval=reqInterval;
    const auto startTime=std::chrono::steady_clock::now();
    auto begin=startTime;

    while(!shutdownPending.load())
    {
        //wait for interval
        if(interval.count()>0)
        {
            std::this_thread::sleep_for(interval);
            //update time for detecting too long processing times
            begin+=interval;
        }

        //logger->Info()<<"current interval: "<<interval.count();
        sender.SendMessage(this,TimerMessage(reqInterval.count()));

        //warn if processing takes too long
        auto now=std::chrono::steady_clock::now();
        if((now-begin)>reqInterval)
            logger->Warning()<<"Processing takes too long: "<<std::chrono::duration_cast<std::chrono::microseconds>(now-begin).count()<<" usec";
        begin=now;

        //tune wait interval for next round and start over
        interval=std::chrono::duration_cast<std::chrono::microseconds>(reqInterval-(now-startTime)%reqInterval);
    }

    logger->Info()<<"Timer shutdown";
}

void Timer::WorkerNoPRF()
{
    logger->Info()<<"Starting up with interval: "<<reqIntervalUsec<<" usec";

    const auto reqInterval=std::chrono::microseconds(reqIntervalUsec);
    const auto startTime=std::chrono::steady_clock::now();

    auto interval=reqInterval;
    while(!shutdownPending.load())
    {
        //wait for interval
        if(interval.count()>0)
            std::this_thread::sleep_for(interval);

        //logger->Info()<<"current interval: "<<interval.count();
        sender.SendMessage(this,TimerMessage(reqInterval.count()));

        //tune wait interval for next round and start over
        interval=std::chrono::duration_cast<std::chrono::microseconds>(reqInterval-(std::chrono::steady_clock::now()-startTime)%reqInterval);
    }

    logger->Info()<<"Timer shutdown";
}

void Timer::OnShutdown()
{
    shutdownPending.store(true);
}
