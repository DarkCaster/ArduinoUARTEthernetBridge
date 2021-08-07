#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <Arduino.h>
#include <UIPServer.h>
#include <UIPClient.h>

#include "alarmtimer.h"
#include "clientstate.h"

class TCPServer
{
    private:
        AlarmTimer& alarmTimer;
        const size_t pkgSz;
        const size_t metaSz;
        const uint16_t netPort;
        uint8_t * const rxBuff;
        uint8_t * const txBuff;
        UIPServer server=UIPServer(0);
        UIPClient client;

        bool connected;
        size_t pkgLeft;
    public:
        TCPServer(AlarmTimer& alarmTimer, uint8_t * const rxBuff, uint8_t * const txBuff, const uint16_t pkgSz, const uint16_t metaSz, const uint16_t netPort);
        void Start();
        ClientEvent ProcessRX();
        bool ProcessTX();
};

#endif // TCPSERVER_H
