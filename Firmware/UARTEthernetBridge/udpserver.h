#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <Arduino.h>
#include <IPAddress.h>
#include <UIPUdp.h>

#include "watchdog.h"
#include "clientstate.h"

class UDPServer
{
    private:
        Watchdog& watchDog;
        const size_t pkgSz;
        const size_t metaSz;
        uint8_t * const rxBuff;
        uint8_t * const txBuff;
        IPAddress clientAddr = INADDR_NONE;
        uint16_t serverSeq = 0;
        uint16_t clientUDPPort = 0;
        uint16_t clientSeq = 0;
        bool serverStarted = false;
        UIPUDP udpServer;
    public:
        UDPServer(Watchdog& watchDog, uint8_t * const rxBuff, uint8_t * const txBuff, const uint16_t pkgSz, const uint16_t metaSz);
        ClientEvent ProcessRX(const ClientEvent& ctlEvent);
        bool ProcessTX();
    private:
        bool DropOldSeq();
};

#endif // UDPSERVER_H
