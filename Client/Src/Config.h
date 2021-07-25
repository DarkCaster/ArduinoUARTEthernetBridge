#ifndef CONFIG_H
#define CONFIG_H

#include "IConfig.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <sys/time.h>

//some default params

#define UART_BUFFER_SIZE 64

#define PKG_HDR_SZ 2
#define CMD_HDR_SIZE 3
#define META_SZ(uart_count) (PKG_HDR_SZ+CMD_HDR_SIZE*uart_count)
#define META_CRC_SZ 1
#define PACKAGE_SIZE(uart_count) (META_SZ(uart_count)+META_CRC_SZ+UART_BUFFER_SIZE*uart_count) //seq number 2 bytes, (1byte cmd + 2bytes payload)*UART_COUNT, 1 byte crc, uart payload -> UART_BUFFER_SIZE*UART_COUNT

#define NET_NAME "ENC28J65E366"
#define NET_DOMAIN "lan"

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
        int portCount;
        int ringBuffSegCount;
        uint16_t udpPort;
        uint16_t tcpPort;
        std::string remoteAddr;

    public:
        //void AddListenAddr(const IPEndpoint &endpoint);
        void SetServiceIntervalMS(int intervalMS);
        void SetTCPBuffSz(int sz);
        void SetLingerSec(int sz);
        void SetMaxCTimeSec(int time);
        void SetPortCount(int portCount);
        void SetUARTBuffSz(int sz);
        void SetRingBuffSegCount(int cnt);
        void SetRemoteAddr(const std::string &addr);
        void SetUDPPort(uint16_t port);
        void SetTCPPort(uint16_t port);

        //from IConfig
        int GetServiceIntervalMS() const final;
        timeval GetServiceIntervalTV() const final;
        int GetTCPBuffSz() const final;
        int GetLingerSec() const final;
        int GetMaxCTimeSec() const final;
        int GetPortCount() const final;
        int GetPackageMetaSz() const final;
        int GetPackageSz() const final;
        int GetUARTBuffSz() const final;
        int GetRingBuffSegCount() const final;
        std::string GetRemoteAddr() const final;
        uint16_t GetTCPPort() const final;
        uint16_t GetUDPPort() const final;
};

#endif //CONFIG_H
