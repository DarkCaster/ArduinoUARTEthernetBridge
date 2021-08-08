#include "RemoteConfig.h"



PortConfig::PortConfig(const uint32_t _speed,
                           const SerialMode _mode,
                           const bool _resetOnConnect,
                           const int64_t _pollInterval,
                           const IPEndpoint& _listener,
                           const std::string& _ptsSymlink):
    speed(_speed),
    mode(_mode),
    resetOnConnect(_resetOnConnect),
    pollInterval(_pollInterval),
    listener(_listener),
    ptsListener(_ptsSymlink)
{
}
