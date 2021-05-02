#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <Arduino.h>

class Watchdog
{
	public:
        virtual bool IsEnabled() = 0;
        virtual void Enable(uint16_t maxDelayMS) = 0;
		virtual void Disable() = 0;
		virtual void Ping() = 0;
        virtual void SystemReset() = 0;
};

#endif
