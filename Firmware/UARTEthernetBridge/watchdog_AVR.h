#ifndef WATCHDOG_AVR_H
#define WATCHDOG_AVR_H

#include <Arduino.h>
#include "watchdog.h"

class WatchdogAVR final : Watchdog
{
	private:
		uint8_t delay=0;
	public:
		WatchdogAVR(uint16_t defaultDelayMS=2000);
		void SetDelay(uint16_t maxDelayMS) final;
		void Enable() final;
		void Disable() final;
		void Ping() final;
};

#endif
