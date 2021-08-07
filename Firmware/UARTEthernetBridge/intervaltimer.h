#ifndef INTERVALTIMER_H
#define INTERVALTIMER_H

#include <Arduino.h>

//TODO: wrap into abstract class, and platform-specific implementation
class IntervalTimer
{
    private:
        unsigned long interval;
        unsigned long startTime;
        unsigned long lastDiff;
    public:
        IntervalTimer();
        void SetInterval(unsigned long intervalUsec);
        bool Update();
        void Reset(bool freshStart);
};

#endif // TIMER_H
