#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include "Connection.h"
#include <atomic>

class TCPConnection final : public Connection
{
    private:
        std::atomic<bool> isDisposed;
    public:
        TCPConnection(const int fd);
        bool GetStatus() final;
        void Dispose() final;
};

#endif //TCP_CONNECTION_H
