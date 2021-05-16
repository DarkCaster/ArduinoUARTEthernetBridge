#ifndef UART_HELPER_H
#define UART_HELPER_H

#include <Arduino.h>
#include <UIPServer.h>
#include <UIPClient.h>

#include "configuration.h"
#include "segmented_buffer.h"
#include "uart_config.h"

class UARTHelper
{
    private:
        //fields set with Setup call
        UIPServer server=UIPServer(0);
        HardwareSerial* uart;
        uint8_t rxPin;
        uint8_t rstPin;
        uint16_t netPort;
        //mutable fields
        UIPClient client;
        SegmentedBuffer rxStorage;
        uint8_t txBuffer[UART_BUFFER_SIZE];
        int txSize;
        UARTConfig config;
        uint8_t state; //0 - not connected, 1-254 - configuration steps, 255 - ready to transfer data
        unsigned long targetTxTime;
        unsigned long targetRstTime;
        bool Connect();
        bool ReadConfig(unsigned long curTime);
        bool UARTOpen();
        bool ResetBegin(unsigned long curTime);
        bool ResetEnd(unsigned long curTime);
        bool Disconnect();
    public:
        //run only once
        void Setup(HardwareSerial* const uart, const uint8_t rxPin, const uint8_t rstPin, const uint16_t netPort);
        void Start();
        //main operation divided into non-blocking steps to ensure better concurrency
        bool RXStep1(unsigned long curTime); //read data from client, check client connection
        void RXStep2(); //write data to uart
        void TXStep1(unsigned long curTime); //collect data from uart
        void TXStep2(); //send data to client
};

#endif //UART_HELPER_H
