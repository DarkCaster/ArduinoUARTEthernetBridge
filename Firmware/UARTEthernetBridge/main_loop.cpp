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

#define CMD_BUFFER_SIZE (1+64)

#define PORTS_BUFFER_SIZE1 REQ_DATA_LEN
#define PORTS_BUFFER_SIZE2 ANS_DATA_LEN
#if PORTS_BUFFER_SIZE1 > PORTS_BUFFER_SIZE2
#define PORTS_BUFFER_SIZE PORTS_BUFFER_SIZE1
#else
#define PORTS_BUFFER_SIZE PORTS_BUFFER_SIZE2
#endif

#if PORTS_BUFFER_SIZE > CMD_BUFFER_SIZE
#define NET_BUFFER_SIZE PORTS_BUFFER_SIZE
#else
#define NET_BUFFER_SIZE CMD_BUFFER_SIZE
#endif

static uint8_t macaddr[] = ENC28J60_MACADDR;

//helper classes
static UARTHelper uartHelpers[UART_COUNT];
static WatchdogAVR watchdog;
static UIPServer server(NET_PORT);

//stuff for reading data
static uint8_t reqBuff[NET_BUFFER_SIZE];
static uint8_t reqLeft = 0;
#define READER_CLEANUP() (__extension__({reqBuff[0]=REQ_NONE;reqLeft=0;}))

//stuff for remote client state
static UIPClient remote;
static int wdInterval = DEFAULT_WD_INTERVAL;
static bool isConnected=false;

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

    //setup UART
    HardwareSerial *extUARTs[] = UART_DEFS;
    uint8_t extUARTPins[] = UART_RX_PINS;
    for(int i=0;i<UART_COUNT;++i)
        uartHelpers[i].Setup(extUARTs[i],extUARTPins[i]);

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

    STATUS(); LOG(F("Server start"));
    server.begin();
    BLINK(10,0,1);
    STATUS(); LOG(F("Init complete!"));
}

void connect()
{
    //check link state
    if(UIPEthernet.linkStatus()!=LinkON)
    {
        STATUS(); LOG(F("Link disconnected! Rebooting"));
        BLINK(500,500,3);
        watchdog.SystemReset();
    }

    //trying to accept new client connection
    remote = server.accept();
    if (remote)
    {
        STATUS(); LOG(F("Client connected"));
        READER_CLEANUP();
        isConnected=true;
        //enable watchdog on client connection to default value
        watchdog.Enable(wdInterval);
    }
}

uint8_t read_fail()
{
    remote.stop();
    isConnected=false;
    return REQ_EOF;
}

uint8_t read_command()
{
    auto avail=remote.available();
    if(avail<1)
    {
        if(!remote.connected())
            return read_fail();
        return REQ_NONE;
    }

    if(avail>0 && reqBuff[0]==REQ_NONE)
    {
        if(remote.read(reqBuff,1)<1)
            return read_fail();
        switch (reqBuff[0])
        {
            case REQ_CONNECT:
                reqLeft=REQ_CONNECT_LEN-1;
                break;
            case REQ_DISCONNECT:
                reqLeft=REQ_DISCONNECT_LEN-1;
                break;
            case REQ_DATA:
                reqLeft=REQ_DATA_LEN-1;
                break;
            case REQ_PING:
                reqLeft=REQ_PING_LEN-1;
                break;
            case REQ_WD:
                reqLeft=REQ_WD_LEN-1;
                break;
            default:
                return read_fail();
        }
    }

    if(reqLeft>0)
    {
        //TODO: try to read as much as possible
    }

    if(reqLeft<1)
    {
        auto result=reqBuff[0];
        READER_CLEANUP();
        return result;
    }

    return REQ_NONE;
}

#define GET_UART_COLLECT_TIME(speed, bsz) (((uint)1000000000/(uint)(speed/8)*(uint)bsz)/(uint)1000000)

void loop()
{
    //no client connected
    if(!isConnected)
    {
        connect();
        return;
    }

    //client connected, try to read the command
    auto cmd=read_command();
    if(cmd==REQ_EOF) //not attempting to do anything on client disconnect
        return;

    //TODO: parse completed request

    //    -> reading data from enabled UARTs
    //    -> rearm watchdog on incoming data

    //    -> send it to client
    //    -> read incoming data, decode package type:
    //    -> perform uart setup, buffer setup, watchdog params setup
    //    -> perform client disconnect -> diactivate watchdog on proper disconnect
    //    -> write incoming data to designated uart port
    //    -> force disconnect on error
}
