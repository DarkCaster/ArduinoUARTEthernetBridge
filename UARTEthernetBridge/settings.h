#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include "configuration.h"

#pragma pack(push, settings, 1)
struct Settings
{
		int32_t utcOffset;
};
#pragma pack(pop, settings)

#endif
