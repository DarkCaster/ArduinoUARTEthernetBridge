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
        //mutable fields
        UIPClient client;
        SegmentedBuffer rxStorage;
        uint8_t txBuffer[UART_BUFFER_SIZE];
        UARTConfig config;
        uint8_t state; //0 - not connected, 1-254 - configuration steps, 255 - ready to transfer data
        bool Connect();
        bool ReadConfig();
        bool UARTOpen();
        bool ResetBegin();
        bool ResetEnd();
        bool Disconnect();
    public:
        //run only once
        void Setup(HardwareSerial* const uart, const uint8_t rxPin, const uint16_t netPort);
        //main operation divided into non-blocking steps to ensure better concurrency
        bool RXStep1(); //read data from client, check client connection
        void RXStep2(); //write data to uart
        void TXStep1(unsigned long curTime); //collect data from uart
        void TXStep2(); //send data to client
};

#endif //UART_HELPER_H
