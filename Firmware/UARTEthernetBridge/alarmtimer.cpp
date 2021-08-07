#include "alarmtimer.h"

AlarmTimer::AlarmTimer()
{
    startTime=millis();
    interval=1;
    lastDiff=0;
}

void AlarmTimer::SetAlarmDelay(unsigned long intervalMS)
{
    interval=intervalMS;
}

bool AlarmTimer::AlarmTriggered()
{
    return millis()-startTime>interval;
}

void AlarmTimer::SnoozeAlarm()
{
    startTime=millis();
}
