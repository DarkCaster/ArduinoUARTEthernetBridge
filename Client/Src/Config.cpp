#include "Config.h"

#define META_SZ(uart_count) (PKG_HDR_SZ+CMD_HDR_SIZE*uart_count)
#define META_CRC_SZ 1
#define PACKAGE_SIZE(uart_count,uart_buffer_size) (META_SZ(uart_count)+META_CRC_SZ+uart_buffer_size*uart_count) //seq number 2 bytes, (1byte cmd + 2bytes payload)*UART_COUNT, 1 byte crc, uart payload -> UART_BUFFER_SIZE*UART_COUNT
#define PORT_OFFSET(uart_count,uart_buffer_size,port_index) (META_SZ(uart_count)+META_CRC_SZ+uart_buffer_size*port_index)

void Config::SetServiceIntervalMS(int intervalMS)
{
    serviceInterval=intervalMS;
}

void Config::SetTCPBuffSz(int sz)
{
    TCPBuffSz=sz;
}

void Config::SetLingerSec(int sz)
{
    linger=sz;
}

void Config::SetMaxCTimeSec(int time)
{
    maxCtSec=time;
}

void Config::SetPortCount(int _portCount)
{
    portCount=_portCount;
}

void Config::SetUARTBuffSz(int sz)
{
    uartBuffSize=sz;
}

void Config::SetRingBuffSegCount(int cnt)
{
    ringBuffSegCount=cnt;
}

void Config::SetRemoteAddr(const std::string& addr)
{
    remoteAddr=addr;
}

void Config::SetUDPPort(uint16_t port)
{
    udpPort=port;
}

void Config::SetTCPPort(uint16_t port)
{
    tcpPort=port;
}

int Config::GetServiceIntervalMS() const
{
    return serviceInterval;
}

timeval Config::GetServiceIntervalTV() const
{
    return timeval{serviceInterval/1000,(serviceInterval-serviceInterval/1000*1000)*1000};
}

int Config::GetIncomingDataBufferSec() const
{
    return incomingDataBufferSec;
}

int Config::GetTCPBuffSz() const
{
    return TCPBuffSz;
}

int Config::GetLingerSec() const
{
    return linger;
}

int Config::GetMaxCTimeSec() const
{
    return maxCtSec;
}

int Config::GetPortCount() const
{
    return portCount;
}

int Config::GetPackageMetaSz() const
{
    return META_SZ(portCount);
}

int Config::GetPackageSz() const
{
    return PACKAGE_SIZE(portCount,uartBuffSize);
}

int Config::GetPortBuffOffset(int portIndex) const
{
    return PORT_OFFSET(portCount,uartBuffSize,portIndex);
}

int Config::GetUARTBuffSz() const
{
    return uartBuffSize;
}

int Config::GetRingBuffSegCount() const
{
    return ringBuffSegCount;
}

int Config::GetIdleTimerInterval() const
{
    return idleTimerInterval;
}

std::string Config::GetRemoteAddr() const
{
    return remoteAddr;
}

uint16_t Config::GetUDPPort() const
{
    return udpPort;
}

uint16_t Config::GetTCPPort() const
{
    return tcpPort;
}

