#include <Arduino.h>
#include <UIPEthernet.h>

#include "configuration.h"
#include "main_loop.h"
#include "watchdog.h"
#include "watchdog_AVR.h"
#include "tcpserver.h"
#include "udpserver.h"
#include "timer.h"
#include "resethelper.h"
#include "uartworker.h"
#include "command.h"

//receive- and send- buffers
static uint8_t rxBuff[PACKAGE_SIZE];
static uint8_t txBuff[PACKAGE_SIZE];

//helper classes
static WatchdogAVR watchdog;
static Timer pollTimer;
static TCPServer tcpServer(rxBuff,txBuff,PACKAGE_SIZE,META_SZ,TCP_PORT);
static UDPServer udpServer(rxBuff,txBuff,PACKAGE_SIZE,META_SZ);
static ResetHelper rstHelper[UART_COUNT];
static UARTWorker uartWorker[UART_COUNT];

//current client state
static bool tcpClientState;
static unsigned long uartPollTimes[UART_COUNT];

static void blink(uint16_t blinkTime, uint16_t pauseTime, uint8_t count)
{
    while(true)
    {
        digitalWrite(STATUS_LED,HIGH);
        delay(blinkTime);
        if(pauseTime>0)
        {
            digitalWrite(STATUS_LED,LOW);
            delay(pauseTime);
        }
        if(count==1)
            return;
        if(count>0)
            count--;
    }
}

void setup()
{
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

    //status pin
    pinMode(STATUS_LED, OUTPUT);

    //setup UART-workers
    HardwareSerial *extUARTs[] = UART_DEFS;
    uint8_t extUARTPins[] = UART_RX_PINS;
    uint8_t extRSTPins[] = UART_RST_PINS;
    for(int i=0;i<UART_COUNT;++i)
    {
        uartPollTimes[i]=IDLE_POLL_INTERVAL_US;
        pinMode(extUARTPins[i],INPUT_PULLUP);
        rstHelper[i].Setup(extRSTPins[i]);
        uartWorker[i].Setup(&(rstHelper[i]),extUARTs[i],rxBuff+META_SZ+META_CRC_SZ+UART_BUFFER_SIZE*i,txBuff+META_SZ+META_CRC_SZ+UART_BUFFER_SIZE*i);
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
        blink(50,50,10);
    }
    else
        blink(10,10,1);

    //initialize network, reset if no network cable detected
    UIPEthernet.init(PIN_SPI_ENC28J60_CS);
    uint8_t macaddr[] = ENC28J60_MACADDR;
    if (UIPEthernet.begin(macaddr) == 0)
    {
        if (UIPEthernet.hardwareStatus() == EthernetNoHardware)
            blink(50,950,6);
        else if (UIPEthernet.linkStatus() == LinkOFF)
            //Cable not inserted
            blink(500,500,6);
        else
            //DHCP failed
            blink(1000,1000,3);
        watchdog.SystemReset();
    }

    //start TCP server
    tcpClientState=false;
    tcpServer.Start();

    blink(0,0,1);

    //setup timer
    pollTimer.SetInterval(IDLE_POLL_INTERVAL_US);
    pollTimer.Reset(true);
}

static void check_link_state()
{
    //check link state
    if(UIPEthernet.linkStatus()!=LinkON)
    {
        blink(500,500,6);
        watchdog.SystemReset();
    }
}

static unsigned long GetMinPollTime()
{
    unsigned long minPollTime=IDLE_POLL_INTERVAL_US;
    for(int i=0;i<UART_COUNT;++i)
        if(uartPollTimes[i]<minPollTime)
            minPollTime=uartPollTimes[i];
    return minPollTime;
}

void loop()
{
    //if client is not connected, check the link state, and reboot on link-failure
    if(!tcpClientState)
    {
        check_link_state();
        //add small delay to speed down SPI communication with ethernet adapter while not connected
        delay(25);
    }

    //try to process incoming request via TCP
    auto clientEvent=tcpServer.ProcessRX();

    //process TCP client event
    tcpClientState|=clientEvent.type==ClientEventType::Connected;
    tcpClientState&=clientEvent.type!=ClientEventType::Disconnected;

    //process incoming data from UDP, with respect to TCP client event
    clientEvent=udpServer.ProcessRX(clientEvent);

    if(clientEvent.type==ClientEventType::NewRequest)
    {
        //process incoming request
        for(int i=0;i<UART_COUNT;++i)
        {
            auto request=Request::Map(i,rxBuff);
            uartPollTimes[i]=uartWorker[i].ProcessRequest(request);
        }
        //update global uart polling interval
        pollTimer.SetInterval(GetMinPollTime());
    }
    else if(clientEvent.type==ClientEventType::Disconnected)
        pollTimer.SetInterval(IDLE_POLL_INTERVAL_US);

    //process other tasks of UART worker -> finish running reset, write data from ring-buffer to uart
    for(int i=0;i<UART_COUNT;++i)
        uartWorker[i].ProcessRX();

    //if poll interval has passed, read available data from UART and send it to the client via UDP or TCP
    if(pollTimer.Update())
    {
        pollTimer.Reset(false);
        for(int i=0;i<UART_COUNT;++i)
        {
            //poll UART ports for incoming data
            auto response=uartWorker[i].ProcessTX();
            Response::Write(response,i,txBuff);
        }
        //if tcpClientConnected, try to send data via UDP first, and via TCP if send via UDP is not possible;
        !tcpClientState||udpServer.ProcessTX()||tcpServer.ProcessTX();
    }
}
