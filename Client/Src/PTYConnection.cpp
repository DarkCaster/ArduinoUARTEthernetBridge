#include "PTYConnection.h"

#include <unistd.h>

PTYConnection::PTYConnection(const int _fd):
    Connection(_fd)
{
    error=0;
    isDisposed=false;
}

error_t PTYConnection::GetStatus()
{
    std::lock_guard<std::mutex> guard(stateLock);
    return error;
}

void PTYConnection::Fail(const error_t _error)
{
    std::lock_guard<std::mutex> guard(stateLock);
    if(isDisposed)
        return;
    error=_error;
}

void PTYConnection::Dispose()
{
    std::lock_guard<std::mutex> guard(stateLock);
    if(isDisposed)
        return;
    isDisposed=true;
}
