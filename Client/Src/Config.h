#ifndef CONFIG_H
#define CONFIG_H

#include "IConfig.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <sys/time.h>

//some hardcoded params
#define PKG_HDR_SZ 6
#define PKG_CNT_OFFSET 2
#define CMD_HDR_SIZE 3

#define NET_NAME "ENC28J65E366"
#define NET_DOMAIN "lan"

class Config final : public IConfig
{
    private:
        int serviceInterval;
        int localRingBuffSec=2;
        int TCPBuffSz;
        int linger;
        int portPLSize;
        int portCount;
        int remoteRingBuffSize;
        bool enableUDP;
        uint16_t tcpPort;
        std::string remoteAddr;
    public:
        void SetRemoteAddr(const std::string &addr);
        void SetTCPPort(uint16_t port);
        void SetPortCount(int portCount);
        void SetPortPayloadSize(int sz);
        void SetRemoteRingBuffSize(int size);
        void SetLocalRingBuffSec(int size);
        void SetUDPEnabled(bool enableUDP);
        void SetServiceIntervalMS(int intervalMS);
        void SetTCPBuffSz(int sz);
        void SetLingerSec(int sz);
        //from IConfig
        std::string GetRemoteAddr() const final;
        uint16_t GetTCPPort() const final;
        int GetPortCount() const final;
        int GetPortPayloadSz() const final;
        int GetRemoteRingBuffSize() const final;
        int GetLocalRingBufferSec() const final;
        bool GetUDPEnabled() const final;

        int GetServiceIntervalMS() const final;
        timeval GetServiceIntervalTV() const final;
        int GetTCPBuffSz() const final;
        int GetLingerSec() const final;

        int GetNetPackageMetaSz() const final;
        int GetNetPackageSz() const final;
        int GetPortBuffOffset(int portIndex) const final;
};

#endif //CONFIG_H
