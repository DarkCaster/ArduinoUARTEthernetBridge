#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include "Connection.h"
#include <mutex>

class TCPConnection final : public Connection
{
    private:
        bool isDisposed;
        error_t error;
        std::mutex stateLock;
    public:
        TCPConnection(const int fd);
        error_t GetStatus() final;
        void Fail(const error_t error) final;
        void Dispose() final;
};

#endif //TCP_CONNECTION_H
