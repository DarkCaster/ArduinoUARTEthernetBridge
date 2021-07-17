#include "resethelper.h"

void ResetHelper::Setup(uint8_t _rstPin)
{
    rstPin=_rstPin;
    pinMode(rstPin, INPUT);
    startTime=0;
    rstTime=0;
}

void ResetHelper::StartReset(uint16_t msec)
{
    startTime=millis();
    rstTime=msec<1?1:msec;
    pinMode(rstPin, OUTPUT);
    digitalWrite(rstPin, LOW);
}

bool ResetHelper::ResetComplete()
{
    if(millis()-startTime<rstTime || rstTime<1)
        return false;
    pinMode(rstPin, INPUT);
    rstTime=0;
    return true;
}
