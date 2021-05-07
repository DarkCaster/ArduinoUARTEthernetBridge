#ifndef UART_HELPER_H
#define UART_HELPER_H

#include <Arduino.h>
#include <UIPServer.h>

class UARTHelper
{
    private:
        UIPServer server=UIPServer(0);
        HardwareSerial* uart;
        uint8_t rxPin;

        uint8_t collectIntMS;
        void PrepareRXPin();
    public:
        //run only once
        void Setup(HardwareSerial* const uart, const uint8_t rxPin, const uint16_t netPort);
        //main operation divided into non-blocking steps to ensure better concurrency
        bool RXStep1(); //read data from client, check client connection
        void RXStep2(); //write data to uart
        void TXStep1(unsigned long curTime); //collect data from uart
        void TXStep2(); //send data to client
};

/*
 *
 *
    private:

        //TODO: remove garbage

        static uint8_t reqBuff[REQ_BUFFER_LEN];
        static uint8_t reqPending = REQ_NONE;
        static uint8_t reqCurrent = REQ_NONE;
        static uint8_t reqLeft = 0;
        static uint8_t reqLen = 0;

        //stuff for writing data
        static uint8_t ansBuff[ANS_DATA_LEN];

        //stuff for remote client state
        static UIPClient remote;
        static int wdInterval = DEFAULT_WD_INTERVAL;
        static bool isConnected=false;
        */

#endif //UART_HELPER_H
