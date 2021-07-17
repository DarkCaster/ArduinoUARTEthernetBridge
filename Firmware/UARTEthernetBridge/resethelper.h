#ifndef RESETHELPER_H
#define RESETHELPER_H

#include <Arduino.h>

class ResetHelper
{
    private:
        unsigned long startTime;
        uint16_t rstTime;
        uint8_t rstPin;
    public:
        void Setup(uint8_t rstPin);
        void StartReset(uint16_t msec);
        bool ResetComplete();
};

#endif // RESETHELPER_H
