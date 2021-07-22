#ifndef ICONFIG_H
#define ICONFIG_H

#include <sys/time.h>
#include <vector>
#include <tuple>
#include <cstdint>
#include <string>

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

        virtual std::string GetRemoteAddr() const = 0;
        virtual uint16_t GetTCPPort() const = 0;
        virtual uint16_t GetUDPPort() const = 0;
};

#endif
