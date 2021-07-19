#include <avr/wdt.h>
#include "watchdog_AVR.h"

const uint8_t rstSig[] PROGMEM = { 0xFE, 0xED, 0xBE, 0xEF };

// https://forum.arduino.cc/t/watchdog-reset-detection/351423/3
WatchdogAVR::WatchdogAVR():
    srBootSig(reinterpret_cast<uint8_t*>(malloc(sizeof(rstSig)))),
    srBootFlag(memcmp_P(srBootSig,rstSig,sizeof(rstSig))==0)
{
    Disable();
    //reset boot-signature to guarantee that we start from scratch on hardware or watchdog-timeout reset
    memset(srBootSig,0,sizeof(rstSig));
}

void WatchdogAVR::SystemReset()
{
    Disable();
    //set signature to detect software-initiated reboot
    memcpy_P(srBootSig,rstSig,sizeof(rstSig));
    //rearm watchdog and wait for reset
    wdt_enable(1);
    while(true){};
}

bool WatchdogAVR::IsSystemResetBoot()
{
    return srBootFlag;
}

bool WatchdogAVR::IsEnabled()
{
    return isEnabled;
}

static uint8_t GetDelayParam(uint16_t reqDelay)
{
    if(reqDelay>4000)
        return WDTO_8S;
    if(reqDelay>2000)
        return WDTO_4S;
    if(reqDelay>1000)
        return WDTO_2S;
    if(reqDelay>500)
        return WDTO_1S;
    if(reqDelay>250)
        return WDTO_500MS;
    if(reqDelay>120)
        return WDTO_250MS;
    if(reqDelay>60)
        return WDTO_120MS;
    if(reqDelay>30)
        return WDTO_60MS;
    if(reqDelay>15)
        return WDTO_30MS;
    else
        return WDTO_15MS;
}

void WatchdogAVR::Enable(uint16_t maxDelayMS)
{
    wdt_enable(GetDelayParam(maxDelayMS));
    isEnabled=true;
}

void WatchdogAVR::Disable()
{
    isEnabled=false;
    wdt_disable();
}

void WatchdogAVR::Ping()
{
    if(isEnabled)
        wdt_reset();
}
