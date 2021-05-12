#include "TCPConnection.h"

#include <unistd.h>

TCPConnection::TCPConnection(const int _fd):
    Connection(_fd)
{
    error=0;
    isDisposed=false;
}

error_t TCPConnection::GetStatus()
{
    std::lock_guard<std::mutex> guard(stateLock);
    return error;
}

void TCPConnection::Fail(const error_t _error)
{
    std::lock_guard<std::mutex> guard(stateLock);
    if(isDisposed)
        return;
    error=_error;
}

void TCPConnection::Dispose()
{
    std::lock_guard<std::mutex> guard(stateLock);
    if(isDisposed)
        return;
    isDisposed=true;
    close(fd);
}
