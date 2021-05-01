#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>
#include "settings.h"

class SettingsManager
{
	public:
		Settings settings;
		virtual void Commit() = 0;
		virtual void Init() = 0;
};

#endif
