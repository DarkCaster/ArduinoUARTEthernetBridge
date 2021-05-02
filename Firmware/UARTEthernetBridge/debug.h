#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>

#define D_INT(n) ((int32_t)(n))
#define D_UINT(n) ((uint32_t)(n))

#ifdef DEBUG
void LOG(const __FlashStringHelper* message, const bool nl=true);
void LOG(const int32_t intNumber, const bool nl=true);
void LOG(const uint32_t intNumber, const bool nl=true);
void STATUS(const bool nl=false);
#else
#define LOG(...) ({})
#define STATUS(...) ({})
#endif

void FAIL(uint16_t blinkTime, uint16_t pauseTime);
void BLINK(uint16_t blinkTime, uint16_t pauseTime, uint8_t count);

#endif //DEBUG_H
