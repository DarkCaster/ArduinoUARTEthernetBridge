#include "TCPConnection.h"

#include <unistd.h>

static bool falseVal=false;

TCPConnection::TCPConnection(const int _fd):
    Connection(_fd)
{
    isDisposed.store(false);
}

bool TCPConnection::GetStatus()
{
    return !isDisposed.load();
}

void TCPConnection::Dispose()
{
    if(isDisposed.compare_exchange_strong(falseVal,true))
        close(fd);
}
