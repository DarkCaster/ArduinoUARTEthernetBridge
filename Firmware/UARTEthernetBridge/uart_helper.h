#ifndef UART_HELPER_H
#define UART_HELPER_H

#include <Arduino.h>

class UARTHelper
{
    private:
        HardwareSerial* port;
        uint8_t rxPin;
        uint8_t collectIntMS;
        bool isActive;

        void PrepareRXPin();
    public:
        void Setup(HardwareSerial* const port, const uint8_t rxPin);
        void Activate(int speed, uint8_t config, uint8_t collectIntMS);
        void Deactivate();
};

#endif //UART_HELPER_H
