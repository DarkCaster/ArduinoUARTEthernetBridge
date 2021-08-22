#include "udpserver.h"
#include "crc8.h"

UDPServer::UDPServer(AlarmTimer& _alarmTimer, Watchdog& _watchDog, uint8_t* const _rxBuff, uint8_t* const _txBuff, const uint16_t _pkgSz, const uint16_t _metaSz):
    alarmTimer(_alarmTimer),
    watchDog(_watchDog),
    pkgSz(_pkgSz),
    metaSz(_metaSz),
    rxBuff(_rxBuff),
    txBuff(_txBuff)
{
    clientAddr = INADDR_NONE;
    clientUDPPort = 0;
    serverSeq = clientSeq = 0;
    serverStarted = false;
}

bool UDPServer::DropOldSeq()
{
    auto checkSeq=static_cast<uint16_t>(*rxBuff|*(rxBuff+1)<<8);
    if(static_cast<uint16_t>(serverSeq-checkSeq)>static_cast<uint16_t>(checkSeq-serverSeq))
    {
        serverSeq=checkSeq;
        return false;
    }
    return true;
}

ClientEvent UDPServer::ProcessRX(const ClientEvent &ctlEvent)
{
    switch (ctlEvent.type)
    {
        //on connect/disconnect events reset currently configured UDP connection
        case ClientEventType::Disconnected:
        case ClientEventType::Connected:
            if(serverStarted)
            {
                udpServer.stop();
                serverStarted=false;
            }
            clientUDPPort = 0;
            serverSeq = clientSeq = 0;
            //store client address for serving UDP connection
            clientAddr=ctlEvent.data.remoteAddr;
            //nothing more to do at this point
            return ctlEvent;
        case ClientEventType::NewRequest:
            //try to start UIP server
            if(!serverStarted && ctlEvent.data.udpPort>0)
            {
                serverStarted=udpServer.begin(ctlEvent.data.udpPort)==1;
                return ClientEvent{ClientEventType::NewRequest,{.udpSrvStarted=serverStarted}};
            }
            //nothing more to do at this point
            return ctlEvent;
        case ClientEventType::NoEvent:
        default:
            //return here if UDP server is not started
            if(!serverStarted||ctlEvent.data.pkgReading)
                return ctlEvent;
            break;
    }

    //parse and read the packet, flush it as fast as possible
    size_t inSz=udpServer.parsePacket();
    if(inSz<1)
        return ClientEvent{ClientEventType::NoEvent,{.pkgReading=false}};
    auto dr=static_cast<size_t>(udpServer.read(rxBuff,pkgSz));
    udpServer.flush();

    if(dr!=pkgSz||CRC8(rxBuff,metaSz)!=*(rxBuff+metaSz)||DropOldSeq())
        return ClientEvent{ClientEventType::NoEvent,{.pkgReading=false}};

    //record client's port if all OK
    if(clientUDPPort<1)
        clientUDPPort=udpServer.remotePort();

    //defer connection state tracking alarm
    alarmTimer.SnoozeAlarm();

    //request is ready
    return ClientEvent{ClientEventType::NewRequest,{.udpSrvStarted=false}};
}

bool UDPServer::ProcessTX()
{
    //do not attempt to send anything if we still do not known client's local port
    if(!serverStarted||clientUDPPort<1||udpServer.beginPacket(clientAddr,clientUDPPort)!=1)
        return false;
    //fill-up server sequence
    *(txBuff)=clientSeq&0xFF;
    *(txBuff+1)=(clientSeq>>8)&0xFF;
    clientSeq++;
    //calculate CRC for package metadata
    *(txBuff+metaSz)=CRC8(txBuff,metaSz);
    //send package
    udpServer.write(txBuff,pkgSz);
    udpServer.endPacket();
    return true;
}
