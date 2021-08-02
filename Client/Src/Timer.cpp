#include "Timer.h"

class TimerMessage: public ITimerMessage { public: TimerMessage(int64_t _interval):ITimerMessage(_interval){} };

Timer::Timer(std::shared_ptr<ILogger>& _logger, IMessageSender& _sender,const IConfig& _config):
    logger(_logger),
    sender(_sender),
    config(_config)
{
    staleTimersCount=0;
}

void Timer::Worker(std::chrono::microseconds reqInterval)
{
    auto interval=reqInterval;
    auto startTime=std::chrono::steady_clock::now();
    while(true)
    {
        //wait for interval
        if(interval.count()>0)
            std::this_thread::sleep_for(interval);
        //check are we still an active timer, exit loop if not
        {
            std::lock_guard<std::mutex> opGuard(manageLock);
            if(activeTimer==nullptr || activeTimer->get_id()!=std::this_thread::get_id())
                break;
        }
        //send message
        //logger->Info()<<"current interval: "<<interval.count();
        sender.SendMessage(this,TimerMessage(reqInterval.count()));
        //tune wait interval for next round and start over
        interval=std::chrono::duration_cast<std::chrono::microseconds>(reqInterval-(std::chrono::steady_clock::now()-startTime)%reqInterval);
    }

    //stop sequence
    std::lock_guard<std::mutex> opGuard(manageLock);
    staleTimersCount--;
}

void Timer::Start(int64_t intervalUsec)
{
    std::lock_guard<std::mutex> opGuard(manageLock);
    //move current timer away
    if(activeTimer!=nullptr)
    {
        staleTimersCount++;
        activeTimer->detach();
        logger->Info()<<"Old timer was detached";
    }
    activeTimer=std::make_shared<std::thread>([=] { Worker(std::chrono::microseconds(intervalUsec)); });
    logger->Info()<<"Interval set to: "<<intervalUsec<<" usec";
}

void Timer::Stop()
{
    std::lock_guard<std::mutex> opGuard(manageLock);
    //move current timer away
    if(activeTimer!=nullptr)
    {
        staleTimersCount++;
        activeTimer->detach();
    }
    activeTimer=nullptr;
}

void Timer::RequestShutdown()
{
    Stop();
}

void Timer::Shutdown()
{
    logger->Info()<<"Shuting down timer";
    Stop();
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(config.GetServiceIntervalMS()));
        std::lock_guard<std::mutex> opGuard(manageLock);
        if(staleTimersCount<1)
            break;
    }
    logger->Info()<<"Timer was shutdown";
}
