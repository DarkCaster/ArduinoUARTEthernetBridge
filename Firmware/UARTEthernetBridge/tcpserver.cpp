#include "tcpserver.h"
#include "crc8.h"
#include "configuration.h"

TCPServer::TCPServer(AlarmTimer& _alarmTimer, uint8_t* const _rxBuff, uint8_t* const _txBuff, const uint16_t _pkgSz, const uint16_t _metaSz, const uint16_t _netPort):
    alarmTimer(_alarmTimer),
    pkgSz(_pkgSz),
    metaSz(_metaSz),
    rxBuff(_rxBuff),
    txBuff(_txBuff),
    server(EthernetServer(_netPort))
{
    connected=false;
}

void TCPServer::Start()
{
    connected=false;
    server.begin();
}

ClientEvent TCPServer::ProcessRX()
{
    //not connected
    if(!connected)
    {
        client=server.accept();
        if(!client)
            return ClientEvent{ClientEventType::NoEvent,{.pkgReading=false}};
        connected=true;
        pkgLeft=pkgSz;
        alarmTimer.SetAlarmDelay(DEFAULT_ALARM_INTERVAL_MS);
        alarmTimer.SnoozeAlarm();
        return ClientEvent{ClientEventType::Connected,{.remoteAddr=client.remoteIP()}};
    }

    //check client is still connected and alive
    if(alarmTimer.AlarmTriggered() || !client.connected())
    {
        connected=false;
        client.stop();
        return ClientEvent{ClientEventType::Disconnected,{.remoteAddr=INADDR_NONE}};
    }

    unsigned int avail=client.available();

    if(avail<1)
        return ClientEvent{ClientEventType::NoEvent,{.pkgReading=pkgLeft<pkgSz}};

    if(pkgLeft>0)
        pkgLeft-=client.read(rxBuff+pkgSz-pkgLeft,pkgLeft>avail?avail:pkgLeft);

    if(pkgLeft>0)
        return ClientEvent{ClientEventType::NoEvent,{.pkgReading=true}};

    //check crc and disconnect on fail
    if(CRC8(rxBuff,metaSz)!=*(rxBuff+metaSz))
    {
        connected=false;
        client.stop();
        return ClientEvent{ClientEventType::Disconnected,{.remoteAddr=INADDR_NONE}};
    }

    //prepare reading next package
    pkgLeft=pkgSz;
    alarmTimer.SnoozeAlarm();
    //read port for UDP connection with new request
    return ClientEvent{ClientEventType::NewRequest,{.udpPort=static_cast<uint16_t>(*rxBuff|*(rxBuff+1)<<8)}};
}

bool TCPServer::ProcessTX()
{
    //clear PKG_HEADER (not needed) and calculate CRC
    *(txBuff)=*(txBuff+1)=0;
    *(txBuff+metaSz)=CRC8(txBuff,metaSz);
    auto dataLeft=pkgSz;
    while(dataLeft>0)
    {
        auto dw=client.write(txBuff+pkgSz-dataLeft,dataLeft);
        //TODO: check this if porting to other ethernet libs, UIPEthernet return 0 if client is not connected
        if(dw<1)
            return false;
        dataLeft-=dw;
    }
    return true;
}
