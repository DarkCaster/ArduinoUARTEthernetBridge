#include "udpserver.h"

UDPServer::UDPServer(uint8_t* const _rxBuff, uint8_t* const _txBuff, const uint16_t _pkgSz, const uint16_t _metaSz):
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

ClientEvent UDPServer::ProcessRX(const ClientEvent &ctlEvent)
{
    switch (ctlEvent.type)
    {
        //on connect/disconnect reset currently configured UDP connection
        case ClientEventType::Disconnected:
        case ClientEventType::Connected:
            if(serverStarted)
            {
                //TODO: close UDP server
                serverStarted=false;
            }
            clientUDPPort = 0;
            serverSeq = clientSeq = 0;
            //store client address for serving UDP connection
            clientAddr=ctlEvent.data.remoteAddr;
            //nothing more to do at this point
            return ctlEvent;
        case ClientEventType::NewRequest:
            //store server UDP port for listen UDP packets at
            if(!serverStarted && ctlEvent.data.udpPort>0)
            {
                //TODO: start UDP server
                serverStarted=true;
            }
            //nothing more to do at this point
            return ctlEvent;
        //try to process further if nothing happened on TCP side
        case ClientEventType::NoEvent:
        default:
            //return here if UDP server is not started
            if(!serverStarted)
               return ctlEvent;
            break;
    }

    //TODO: read the package
    //TODO: compare package origin IP address
    //TODO: verify CRC for control block
    //TODO: compare sequence address
    //TODO: record client port if all OK
    return ClientEvent{ClientEventType::NewRequest,{.udpSeq=serverSeq}};
}

bool UDPServer::ProcessTX()
{
    return false;
}
