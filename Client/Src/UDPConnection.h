#ifndef UDPCONNECTION_H
#define UDPCONNECTION_H

#include "Connection.h"
#include <atomic>

class UDPConnection final : public Connection
{
    private:
        std::atomic<bool> isDisposed;
        const uint16_t remotePort;
        uint16_t txSeq;
        uint16_t rxSeq;
    public:
        UDPConnection(const int fd, const uint16_t port);
        bool GetStatus() final;
        void Dispose() final;
        uint16_t GetUDPTransportPort();
        uint16_t TXSeqIncrement();
        bool RXSeqCheckIncrement(uint16_t testSeq);
};

#endif // UDPCONNECTION_H
