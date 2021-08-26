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
        virtual std::string GetRemoteAddr() const = 0; //-ra
        virtual uint16_t GetTCPPort() const = 0; //-tp
        virtual int GetPortCount() const = 0; //-pc
        virtual int GetPortPayloadSz() const = 0; //-pls
        virtual int GetRemoteRingBuffSize() const = 0; //-rbs
        virtual int GetLocalRingBufferSec() const = 0; //TODO
        virtual bool GetUDPEnabled() const = 0; //-udp
        virtual int GetRemotePollIntervalUS() const = 0;//-ptr

        virtual int GetServiceIntervalMS() const = 0; //service param, not configurable for now
        virtual timeval GetServiceIntervalTV() const = 0; //service param, not configurable for now
        virtual int GetTCPBuffSz() const = 0; //service param, not configurable for now
        virtual int GetLingerSec() const = 0; //service param, not configurable for now

        virtual int GetNetPackageMetaSz() const = 0; //auto-calculated
        virtual int GetNetPackageSz() const = 0; //auto-calculated
        virtual int GetPortBuffOffset(int portIndex) const = 0; //auto-calculated
};

#endif
