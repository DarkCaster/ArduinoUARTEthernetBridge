#include <Arduino.h>
#include <UIPEthernet.h>
#include <UIPUdp.h>

#include "debug.h"
#include "configuration.h"
#include "main_loop.h"
#include "watchdog.h"
#include "watchdog_AVR.h"

#define PORTS_BUFFER_SIZE (1+UART_COUNT*(1+UART_BUFFER_SIZE))
#define CMD_BUFFER_SIZE (1+64)

#if PORTS_BUFFER_SIZE > CMD_BUFFER_SIZE
#define NET_BUFFER_SIZE PORTS_BUFFER_SIZE
#else
#define NET_BUFFER_SIZE CMD_BUFFER_SIZE
#endif

static uint8_t macaddr[] = ENC28J60_MACADDR;
static IPAddress remoteIP(0,0,0,0);
static uint16_t remotePort=0;

//create some helpers
static WatchdogAVR watchdog;
static UIPUDP server;

static uint8_t buff[NET_BUFFER_SIZE];

void setup()
{
    SETUP_DEBUG_SERIAL();
    STATUS(); LOG(F("Startup"));

    //TODO: read settings
    //settingsManager.Init();

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

    //TODO: setup UART

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
            BLINK(500,500,10);
            watchdog.SystemReset();
        }
    }

    STATUS(); LOG(F("Server start"));
    server.begin(UDP_PORT);
    BLINK(10,0,1);
    STATUS(); LOG(F("Init complete!"));
}

void loop()
{
    // if there's data available, read a packet
    /*int packetSize = Udp.parsePacket();
    if (packetSize) {
        //Serial.print("Received packet of size ");
        //Serial.println(packetSize);
        //Serial.print("From ");
        //IPAddress remote = Udp.remoteIP();
        for (int i=0; i < 4; i++) {
            Serial.print(remote[i], DEC);
            if (i < 3) {
                Serial.print(".");
            }
        }
        //Serial.print(", port ");
        //Serial.println(Udp.remotePort());

        // read the packet into packetBufffer
        auto dr=Udp.read(buff, 100);
        //Serial.println("Contents:");
        //Serial.println(packetBuffer);

        // send a reply to the IP address and port that sent us the packet we received
        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
        Udp.write(buff,dr);
        Udp.endPacket();
    }*/
















    //possible states and actions performed

    //no client connected
    //    -> check for link state
    //    -> trying to accept new client connection
    //    -> enable watchdog on client connection to default value



    //client connected
    //    -> reading data from enabled UARTs
    //    -> rearm watchdog on incoming data
    //    -> send it to client
    //    -> read incoming data, decode package type:
    //    -> perform uart setup, buffer setup, watchdog params setup
    //    -> perform client disconnect -> diactivate watchdog on proper disconnect
    //    -> write incoming data to designated uart port
    //    -> force disconnect on error
}
