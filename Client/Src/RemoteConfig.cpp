#include "RemoteConfig.h"

RemoteConfig::RemoteConfig(const uint32_t _speed, const uint8_t _mode, const uint8_t _flags, const uint8_t _collectIntMS, const IPEndpoint& _listener, const std::string& _serverAddr, const uint16_t _serverPort):
    speed(_speed),
    mode(_mode),
    flags(_flags),
    collectIntMS(_collectIntMS),
    listener(_listener),
    serverAddr(_serverAddr),
    serverPort(_serverPort)
{
}
