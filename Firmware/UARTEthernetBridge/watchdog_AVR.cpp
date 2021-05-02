#include <avr/wdt.h>
#include "watchdog_AVR.h"

WatchdogAVR::WatchdogAVR()
{
    Disable();
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
	wdt_disable();
    isEnabled=false;
}

void WatchdogAVR::Ping()
{
    if(isEnabled)
        wdt_reset();
}

void WatchdogAVR::SystemReset()
{
    wdt_disable();
    isEnabled=false; //prohibit interrupts to perform pings on watchdog while we are awaiting for reset
    wdt_enable(1);
    while(true){}; //there is no return, await for reset
}
