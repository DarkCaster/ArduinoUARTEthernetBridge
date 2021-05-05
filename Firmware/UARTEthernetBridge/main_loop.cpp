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

#define PORTS_BUFFER_SIZE (1+UART_COUNT*(1+UART_BUFFER_SIZE))
#define CMD_BUFFER_SIZE (1+64)

#if PORTS_BUFFER_SIZE > CMD_BUFFER_SIZE
#define NET_BUFFER_SIZE PORTS_BUFFER_SIZE
#else
#define NET_BUFFER_SIZE CMD_BUFFER_SIZE
#endif

#define CMD_NONE 0x00
#define CMD_CONNECT 0x01
#define CMD_DISCONNECT 0x02
#define CMD_DATA 0x03
#define CMD_PING 0x04
#define CMD_EOF 0xFF

static uint8_t macaddr[] = ENC28J60_MACADDR;
static bool isConnected=false;
static UIPClient remote;
static uint8_t rxBuff[NET_BUFFER_SIZE];
static uint8_t txBuff[NET_BUFFER_SIZE];
static int wdInterval = 1000;

static uint8_t cmdPending = CMD_NONE;
static uint8_t cmdLeft = 0;

//helper classes
static UARTHelper uartHelpers[UART_COUNT];
static WatchdogAVR watchdog;
static UIPServer server(NET_PORT);



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
        cmdPending=CMD_NONE;
        cmdLeft=0;
        isConnected=true;
        watchdog.Enable(wdInterval); //enable watchdog on client connection to default value
    }
}

uint8_t read_command()
{
    if(!remote.connected())
    {
        remote.stop();
        isConnected=false;
        return CMD_EOF;
    }

    auto avail=remote.available();
    if(cmdPending==CMD_NONE && avail>0)
    {

    }

    return CMD_NONE;
}

#define GET_UART_COLLECT_TIME(speed, bsz) (((uint)1000000000/(uint)(speed/8)*(uint)bsz)/(uint)1000000)

void loop()
{
    /*if(isConnected)
    {
        auto avail=remote.available();
        if(avail>NET_BUFFER_SIZE)
            avail=NET_BUFFER_SIZE;
        auto read=remote.read(buff,avail);
        if(read>0)
        {
            remote.write(buff,NET_BUFFER_SIZE);
        }

        if(!remote.connected())
        {
            remote.stop();
            isConnected=false;
        }
    }*/

    //no client connected
    if(!isConnected)
    {
        connect();
        return;
    }

    //client connected, try to read the command
    if(read_command()==CMD_NONE)
        return;

    //    -> reading data from enabled UARTs
    //    -> rearm watchdog on incoming data

    //    -> send it to client
    //    -> read incoming data, decode package type:
    //    -> perform uart setup, buffer setup, watchdog params setup
    //    -> perform client disconnect -> diactivate watchdog on proper disconnect
    //    -> write incoming data to designated uart port
    //    -> force disconnect on error
}
