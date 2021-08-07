#ifndef ALARMTIMER_H
#define ALARMTIMER_H

#include <Arduino.h>

//TODO: wrap into abstract class with platform-specific implementation
class AlarmTimer
{
    private:
        unsigned long interval;
        unsigned long startTime;
        unsigned long lastDiff;
    public:
        AlarmTimer();
        void SetAlarmDelay(unsigned long intervalMS);
        bool AlarmTriggered();
        void SnoozeAlarm();
};

#endif // ALARMTIMER_H
