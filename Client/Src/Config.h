#ifndef CONFIG_H
#define CONFIG_H

#include "IConfig.h"
#include "IPEndpoint.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <sys/time.h>

//some default params

#define UART_BUFFER_SIZE 64
#define UART_COUNT 3
#define UART_MAX_POLL_INTERVAL_MS 5

#define NET_PORTS { 50000, 50001, 50002 }
#define NET_NAME "ENC28J65E366"
#define NET_DOMAIN "lan"

#define BACK_BUFFER_SEGMENTS 5

class Config final : public IConfig
{
    private:
        int socketTimeout;
        int serviceInterval;
        int TCPBuffSz;
        int linger;
        int maxCtSec;
    public:
        //void AddListenAddr(const IPEndpoint &endpoint);
        void SetServiceIntervalMS(int intervalMS);
        void SetTCPBuffSz(int sz);
        void SetLingerSec(int sz);
        void SetMaxCTimeSec(int time);

        //from IConfig
        int GetServiceIntervalMS() const final;
        timeval GetServiceIntervalTV() const final;
        int GetTCPBuffSz() const final;
        int GetLingerSec() const final;
        int GetMaxCTimeSec() const final;
};

#endif //CONFIG_H
