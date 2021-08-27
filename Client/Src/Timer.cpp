#include "Timer.h"
#include <chrono>

class TimerMessage: public ITimerMessage { public: TimerMessage(uint32_t _counter):ITimerMessage(_counter){} };

Timer::Timer(std::shared_ptr<ILogger>& _logger, IMessageSender& _sender, const IConfig& _config, const int64_t _intervalUsec):
    logger(_logger),
    sender(_sender),
    config(_config),
    reqIntervalUsec(_intervalUsec)
{
    shutdownPending.store(false);
    connectPending.store(true);
    eventCounter=0;
}

bool Timer::ReadyForMessage(const MsgType msgType)
{
    return msgType==MSG_CONNECTED;
}

void Timer::OnMessage(const void* const /*source*/, const IMessage& /*message*/)
{
    connectPending.store(false);
}

void Timer::Worker()
{
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
        sender.SendMessage(this,TimerMessage(++eventCounter));

        auto now=std::chrono::steady_clock::now();
        //if(profilingEnabled)
        //{
        auto processTime=now-prev;
        if(processTime>reqInterval && !connectPending.load())
            logger->Warning()<<"Processing takes too long: "<<std::chrono::duration_cast<std::chrono::microseconds>(processTime).count()<<" usec; event: "<<eventCounter;
        //}

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
