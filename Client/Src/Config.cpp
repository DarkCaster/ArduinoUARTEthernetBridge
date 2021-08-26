#include "Config.h"

#define META_SZ(uart_count) (PKG_HDR_SZ+CMD_HDR_SIZE*uart_count)
#define META_CRC_SZ 1
#define PACKAGE_SIZE(uart_count,data_payload_size) (META_SZ(uart_count)+META_CRC_SZ+data_payload_size*uart_count)
#define PORT_OFFSET(uart_count,data_payload_size,port_index) (META_SZ(uart_count)+META_CRC_SZ+data_payload_size*port_index)

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

void Config::SetPortCount(int _portCount)
{
    portCount=_portCount;
}

void Config::SetHwUARTSz(int sz)
{
    hwUARTSize=sz;
}

void Config::SetNwMult(int mult)
{
    nwMult=mult;
}

void Config::SetRingBuffSize(int size)
{
    ringBuffSize=size;
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
    return PACKAGE_SIZE(portCount,(hwUARTSize*nwMult));
}

int Config::GetPortBuffOffset(int portIndex) const
{
    return PORT_OFFSET(portCount,(hwUARTSize*nwMult),portIndex);
}

int Config::GetNetworkPayloadSz() const
{
    return hwUARTSize*nwMult;
}

int Config::GetHwUARTSz() const
{
    return hwUARTSize;
}

int Config::GetRingBuffSize() const
{
    return ringBuffSize;
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

