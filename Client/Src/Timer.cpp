#include "Timer.h"

Timer::Timer()
{
    startTime=std::chrono::steady_clock::now();
    interval=std::chrono::microseconds(1);
    lastDiff=std::chrono::microseconds::zero();
}

void Timer::SetInterval(int64_t intervalUsec)
{
    interval=std::chrono::microseconds(intervalUsec>0?intervalUsec:1);
}

std::chrono::microseconds Timer::TimeLeft()
{
    lastDiff=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-startTime);
    auto result=interval-lastDiff;
    return result.count()<0?std::chrono::microseconds::zero():result;
}

bool Timer::Update()
{
    return TimeLeft().count()<1;
}

void Timer::Reset(bool freshStart)
{
    if(freshStart)
    {
        startTime=std::chrono::steady_clock::now();
        lastDiff=std::chrono::microseconds::zero();
        return;
    }
    //time must be rounded down to the interval border
    startTime+=(lastDiff/interval)*interval;
}
