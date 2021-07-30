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
        unsigned long pollInterval;
        uint8_t curMode;
        uint8_t sessionId;
        //filled on setup
        ResetHelper* resetHelper;
        HardwareSerial* uart;
        uint8_t* rxDataBuff;
        uint8_t* txDataBuff;
    public:
        void Setup(ResetHelper* const resetHelper, HardwareSerial* const uart, uint8_t * rxDataBuff, uint8_t * txDataBuff);
        unsigned long ProcessRequest(const Request& request);
        void ProcessRX();
        Response ProcessTX();
};

#endif // UARTWORKER_H
