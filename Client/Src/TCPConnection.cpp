#include "TCPConnection.h"

#include <unistd.h>

static bool falseVal=false;

TCPConnection::TCPConnection(const int _fd, const uint16_t udpPort):
    Connection(_fd),
    udpTransportPort(udpPort)
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

uint16_t TCPConnection::GetUDPTransportPort()
{
    return udpTransportPort;
}
