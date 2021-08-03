#ifndef CONNECTION_H
#define CONNECTION_H

//base class for connections that may be represented with fd - TCP, socket, file, pipe, etc..
class Connection
{
    public:
        const int fd;
        Connection(const int fd);
        virtual bool GetStatus() = 0;
        virtual void Dispose() = 0;
};

#endif // CONNECTION_H
