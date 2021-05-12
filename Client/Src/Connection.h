#ifndef CONNECTION_H
#define CONNECTION_H

#include <cerrno>

//base class for connections that may be represented with fd - TCP, socket, file, pipe, etc..
class Connection
{
    public:
        const int fd;
        Connection(const int fd);
        virtual error_t GetStatus() = 0;
        virtual void Fail(const error_t error) = 0;
        virtual void Dispose() = 0;
};

#endif // CONNECTION_H
