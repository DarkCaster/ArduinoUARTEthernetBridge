#ifndef CLIENTSTATE_H
#define CLIENTSTATE_H

#include <Arduino.h>

enum struct ClientEventType : uint8_t
{
    NoEvent=0x00,
    Connected=0x01,
    NewRequest=0x02,
    Disconnected=0xFF,
};

union ClientEventData
{
    uint32_t remoteAddr;
    uint16_t udpPort;
    uint16_t udpSeq;
};

struct ClientEvent
{
    ClientEventType type;
    ClientEventData data;
};

#endif // CLIENTSTATE_H
