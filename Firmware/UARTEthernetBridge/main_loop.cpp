#include <Arduino.h>
#include <UIPEthernet.h>
#include <UIPServer.h>
#include <UIPClient.h>

#include "debug.h"
#include "configuration.h"
#include "main_loop.h"
#include "watchdog.h"
#include "watchdog_AVR.h"
#include "uart_helper.h"

static uint8_t macaddr[] = ENC28J60_MACADDR;

//helper classes
static WatchdogAVR watchdog;
static UARTHelper uartHelpers[UART_COUNT];

void setup()
{
    SETUP_DEBUG_SERIAL();
    STATUS(); LOG(F("Startup"));

    //setup SPI pins
    pinMode(PIN_SPI_MISO,INPUT_PULLUP);
    pinMode(PIN_SPI_MOSI,OUTPUT);
    digitalWrite(PIN_SPI_MOSI,LOW);
    pinMode(PIN_SPI_ENC28J60_CS,OUTPUT);
    digitalWrite(PIN_SPI_ENC28J60_CS,LOW);
    pinMode(PIN_SPI_SCK,OUTPUT);
    digitalWrite(PIN_SPI_SCK,LOW);

    //perform ENC28J60 reset, https://github.com/njh/EtherCard/issues/238
    pinMode(PIN_ENC28J60_RST, OUTPUT);
    digitalWrite(PIN_ENC28J60_RST, LOW);
    delay(100);
    digitalWrite(PIN_ENC28J60_RST, HIGH);
    pinMode(PIN_ENC28J60_RST, INPUT);

    //setup UART-helpers
    HardwareSerial *extUARTs[] = UART_DEFS;
    uint8_t extUARTPins[] = UART_RX_PINS;
    uint16_t ports[]= NET_PORTS;
    for(int i=0;i<UART_COUNT;++i)
        uartHelpers[i].Setup(extUARTs[i], extUARTPins[i], ports[i]);

    //blink LED pin indicating hardware setup is complete
    BLINK(50,50,10);

    //initialize network, reset if no network cable detected
    STATUS(); LOG(F("DHCP start"));
    UIPEthernet.init(PIN_SPI_ENC28J60_CS);
    if (UIPEthernet.begin(macaddr) == 0)
    {
        if (UIPEthernet.hardwareStatus() == EthernetNoHardware)
            FAIL(250,250);
        if (UIPEthernet.linkStatus() == LinkOFF)
        {
            BLINK(500,500,3);
            watchdog.SystemReset();
        }
    }

    STATUS(); LOG(F("Init complete!"));
    BLINK(10,0,1);
}

void check_link_state()
{
    //check link state
    if(UIPEthernet.linkStatus()!=LinkON)
    {
        STATUS(); LOG(F("Link disconnected! Rebooting"));
        BLINK(500,500,3);
        watchdog.SystemReset();
    }

}

void loop()
{
    bool result=false;

    //receive data incoming from TCP client
    for(uint8_t p=0;p<UART_COUNT;++p)
        result|=uartHelpers[p].RXStep1();

    //write data to UART
    for(uint8_t p=0;p<UART_COUNT;++p)
        uartHelpers[p].RXStep2();

    //read data incoming from UART
    for(uint8_t p=0;p<UART_COUNT;++p)
        uartHelpers[p].TXStep1();

    //transmit data back to TCP client
    for(uint8_t p=0;p<UART_COUNT;++p)
        uartHelpers[p].TXStep2();

    //if there are no connected uart-helpers, then check link status
    if(!result)
        check_link_state();
}
