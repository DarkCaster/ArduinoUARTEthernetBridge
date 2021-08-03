#include "PTYConnection.h"

#include <unistd.h>

PTYConnection::PTYConnection(const int _fd):
    Connection(_fd)
{
    isDisposed.store(false);
}

bool PTYConnection::GetStatus()
{
    return !isDisposed.load();
}

void PTYConnection::Dispose()
{
    isDisposed.store(true);
}
