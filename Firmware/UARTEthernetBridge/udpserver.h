#ifndef UDPSERVER_H
#define UDPSERVER_H

#include <Arduino.h>
#include <IPAddress.h>
#include <EthernetUdp.h>

#include "alarmtimer.h"
#include "watchdog.h"
#include "clientstate.h"

class UDPServer
{
    private:
        AlarmTimer& alarmTimer;
        const size_t pkgSz;
        const size_t metaSz;
        uint8_t * const rxBuff;
        uint8_t * const txBuff;
        IPAddress clientAddr = INADDR_NONE;
        uint16_t serverSeq = 0;
        uint16_t clientUDPPort = 0;
        uint16_t clientSeq = 0;
        bool serverStarted = false;
        EthernetUDP udpServer;
    public:
        UDPServer(AlarmTimer& alarmTimer, uint8_t * const rxBuff, uint8_t * const txBuff, const uint16_t pkgSz, const uint16_t metaSz);
        ClientEvent ProcessRX(const ClientEvent& ctlEvent);
        bool ProcessTX();
    private:
        bool DropOldSeq();
};

#endif // UDPSERVER_H
