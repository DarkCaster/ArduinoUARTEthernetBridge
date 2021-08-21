#ifndef INTERVALTIMER_H
#define INTERVALTIMER_H

#include <Arduino.h>

//TODO: wrap into abstract class, and platform-specific implementation
class IntervalTimer
{
    private:
        unsigned long interval;
        unsigned long checkPoint;
    public:
        IntervalTimer();
        void SetInterval(unsigned long intervalUsec);
        bool Update();
        void Reset();
        void Next();
};

#endif // TIMER_H
