#ifndef CONFIG_H
#define CONFIG_H

#include "IConfig.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <sys/time.h>

//some hardcoded params
#define UART_BUFFER_SIZE 64
#define IDLE_TIMER_INTERVAL_US 1000000

#define PKG_HDR_SZ 2
#define CMD_HDR_SIZE 3


#define NET_NAME "ENC28J65E366"
#define NET_DOMAIN "lan"

class Config final : public IConfig
{
    private:
        int socketTimeout;
        int serviceInterval;
        int incomingDataBufferSec=2;
        int TCPBuffSz;
        int linger;
        int maxCtSec;
        int uartBuffSize=UART_BUFFER_SIZE;
        int portCount;
        int ringBuffSegCount;
        int idleTimerInterval=IDLE_TIMER_INTERVAL_US;
        bool enableUDP;
        uint16_t tcpPort;
        std::string remoteAddr;

    public:
        //void AddListenAddr(const IPEndpoint &endpoint);
        void SetServiceIntervalMS(int intervalMS);
        void SetTCPBuffSz(int sz);
        void SetLingerSec(int sz);
        void SetMaxCTimeSec(int time);
        void SetPortCount(int portCount);
        void SetNetworkPayloadSz(int sz);
        void SetRingBuffSegCount(int cnt);
        void SetRemoteAddr(const std::string &addr);
        void SetUDPEnabled(bool enableUDP);
        void SetTCPPort(uint16_t port);

        //from IConfig
        int GetServiceIntervalMS() const final;
        timeval GetServiceIntervalTV() const final;
        int GetIncomingDataBufferSec() const final;
        int GetTCPBuffSz() const final;
        int GetLingerSec() const final;
        int GetMaxCTimeSec() const final;
        int GetPortCount() const final;
        int GetPackageMetaSz() const final;
        int GetPackageSz() const final;
        int GetPortBuffOffset(int portIndex) const final;
        int GetNetworkPayloadSz() const final;
        int GetRingBuffSegCount() const final;
        int GetIdleTimerInterval() const final;
        std::string GetRemoteAddr() const final;
        uint16_t GetTCPPort() const final;
        bool GetUDPEnabled() const final;
};

#endif //CONFIG_H
