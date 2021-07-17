#ifndef TIMER_H
#define TIMER_H

#include <Arduino.h>

class Timer
{
    private:
        unsigned long interval;
        unsigned long startTime;
        unsigned long lastDiff;
    public:
        Timer();
        void SetInterval(unsigned long intervalUsec);
        bool Update();
        void Reset(bool freshStart);
};

#endif // TIMER_H
