#ifndef WATCHDOG_AVR_H
#define WATCHDOG_AVR_H

#include <Arduino.h>
#include "watchdog.h"

class WatchdogAVR final : Watchdog
{
    private:
        uint8_t * const srBootSig;
        const bool srBootFlag;
        volatile bool isEnabled;
	public:
        WatchdogAVR();
        bool IsSystemResetBoot() final;
        bool IsEnabled() final;
        void Enable(uint16_t maxDelayMS) final;
		void Disable() final;
        void Ping() final;
        void SystemReset() final;
};

#endif
