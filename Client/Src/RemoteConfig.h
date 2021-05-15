#ifndef REMOTE_CONFIG_H
#define REMOTE_CONFIG_H

#include "IPEndpoint.h"
#include <cstdint>
#include <string>

class RemoteConfig
{
    public:
        const uint32_t speed;
        const uint8_t mode;
        const uint8_t flags;
        const uint8_t collectIntMS;
        const IPEndpoint listener;
        const std::string ptsListener;
        const std::string serverAddr;
        const uint16_t serverPort;
        RemoteConfig(const uint32_t speed,
                     const uint8_t mode,
                     const uint8_t flags,
                     const uint8_t collectIntMS,
                     const IPEndpoint &listener,
                     const std::string &ptsSymlink,
                     const std::string &serverAddr,
                     const uint16_t serverPort);
};

#endif //REMOTE_CONFIG_H
