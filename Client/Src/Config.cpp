#include "Config.h"

#define META_SZ(uart_count) (PKG_HDR_SZ+CMD_HDR_SIZE*uart_count)
#define META_CRC_SZ 1
#define PACKAGE_SIZE(uart_count,uart_buffer_size) (META_SZ(uart_count)+META_CRC_SZ+uart_buffer_size*uart_count)
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

void Config::SetNetworkPayloadSz(int sz)
{
    nwPayloadSize=sz;
}

void Config::SetRingBuffSegCount(int cnt)
{
    ringBuffSegCount=cnt;
}

void Config::SetRemoteAddr(const std::string& addr)
{
    remoteAddr=addr;
}

void Config::SetUDPEnabled(bool _enableUDP)
{
    enableUDP=_enableUDP;
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
    return PACKAGE_SIZE(portCount,nwPayloadSize);
}

int Config::GetPortBuffOffset(int portIndex) const
{
    return PORT_OFFSET(portCount,nwPayloadSize,portIndex);
}

int Config::GetNetworkPayloadSz() const
{
    return nwPayloadSize;
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

bool Config::GetUDPEnabled() const
{
    return enableUDP;
}

uint16_t Config::GetTCPPort() const
{
    return tcpPort;
}

