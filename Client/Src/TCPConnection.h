#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include "Connection.h"
#include <atomic>

class TCPConnection final : public Connection
{
    private:
        std::atomic<bool> isDisposed;
        const uint16_t udpTransportPort;
    public:
        TCPConnection(const int fd, const uint16_t udpPort);
        bool GetStatus() final;
        void Dispose() final;
        uint16_t GetUDPTransportPort();
};

#endif //TCP_CONNECTION_H
