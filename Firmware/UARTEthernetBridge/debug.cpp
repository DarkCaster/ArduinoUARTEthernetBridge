#include "debug.h"
#include "configuration.h"

#ifdef DEBUG

#if __AVR__
#include <util/delay.h>
#endif

// https://playground.arduino.cc/Code/AvailableMemory

extern unsigned int __heap_start;
extern void *__brkval;

struct Freelist
{
        size_t sz;
        struct Freelist *nx;
};

extern struct Freelist *__flp;

uint32_t get_free_list_size()
{
    struct Freelist* current;
    uint32_t total = 0;
    for (current = __flp; current; current = current->nx)
    {
        total += 2; /* Add two bytes for the memory block's header  */
        total += static_cast<uint32_t>(current->sz);
    }
    return total;
}

uint32_t get_free_memory()
{
    uint32_t free_memory;
    if (__brkval == nullptr)
        free_memory = (reinterpret_cast<uint32_t>(&free_memory)) - (reinterpret_cast<uint32_t>(&__heap_start));
    else
    {
        free_memory = (reinterpret_cast<uint32_t>(&free_memory)) - (reinterpret_cast<uint32_t>(__brkval));
        free_memory += get_free_list_size();
    }
    return free_memory;
}

void STATUS(const bool nl)
{
    auto milliseconds=millis();
    auto seconds=milliseconds/1000UL;
    DEBUG_SERIAL_PORT.print(F("["));
    DEBUG_SERIAL_PORT.print(seconds);
    DEBUG_SERIAL_PORT.print(F("s "));
    DEBUG_SERIAL_PORT.print(milliseconds%(seconds*1000UL));
    DEBUG_SERIAL_PORT.print(F("ms "));
    DEBUG_SERIAL_PORT.print(get_free_memory());
    DEBUG_SERIAL_PORT.print(F("mem] "));
    if(nl)
        DEBUG_SERIAL_PORT.print(F("\n"));
}

void LOG(const __FlashStringHelper* message, const bool nl)
{
    DEBUG_SERIAL_PORT.print(message);
    if(nl)
        DEBUG_SERIAL_PORT.print(F("\n"));
}

void LOG(const int32_t intNumber, const bool nl)
{
    DEBUG_SERIAL_PORT.print(intNumber);
    if(nl)
        DEBUG_SERIAL_PORT.print(F("\n"));
}

void LOG(const uint32_t intNumber, const bool nl)
{
    DEBUG_SERIAL_PORT.print(intNumber);
    if(nl)
        DEBUG_SERIAL_PORT.print(F("\n"));
}

#endif

void BLINK(uint16_t blinkTime, uint16_t pauseTime, uint8_t count)
{
    pinMode(DEBUG_LED, OUTPUT);
    blinkTime/=10;
    pauseTime/=10;

    while(true)
    {
        digitalWrite(DEBUG_LED,HIGH);
        uint16_t cnt=0;
        for(cnt=0; cnt<blinkTime; ++cnt)
            _delay_ms(10);
        if(pauseTime>0)
        {
            digitalWrite(DEBUG_LED,LOW);
            for(cnt=0; cnt<pauseTime; ++cnt)
                _delay_ms(10);
        }
        if(count==1)
            return;
        if(count>0)
            count--;
    }
}

void FAIL(uint16_t blinkTime, uint16_t pauseTime)
{
    BLINK(blinkTime,pauseTime,0);
}

//If SERIAL_RX_PIN defined, define macro to enable pullup on serial rx-pin
#ifdef DEBUG_SERIAL_RX_PIN
#define RX_PIN_PREP() (__extension__({pinMode(DEBUG_SERIAL_RX_PIN,INPUT_PULLUP);}))
#else
#define RX_PIN_PREP() (__extension__({}))
#endif

void SETUP_DEBUG_SERIAL()
{
    //setup serial port for debugging
    DEBUG_SERIAL_PORT.begin(DEBUG_SERIAL_PORT_SPEED);
    DEBUG_SERIAL_PORT.setTimeout(100);
    RX_PIN_PREP(); // enable pullup on serial RX-pin
}
