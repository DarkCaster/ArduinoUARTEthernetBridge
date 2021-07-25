#ifndef PTYCONNECTION_H
#define PTYCONNECTION_H

#include "Connection.h"
#include <mutex>

class PTYConnection final : public Connection
{
    private:
        bool isDisposed;
        error_t error;
        std::mutex stateLock;
    public:
        PTYConnection(const int fd);
        error_t GetStatus() final;
        void SetStatus(const error_t error) final;
        void Dispose() final;
};


#endif // PTYCONNECTION_H
