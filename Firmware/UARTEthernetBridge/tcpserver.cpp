#include "tcpserver.h"

TCPServer::TCPServer(uint8_t* const _rxBuff, uint8_t* const _txBuff, const uint16_t _pkgSz, const uint16_t _netPort):
    pkgSz(_pkgSz),
    netPort(_netPort),
    rxBuff(_rxBuff),
    txBuff(_txBuff)
{
    connected=false;
}

void TCPServer::Start()
{
    connected=false;
    server.begin(netPort);
}

ClientEvent TCPServer::ProcessRX()
{
    //not connected
    if(!connected)
    {
        client=server.accept();
        if(!client)
            return ClientEvent{ClientEventType::NoEvent,{}};
        connected=true;
        pkgLeft=pkgSz;
        return ClientEvent{ClientEventType::Connected,{.remoteAddr=client.remoteIP()}};
    }

    //check it is still connected
    if(!client.connected())
    {
        connected=false;
        client.stop();
        return ClientEvent{ClientEventType::Disconnected,{}};
    }

    auto avail=client.available();

    if(avail<1)
        return ClientEvent{ClientEventType::NoEvent,{}};

    if(pkgLeft>0)
    {
        //TODO read as much data as we can
    }

    if(pkgLeft>0)
        return ClientEvent{ClientEventType::NoEvent,{}};

    //TODO: check crc
    //TODO: disconnect if CRC failed - this should not happen on TCP connection, so, client sending package with invalid size
    //TODO: read port for UDP connection
    uint16_t port=0;
    return ClientEvent{ClientEventType::NewRequest,{.udpPort=port}};
}
