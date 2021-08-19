#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <Arduino.h>
#include <EthernetServer.h>
#include <EthernetClient.h>

#include "alarmtimer.h"
#include "clientstate.h"

class TCPServer
{
    private:
        AlarmTimer& alarmTimer;
        const size_t pkgSz;
        const size_t metaSz;
        uint8_t * const rxBuff;
        uint8_t * const txBuff;
        EthernetServer server;
        EthernetClient client;

        bool connected;
        size_t pkgLeft;
    public:
        TCPServer(AlarmTimer& alarmTimer, uint8_t * const rxBuff, uint8_t * const txBuff, const uint16_t pkgSz, const uint16_t metaSz, const uint16_t netPort);
        void Start();
        ClientEvent ProcessRX();
        bool ProcessTX();
};

#endif // TCPSERVER_H
