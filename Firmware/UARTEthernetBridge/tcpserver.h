#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <Arduino.h>
#include <UIPServer.h>
#include <UIPClient.h>
#include "clientstate.h"

class TCPServer
{
    private:
        const uint16_t pkgSz;
        const uint16_t metaSz;
        const uint16_t netPort;
        uint8_t * const rxBuff;
        uint8_t * const txBuff;
        UIPServer server=UIPServer(0);
        UIPClient client;

        bool connected;
        uint16_t pkgLeft;
    public:
        TCPServer(uint8_t * const rxBuff, uint8_t * const txBuff, const uint16_t pkgSz, const uint16_t metaSz, const uint16_t netPort);
        void Start();
        ClientEvent ProcessRX();
};

#endif // TCPSERVER_H
