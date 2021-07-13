#ifndef COMMAND_H
#define COMMAND_H

#include <Arduino.h>
#include "configuration.h"

enum struct ReqType : uint8_t
{
    ResetOpen = 0x01,
    Open = 0x02,
    Close = 0x04,
    Data = 0x08,
};

enum struct RespType : uint8_t
{
    Data = 0x08,
};

struct Request
{
    static Request Map(const int portIndex, const uint8_t * const rawBuffer);
    ReqType type;
    uint8_t arg;
    uint8_t plSz;
};

struct Response
{
    static void Write(const Response &source, const int portIndex, uint8_t * const rawBuffer);
    RespType type;
    uint8_t arg;
    uint8_t plSz;
};

#endif // COMMAND_H
