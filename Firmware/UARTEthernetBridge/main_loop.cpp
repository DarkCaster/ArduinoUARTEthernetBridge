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

//helper classes
static WatchdogAVR watchdog;
static UARTHelper uartHelpers[UART_COUNT];

//other stuff
static uint8_t macaddr[] = ENC28J60_MACADDR;
static unsigned long time;

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

    //setup ENC28J60 reset pin (as precaution)
    pinMode(PIN_ENC28J60_RST, INPUT);

    //setup UART-helpers
    HardwareSerial *extUARTs[] = UART_DEFS;
    uint8_t extUARTPins[] = UART_RX_PINS;
    uint8_t extRSTPins[] = UART_RST_PINS;
    uint16_t ports[]= NET_PORTS;
    for(int i=0;i<UART_COUNT;++i)
        uartHelpers[i].Setup(extUARTs[i], extUARTPins[i], extRSTPins[i], ports[i]);

    //wait PSU to become stable on cold boot
    if(!watchdog.IsSystemResetBoot())
        delay(COLD_BOOT_WARMUP);

    //perform ENC28J60 reset, https://github.com/njh/EtherCard/issues/238
    pinMode(PIN_ENC28J60_RST, OUTPUT);
    digitalWrite(PIN_ENC28J60_RST, LOW);
    delay(RESET_TIME_MS);
    digitalWrite(PIN_ENC28J60_RST, HIGH);
    pinMode(PIN_ENC28J60_RST, INPUT);

    //perform reset of connected MCUs on cold boot
    if(!watchdog.IsSystemResetBoot())
    {
        for(int i=0;i<UART_COUNT;++i)
            uartHelpers[i].StartReset();
        delay(RESET_TIME_MS);
        for(int i=0;i<UART_COUNT;++i)
            uartHelpers[i].StopReset();
        //blink LED pin indicating hardware setup is complete
        BLINK(25,75,5);
    }
    else
        BLINK(10,10,1);

    //initialize network, reset if no network cable detected
    STATUS(); LOG(F("DHCP start"));
    UIPEthernet.init(PIN_SPI_ENC28J60_CS);
    if (UIPEthernet.begin(macaddr) == 0)
    {
        if (UIPEthernet.hardwareStatus() == EthernetNoHardware)
        {
            STATUS(); LOG(F("Ethernet controller not detected or failed!"));
            BLINK(50,1950,3);
        }
        else if (UIPEthernet.linkStatus() == LinkOFF)
        {
            STATUS(); LOG(F("No link detected! Rebooting"));
            BLINK(250,1750,3);
        }
        else
        {
            STATUS(); LOG(F("DHCP failed"));
            BLINK(500,1500,3);
        }
        watchdog.SystemReset();
    }

    //start TCP server-listeners
    for(int i=0;i<UART_COUNT;++i)
        uartHelpers[i].Start();

    STATUS(); LOG(F("Init complete!"));
    BLINK(10,0,1);
    time=millis();
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
        result|=uartHelpers[p].RXStep1(time);

    //write data to UART
    for(uint8_t p=0;p<UART_COUNT;++p)
        uartHelpers[p].RXStep2();

    //read data incoming from UART
    for(uint8_t p=0;p<UART_COUNT;++p)
        uartHelpers[p].TXStep1(time);

    //transmit data back to TCP client
    for(uint8_t p=0;p<UART_COUNT;++p)
        uartHelpers[p].TXStep2();

    //if there are no connected uart-helpers, then check link status
    if(!result)
        check_link_state();

    time=millis();
}
