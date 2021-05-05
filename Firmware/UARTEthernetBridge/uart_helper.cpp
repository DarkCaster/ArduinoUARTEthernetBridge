#include "uart_helper.h"
#include "configuration.h"



void UARTHelper::PrepareRXPin()
{
    pinMode(rxPin,INPUT_PULLUP);
}

void UARTHelper::Setup(HardwareSerial* const _port, const uint8_t _rxPin)
{
    port=_port;
    rxPin=_rxPin;
    isActive=false;
    PrepareRXPin();
}

void UARTHelper::Activate(int speed, uint8_t config, uint8_t _collectIntMS)
{
    collectIntMS=_collectIntMS;
    port->begin(speed,config);
    PrepareRXPin();
}

void UARTHelper::Deactivate()
{
    port->end();
}

