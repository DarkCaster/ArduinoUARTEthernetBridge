#include "timer.h"

IntervalTimer::IntervalTimer()
{
    startTime=micros();
    interval=1;
    lastDiff=0;
}

void IntervalTimer::SetInterval(unsigned long intervalUsec)
{
    interval=intervalUsec>0?intervalUsec:1;
}

bool IntervalTimer::Update()
{
    //will overflow at some point, not a problem in this case
    lastDiff=micros()-startTime;
    return lastDiff>=interval;
}

void IntervalTimer::Reset(bool freshStart)
{
    if(freshStart)
    {
        startTime=micros();
        lastDiff=0;
        return;
    }
    //time must be rounded down to the interval border
    startTime+=(lastDiff/interval)*interval;
}
