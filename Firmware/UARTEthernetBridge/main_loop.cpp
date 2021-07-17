#include <Arduino.h>
#include <UIPEthernet.h>

#include "debug.h"
#include "configuration.h"
#include "main_loop.h"
#include "watchdog.h"
#include "watchdog_AVR.h"
#include "tcpserver.h"
#include "timer.h"
#include "resethelper.h"

//receive- and send- buffers
static uint8_t rxBuff[PACKAGE_SIZE];
static uint8_t txBuff[PACKAGE_SIZE];

//helper classes
static WatchdogAVR watchdog;
static Timer pollTimer;
static TCPServer tcpServer(rxBuff,txBuff,PACKAGE_SIZE,META_SZ,TCP_PORT);
static ResetHelper rstHelper[UART_COUNT];

//current client state
static bool tcpClientState;

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
    HardwareSerial *extUARTs[] = UART_DEFS; //TODO: setup uart-helpers
    uint8_t extUARTPins[] = UART_RX_PINS;
    uint8_t extRSTPins[] = UART_RST_PINS;
    for(int i=0;i<UART_COUNT;++i)
    {
        pinMode(extUARTPins[i],INPUT_PULLUP);
        rstHelper[i].Setup(extRSTPins[i]);
    }

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
            rstHelper[i].StartReset(RESET_TIME_MS);
        bool rstComplete=true;
        do
        {
            rstComplete=true;
            for(int i=0;i<UART_COUNT;++i)
                rstComplete&=rstHelper[i].ResetComplete();
        } while(!rstComplete);
        //blink LED pin indicating hardware setup is complete
        BLINK(25,75,5);
    }
    else
        BLINK(10,10,1);

    //initialize network, reset if no network cable detected
    STATUS(); LOG(F("DHCP start"));
    UIPEthernet.init(PIN_SPI_ENC28J60_CS);
    uint8_t macaddr[] = ENC28J60_MACADDR;
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

    //start TCP server
    tcpClientState=false;
    tcpServer.Start();

    //TODO: start UDP server

    STATUS(); LOG(F("Init complete!"));
    BLINK(10,0,1);

    //setup timer
    pollTimer.SetInterval(1000000);
    pollTimer.Reset(true);
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
    //if client is not connected, check the link state, and reboot on link-failure
    if(!tcpClientState)
        check_link_state();

    //process incoming data from TCP
    auto clientEvent=tcpServer.ProcessRX();

    //process TCP client event
    switch (clientEvent.type)
    {
        case ClientEventType::Connected:
            tcpClientState=true;
            break;
        case ClientEventType::Disconnected:
            tcpClientState=false;
            break;
        default:
            break;
    }

    //TODO: process incoming data from UDP, with respect to TCP client event

    //TODO: process payload on new request, write data to ring-buffer

    //TODO: setup timer based on currently configured uart speed

    //TODO: write buffered data to UART

    /*switch (clientEvent.type)
    {
        case ClientEventType::NewRequest:
            clientState.serverUDPPort=clientEvent.data.udpPort;
            break;
        case ClientEventType::NoEvent:
            break;
        case ClientEventType::Connected:
        case ClientEventType::Disconnected:
        default:
            clientState.connected=clientEvent.type==ClientEventType::Connected;
            clientState.clientAddr=clientEvent.data.remoteAddr;
            clientState.serverUDPPort=0;
            clientState.clientUDPPort=0;
            break;
    }*/

    //if poll interval has passed, process data from UART and send it
    if(pollTimer.Update())
    {
        pollTimer.Reset(false);
        //TODO: poll uart data

        if(tcpClientState)
        {
            //TODO: send data over UDP
            bool dataSent=false;
            //send data over TCP if sending it over UDP is not possible
            if(!dataSent)
                tcpServer.ProcessTX();
        }
    }
}
