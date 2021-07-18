#include "resethelper.h"

void ResetHelper::Setup(uint8_t _rstPin)
{
    rstPin=_rstPin;
    pinMode(rstPin, INPUT);
    startTime=0;
    rstTime=0;
    started=false;
}

void ResetHelper::StartReset(uint16_t msec)
{
    startTime=millis();
    rstTime=msec<1?1:msec;
    pinMode(rstPin, OUTPUT);
    digitalWrite(rstPin, LOW);
    started=true;
}

bool ResetHelper::ResetComplete()
{
    if(!started)
        return true;
    if(millis()-startTime<rstTime)
        return false;
    pinMode(rstPin, INPUT);
    started=false;
    return true;
}
