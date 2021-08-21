#include "intervaltimer.h"

IntervalTimer::IntervalTimer()
{
    checkPoint=micros();
    interval=1;
}

void IntervalTimer::SetInterval(unsigned long intervalUsec)
{
    interval=intervalUsec>0?intervalUsec:1;
}

bool IntervalTimer::Update()
{
    //will overflow at some point, not a problem in this case
    return (micros()-checkPoint)>=interval;
}

void IntervalTimer::Reset()
{
    checkPoint=micros();
}

void IntervalTimer::Next()
{
    checkPoint+=interval;
}
