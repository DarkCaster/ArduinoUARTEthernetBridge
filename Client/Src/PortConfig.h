#ifndef REMOTE_CONFIG_H
#define REMOTE_CONFIG_H

#include "IPEndpoint.h"
#include <cstdint>
#include <string>

enum class SerialMode : uint8_t
{
    SERIAL_5N1=0x00,
    SERIAL_6N1=0x02,
    SERIAL_7N1=0x04,
    SERIAL_8N1=0x06,
    SERIAL_5N2=0x08,
    SERIAL_6N2=0x0A,
    SERIAL_7N2=0x0C,
    SERIAL_8N2=0x0E,
    SERIAL_5E1=0x20,
    SERIAL_6E1=0x22,
    SERIAL_7E1=0x24,
    SERIAL_8E1=0x26,
    SERIAL_5E2=0x28,
    SERIAL_6E2=0x2A,
    SERIAL_7E2=0x2C,
    SERIAL_8E2=0x2E,
    SERIAL_5O1=0x30,
    SERIAL_6O1=0x32,
    SERIAL_7O1=0x34,
    SERIAL_8O1=0x36,
    SERIAL_5O2=0x38,
    SERIAL_6O2=0x3A,
    SERIAL_7O2=0x3C,
    SERIAL_8O2=0x3E,
    SERIAL_LOOPBACK=0xFF,
};

struct PortConfig
{
    public:
        const uint32_t speed;
        const SerialMode mode;
        const bool resetOnConnect;
        const int64_t pollInterval;
        const IPEndpoint listener;
        const std::string ptsListener;
        PortConfig(const uint32_t speed,
                     const SerialMode mode,
                     const bool resetOnConnect,
                     const int64_t pollInterval,
                     const IPEndpoint& listener,
                     const std::string& ptsSymlink);
};

#endif //REMOTE_CONFIG_H
