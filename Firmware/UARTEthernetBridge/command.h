#ifndef COMMAND_H
#define COMMAND_H

#include <Arduino.h>
#include "configuration.h"

enum struct ReqType : uint8_t
{
    NoCommand = 0x00,
    Reset = 0x01,
    Open = 0x02,
    ResetOpen = 0x03,
    Close = 0x04,
    Data = 0x08,
};

enum struct RespType : uint8_t
{
    NoCommand = 0x00,
    Data = 0x08,
};

struct Request
{
    ReqType type;
    uint8_t arg;
    uint8_t plSz;
};



struct Response
{
    RespType type;
    uint8_t arg;
    uint8_t plSz;
};



#endif // COMMAND_H
