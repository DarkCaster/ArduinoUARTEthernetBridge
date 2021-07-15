#include "tcpserver.h"
#include "crc8.h"

TCPServer::TCPServer(uint8_t* const _rxBuff, uint8_t* const _txBuff, const uint16_t _pkgSz, const uint16_t _metaSz, const uint16_t _netPort):
    pkgSz(_pkgSz),
    metaSz(_metaSz),
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

    unsigned int avail=client.available();

    if(avail<1)
        return ClientEvent{ClientEventType::NoEvent,{}};

    if(pkgLeft>0)
        pkgLeft-=client.read(rxBuff+pkgSz-pkgLeft,pkgLeft>avail?avail:pkgLeft);

    if(pkgLeft>0)
        return ClientEvent{ClientEventType::NoEvent,{}};

    //check crc and disconnect on fail
    if(CRC8(rxBuff,metaSz)!=*(rxBuff+metaSz))
    {
        connected=false;
        client.stop();
        return ClientEvent{ClientEventType::Disconnected,{}};
    }

    //read port for UDP connection
    return ClientEvent{ClientEventType::NewRequest,{.udpPort=static_cast<uint16_t>(*rxBuff|*(rxBuff+1)<<8)}};
}
