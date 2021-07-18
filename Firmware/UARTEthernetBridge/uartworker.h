#ifndef UARTWORKER_H
#define UARTWORKER_H

#include <Arduino.h>
#include "resethelper.h"
#include "command.h"
#include "databuffer.h"

class UARTWorker
{
    private:
        DataBuffer rxRingBuff;
        //filled on setup
        ResetHelper* resetHelper;
        HardwareSerial* uart;
        uint8_t const * rxDataBuff;
        uint8_t const * txDataBuff;
    public:
        void Setup(ResetHelper* const resetHelper, HardwareSerial* const uart, uint8_t const * rxDataBuff, uint8_t const * txDataBuff);
};

#endif // UARTWORKER_H
