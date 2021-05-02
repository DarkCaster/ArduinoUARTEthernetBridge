#include <Arduino.h>
#include <UIPEthernet.h>

#include "debug.h"
#include "configuration.h"
#include "main_loop.h"
#include "watchdog.h"
#include "watchdog_AVR.h"
#include "settings_manager.h"

static uint8_t macaddr[] = ENC28J60_MACADDR;
static bool connected = false;

//create some helpers
//static EEPROMSettingsManager settingsManager(EEPROM_SETTINGS_ADDR, EEPROM_SETTINGS_LEN, cipher, encKey, encTweak);
static WatchdogAVR watchdog;
EthernetServer server(TCP_PORT);

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
    Ethernet.init(PIN_SPI_ENC28J60_CS);
    if (Ethernet.begin(macaddr) == 0)
    {
        if (Ethernet.hardwareStatus() == EthernetNoHardware)
            FAIL(250,250);
        if (Ethernet.linkStatus() == LinkOFF)
        {
            BLINK(500,500,10);
            watchdog.SystemReset();
        }
    }

    STATUS(); LOG(F("Server start"));
    server.begin();
    BLINK(10,0,1);
    STATUS(); LOG(F("Init complete!"));
}

void loop()
{
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
    //    -> perform client disconnect
    //    -> write incoming data to designated uart port
    //    -> force disconnect on error

    delay(60000);
    watchdog.SystemReset();
}
