#ifndef ICONFIG_H
#define ICONFIG_H

#include "IPEndpoint.h"
#include <sys/time.h>
#include <vector>
#include <tuple>

class IConfig
{
    public:
        virtual int GetServiceIntervalMS() const = 0;
        virtual timeval GetServiceIntervalTV() const = 0;
        virtual int GetTCPBuffSz() const = 0;
        virtual int GetLingerSec() const = 0;
        virtual int GetMaxCTimeSec() const = 0;
        virtual int GetUARTBuffSz() const = 0;
        virtual int GetRingBuffSegCount() const = 0;
};

#endif
