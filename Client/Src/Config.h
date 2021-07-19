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

#define SERIAL_5N1 0x00
#define SERIAL_6N1 0x02
#define SERIAL_7N1 0x04
#define SERIAL_8N1 0x06
#define SERIAL_5N2 0x08
#define SERIAL_6N2 0x0A
#define SERIAL_7N2 0x0C
#define SERIAL_8N2 0x0E
#define SERIAL_5E1 0x20
#define SERIAL_6E1 0x22
#define SERIAL_7E1 0x24
#define SERIAL_8E1 0x26
#define SERIAL_5E2 0x28
#define SERIAL_6E2 0x2A
#define SERIAL_7E2 0x2C
#define SERIAL_8E2 0x2E
#define SERIAL_5O1 0x30
#define SERIAL_6O1 0x32
#define SERIAL_7O1 0x34
#define SERIAL_8O1 0x36
#define SERIAL_5O2 0x38
#define SERIAL_6O2 0x3A
#define SERIAL_7O2 0x3C
#define SERIAL_8O2 0x3E

class Config final : public IConfig
{
    private:
        int socketTimeout;
        int serviceInterval;
        int TCPBuffSz;
        int linger;
        int maxCtSec;
        int uartBuffSize=UART_BUFFER_SIZE;
        int ringBuffSegCount;
    public:
        //void AddListenAddr(const IPEndpoint &endpoint);
        void SetServiceIntervalMS(int intervalMS);
        void SetTCPBuffSz(int sz);
        void SetLingerSec(int sz);
        void SetMaxCTimeSec(int time);
        void SetUARTBuffSz(int sz);
        void SetRingBuffSegCount(int cnt);

        //from IConfig
        int GetServiceIntervalMS() const final;
        timeval GetServiceIntervalTV() const final;
        int GetTCPBuffSz() const final;
        int GetLingerSec() const final;
        int GetMaxCTimeSec() const final;
        int GetUARTBuffSz() const final;
        int GetRingBuffSegCount() const final;
};

#endif //CONFIG_H
