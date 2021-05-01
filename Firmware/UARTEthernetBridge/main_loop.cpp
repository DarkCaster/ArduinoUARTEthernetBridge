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

#ifdef LED_SYNC
#define SYNC_OK() (__extension__({digitalWrite(LED_SYNC, HIGH);}))
#define SYNC_ERR() (__extension__({digitalWrite(LED_SYNC, LOW);}))
#define SYNC_LED_PREP() (__extension__({pinMode(LED_SYNC, OUTPUT);}))
#else
#define SYNC_OK() (__extension__({}))
#define SYNC_ERR() (__extension__({}))
#define SYNC_LED_PREP() (__extension__({}))
#endif

//create main logic blocks and perform "poor man's" dependency injection
//static EEPROMSettingsManager settingsManager(EEPROM_SETTINGS_ADDR, EEPROM_SETTINGS_LEN, cipher, encKey, encTweak);
static WatchdogAVR watchdog(SYNC_WATCHDOG_TIMEOUT);

//stuff needed for requests, commands and responses parsing
//static uint8_t commandBuffer[COMMAND_BUFFER_SIZE];
//static uint8_t responseBuffer[RESPONSE_BUFFER_SIZE];
//static CMDRSP_BUFF_TYPE cmdBuffPos=0;
//static CMDRSP_BUFF_TYPE rspBuffPos=0;
//static CMDRSP_BUFF_TYPE rspSize=0;
//static uint8_t cmdType=0;
//static uint8_t rspType=0;

static unsigned long lastTime=0;

void setup()
{
	//read settings
    //settingsManager.Init();

	//deactivate watchdog
	watchdog.Disable();
	SYNC_LED_PREP();
	SYNC_ERR();

	SERIAL_PORT.begin(SERIAL_PORT_SPEED);
    SERIAL_PORT.setTimeout(1000);
	RX_PIN_PREP(); // enable pullup on serial RX-pin

    // initialize digital pin LED_BUILTIN as an output.
    pinMode(LED_BUILTIN, OUTPUT);

	//reset last-time
	lastTime=millis();
	STATUS();
	LOG(F("Setup complete!"));
}

void loop()
{
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(10);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(1000);
}
