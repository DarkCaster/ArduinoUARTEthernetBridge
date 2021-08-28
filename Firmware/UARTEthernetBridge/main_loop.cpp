#include <Arduino.h>
#include <Ethernet.h>

#include "configuration.h"
#include "main_loop.h"
#include "watchdog.h"
#include "watchdog_AVR.h"
#include "tcpserver.h"
#include "udpserver.h"
#include "alarmtimer.h"
#include "intervaltimer.h"
#include "resethelper.h"
#include "uartworker.h"
#include "command.h"

//receive- and send- buffers
static uint8_t rxBuff[PACKAGE_SIZE];
static uint8_t txBuff[PACKAGE_SIZE];

//helper classes
static WatchdogAVR watchdog;
static IntervalTimer pollTimer;
static AlarmTimer alarmTimer;
static TCPServer tcpServer(alarmTimer,rxBuff,txBuff,PACKAGE_SIZE,META_SZ,TCP_PORT);
static UDPServer udpServer(alarmTimer,rxBuff,txBuff,PACKAGE_SIZE,META_SZ);
static ResetHelper rstHelper[UART_COUNT];
static UARTWorker uartWorker[UART_COUNT];

//current client state
static bool tcpClientState;
static bool pollIntervalSetPending;
static ClientEvent clientEvent;
#if IO_AGGREGATE_MULTIPLIER > 1
static uint8_t segmentCounter;
#endif

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
    for(uint8_t i=0;i<UART_COUNT;++i)
    {
        pinMode(extUARTPins[i],INPUT_PULLUP);
        rstHelper[i].Setup(extRSTPins[i]);
        uartWorker[i].Setup(&(rstHelper[i]),extUARTs[i],rxBuff+META_SZ+META_CRC_SZ+DATA_PAYLOAD_SIZE*i,txBuff+META_SZ+META_CRC_SZ+DATA_PAYLOAD_SIZE*i);
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
        for(uint8_t i=0;i<UART_COUNT;++i)
            rstHelper[i].StartReset(RESET_TIME_MS);
        bool rstComplete=true;
        do
        {
            rstComplete=true;
            for(uint8_t i=0;i<UART_COUNT;++i)
                rstComplete&=rstHelper[i].ResetComplete();
        } while(!rstComplete);
        //blink LED pin indicating hardware setup is complete
        blink(50,50,10);
    }
    else
        blink(10,10,1);

    //initialize network, reset if no network cable detected
    Ethernet.init(PIN_SPI_ENC28J60_CS);
    uint8_t macaddr[] = ENC28J60_MACADDR;
    if (Ethernet.begin(macaddr) == 0)
    {
        if (Ethernet.hardwareStatus() == EthernetNoHardware)
            blink(50,950,6);
        else if (Ethernet.linkStatus() == LinkOFF)
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

    //setup timers
#if IO_AGGREGATE_MULTIPLIER > 1
    segmentCounter=0;
#endif
    pollIntervalSetPending=false;
    alarmTimer.SetAlarmDelay(DEFAULT_ALARM_INTERVAL_MS);
    pollTimer.SetInterval(UART_POLL_INTERVAL_US_DEFAULT);
    pollTimer.Reset();
}

static bool check_link_state()
{
    //check link state
    if(Ethernet.linkStatus()!=LinkON)
    {
        blink(500,500,6);
        watchdog.SystemReset();
    }
    //add small delay to speed down SPI communication with ethernet adapter while not connected
    delay(25);
    return true;
}

inline Request MapRequest(const int portIndex, const uint8_t * const rawBuffer)
{
    const auto offset=PKG_HDR_SZ+portIndex*CMD_HDR_SIZE;
    return Request{static_cast<ReqType>(*(rawBuffer+offset)),*(rawBuffer+offset+1),*(rawBuffer+offset+2)};
}

inline void WriteResponse(const Response &source, const int portIndex, uint8_t * const rawBuffer)
{
    const auto offset=PKG_HDR_SZ+portIndex*CMD_HDR_SIZE;
    *(rawBuffer+offset)=static_cast<uint8_t>(source.type);
    *(rawBuffer+offset+1)=source.arg;
    *(rawBuffer+offset+2)=source.plSz;
}

void loop()
{
    //if client is not connected, check the link state, and reboot on link-failure
    tcpClientState || check_link_state();

    //try to process incoming request via TCP
    clientEvent=tcpServer.ProcessRX();

    //process TCP client event
    if (clientEvent.type==ClientEventType::Connected)
    {
        pollTimer.SetInterval(UART_POLL_INTERVAL_US_DEFAULT);
        tcpClientState=true;
        pollIntervalSetPending=true;
#if IO_AGGREGATE_MULTIPLIER > 1
        segmentCounter=0;
#endif
    }
    else if(clientEvent.type==ClientEventType::Disconnected)
    {
        pollTimer.SetInterval(UART_POLL_INTERVAL_US_DEFAULT);
        tcpClientState=false;
        pollIntervalSetPending=false;
    }

    //process incoming data from UDP, with respect to TCP client event
    clientEvent=udpServer.ProcessRX(clientEvent);

    //process incoming request
    if(clientEvent.type==ClientEventType::NewRequest)
    {
        for(uint8_t i=0;i<UART_COUNT;++i)
            uartWorker[i].ProcessRequest(MapRequest(i,rxBuff));
        //save new counter to the txbuff
        txBuff[PKG_CNT_OFFSET]=rxBuff[PKG_CNT_OFFSET];
        txBuff[PKG_CNT_OFFSET+1]=rxBuff[PKG_CNT_OFFSET+1];
        txBuff[PKG_CNT_OFFSET+2]=rxBuff[PKG_CNT_OFFSET+2];
        txBuff[PKG_CNT_OFFSET+3]=rxBuff[PKG_CNT_OFFSET+3];
        //or save new poll interval
        if(pollIntervalSetPending)
        {
            pollIntervalSetPending=false;
            txBuff[PKG_CNT_OFFSET]=txBuff[PKG_CNT_OFFSET+1]=txBuff[PKG_CNT_OFFSET+2]=txBuff[PKG_CNT_OFFSET+3]=0;
            auto interval=(static_cast<unsigned long>(rxBuff[PKG_CNT_OFFSET]))|(static_cast<unsigned long>(rxBuff[PKG_CNT_OFFSET+1])<<8)|
                    (static_cast<unsigned long>(rxBuff[PKG_CNT_OFFSET+2])<<16)|(static_cast<unsigned long>(rxBuff[PKG_CNT_OFFSET+2])<<24);
            interval/=IO_AGGREGATE_MULTIPLIER;
            if(interval>0)
                pollTimer.SetInterval(interval);
        }
    }

    //process other tasks of UART worker -> finish running reset, write data from ring-buffer to uart
    for(uint8_t i=0;i<UART_COUNT;++i)
        uartWorker[i].ProcessRX();

    //if poll interval has passed, read available data from UART and send it to the client via UDP or TCP
    if(pollTimer.Update())
    {
        pollTimer.Next();
#if IO_AGGREGATE_MULTIPLIER > 1
        segmentCounter++;
        for(uint8_t i=0;i<UART_COUNT;++i)
            uartWorker[i].FillTXBuff(segmentCounter==1);
        if(segmentCounter<IO_AGGREGATE_MULTIPLIER)
            return;
        for(uint8_t i=0;i<UART_COUNT;++i)
            WriteResponse(uartWorker[i].ProcessTX(),i,txBuff);
        segmentCounter=0;
#else
        for(uint8_t i=0;i<UART_COUNT;++i)
        {
            uartWorker[i].FillTXBuff(true);
            WriteResponse(uartWorker[i].ProcessTX(),i,txBuff);
        }
#endif
        //if tcpClientConnected, try to send data via UDP first, and via TCP if send via UDP is not possible;
        !tcpClientState||udpServer.ProcessTX()||tcpServer.ProcessTX();
    }
}
