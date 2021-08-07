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
    if((serverSeq-checkSeq)>(checkSeq-serverSeq))
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
                //reset MCU if server was not started
                if(!serverStarted)
                    watchDog.SystemReset();
            }
            //nothing more to do at this point
            return ctlEvent;
        case ClientEventType::NoEvent:
        default:
            //return here if UDP server is not started
            if(!serverStarted)
                return ctlEvent;
            break;
    }

    //parse and read the packet, verify CRC for package metadata, veridy sequence number
    size_t inSz=udpServer.parsePacket();
    if(
        inSz!=pkgSz || //check package size
        static_cast<size_t>(udpServer.read(rxBuff,pkgSz))!=pkgSz || //try to read it
        udpServer.remoteIP()!=clientAddr || //check remote address
        CRC8(rxBuff,metaSz)!=*(rxBuff+metaSz) || //check CRC for package metadata
        DropOldSeq() //process package seqence number
      )
    {
        if(inSz!=0)
            udpServer.flush();
        return ClientEvent{ClientEventType::NoEvent,{}};
    }

    //record client's port if all OK, and flush the current package
    if(clientUDPPort<1)
        clientUDPPort=udpServer.remotePort();
    udpServer.flush();

    //defer connection state tracking alarm
    alarmTimer.SnoozeAlarm();

    //request is ready
    return ClientEvent{ClientEventType::NewRequest,{.udpSeq=serverSeq}};
}

bool UDPServer::ProcessTX()
{
    //do not attempt to send anything if we still do not known client's local port
    if(!serverStarted||clientUDPPort<1)
        return false;
    if(udpServer.beginPacket(clientAddr,clientUDPPort)!=1)
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
