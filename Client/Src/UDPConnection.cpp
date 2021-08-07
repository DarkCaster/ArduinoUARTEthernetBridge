#include "UDPConnection.h"

#include <unistd.h>

static bool falseVal=false;

UDPConnection::UDPConnection(const int _fd, const uint16_t _port):
    Connection(_fd),
    remotePort(_port)
{
    isDisposed.store(false);
    rxSeq=txSeq=UINT16_MAX;
}

bool UDPConnection::GetStatus()
{
    return !isDisposed.load();
}

void UDPConnection::Dispose()
{
    if(isDisposed.compare_exchange_strong(falseVal,true))
        close(fd);
}

uint16_t UDPConnection::GetUDPTransportPort()
{
    return remotePort;
}

uint16_t UDPConnection::TXSeqIncrement()
{
    return txSeq++;
}

bool UDPConnection::RXSeqCheckIncrement(uint16_t testSeq)
{
    if((rxSeq-testSeq)>(testSeq-rxSeq))
    {
        rxSeq=testSeq;
        return true;
    }
    return false;
}
