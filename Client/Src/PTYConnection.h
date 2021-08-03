#ifndef PTYCONNECTION_H
#define PTYCONNECTION_H

#include "Connection.h"

#include <atomic>

class PTYConnection final : public Connection
{
    private:
        std::atomic<bool> isDisposed;
    public:
        PTYConnection(const int fd);
        bool GetStatus() final;
        void Dispose() final;
};


#endif // PTYCONNECTION_H
