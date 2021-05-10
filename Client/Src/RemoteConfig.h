#ifndef REMOTE_CONFIG_H
#define REMOTE_CONFIG_H

#include <IPEndpoint.h>

#define CFG_RESET_REQUESTED(cfg) ((cfg.flags&0x1)!=0)
#define CFG_TEST_MODE(cfg) ((cfg.flags&0x2)!=0)

#include <cstdint>
#include <string>

class RemoteConfig
{
    uint32_t speed;
    uint8_t mode;
    uint8_t flags;
    uint8_t collectIntMS;
    IPEndpoint listener;
    std::string serverAddr;
    uint16_t serverPort;
};

#endif //REMOTE_CONFIG_H
