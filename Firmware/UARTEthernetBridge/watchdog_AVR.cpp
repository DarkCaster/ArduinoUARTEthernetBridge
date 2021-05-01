#include <avr/wdt.h>
#include "watchdog_AVR.h"

WatchdogAVR::WatchdogAVR(uint16_t defaultDelayMS)
{
	SetDelay(defaultDelayMS);
}

void WatchdogAVR::SetDelay(uint16_t maxDelayMS)
{
	if(maxDelayMS>1000)
		delay=WDTO_2S;
	else if(maxDelayMS>500)
		delay=WDTO_1S;
	else if(maxDelayMS>250)
		delay=WDTO_500MS;
	else if(maxDelayMS>120)
		delay=WDTO_250MS;
	else if(maxDelayMS>60)
		delay=WDTO_120MS;
	else if(maxDelayMS>30)
		delay=WDTO_60MS;
	else if(maxDelayMS>15)
		delay=WDTO_30MS;
	else
		delay=WDTO_15MS;
}

void WatchdogAVR::Enable()
{
	wdt_enable(delay);
}

void WatchdogAVR::Disable()
{
	wdt_disable();
}

void WatchdogAVR::Ping()
{
	wdt_reset();
}
