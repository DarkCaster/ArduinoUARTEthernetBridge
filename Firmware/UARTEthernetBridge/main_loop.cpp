#include <Arduino.h>
#include "debug.h"
#include "configuration.h"
#include "main_loop.h"
#include "watchdog.h"
#include "watchdog_AVR.h"
#include "settings_manager.h"

//If SERIAL_RX_PIN defined, define macro to enable pullup on serial rx-pin
//We need this in order to prevent false incoming connection events when device enabled and not connected to PC
#ifdef SERIAL_RX_PIN
#define RX_PIN_PREP() (__extension__({pinMode(SERIAL_RX_PIN,INPUT_PULLUP);}))
#else
#define RX_PIN_PREP() (__extension__({}))
#endif

//create main logic blocks and perform "poor man's" dependency injection
//static EEPROMSettingsManager settingsManager(EEPROM_SETTINGS_ADDR, EEPROM_SETTINGS_LEN, cipher, encKey, encTweak);
static WatchdogAVR watchdog;

void setup()
{
    //setup serial port for debugging
    SERIAL_PORT.begin(SERIAL_PORT_SPEED);
    SERIAL_PORT.setTimeout(1000);
    RX_PIN_PREP(); // enable pullup on serial RX-pin

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

    //blink LED pin indicating hardware setup is complete
    BLINK(1000,1000,3);

	//read settings
    //settingsManager.Init();

    //initialize network, reset if no network cable detected



	STATUS();
	LOG(F("Setup complete!"));
}

void loop()
{
    //delay(5000);
    watchdog.SystemReset();
}
