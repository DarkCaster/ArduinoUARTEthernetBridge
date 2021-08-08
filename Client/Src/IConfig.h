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
        virtual int GetIncomingDataBufferSec() const = 0;

        virtual int GetTCPBuffSz() const = 0;
        virtual int GetLingerSec() const = 0;
        virtual int GetMaxCTimeSec() const = 0;

        virtual int GetPortCount() const = 0;
        virtual int GetPackageMetaSz() const = 0;
        virtual int GetPackageSz() const = 0;
        virtual int GetPortBuffOffset(int portIndex) const = 0;
        virtual int GetNetworkPayloadSz() const = 0;
        virtual int GetRingBuffSegCount() const = 0;
        virtual int GetIdleTimerInterval() const = 0;
        virtual std::string GetRemoteAddr() const = 0;
        virtual uint16_t GetTCPPort() const = 0;
        virtual bool GetUDPEnabled() const = 0;
};

#endif
