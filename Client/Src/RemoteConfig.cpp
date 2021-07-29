#include "RemoteConfig.h"



RemoteConfig::RemoteConfig(const uint32_t _speed,
                           const SerialMode _mode,
                           const IPEndpoint& _listener,
                           const std::string& _ptsSymlink):
    speed(_speed),
    mode(_mode),
    listener(_listener),
    ptsListener(_ptsSymlink)
{
}
