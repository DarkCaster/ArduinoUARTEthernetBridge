#ifndef COMMAND_H
#define COMMAND_H

#include <cstdint>

enum struct ReqType : uint8_t
{
    NoCommand = 0x00,
    Reset = 0x01,
    Open = 0x02,
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
    static void Write(const Request& source, const int portIndex, uint8_t* const rawBuffer);
    ReqType type;
    uint8_t arg;
    uint8_t plSz;
};

struct Response
{
    static Response Map(const int portIndex, const uint8_t* const rawBuffer);
    RespType type;
    uint8_t arg;
    uint8_t plSz;
    uint32_t counter;
};

void WriteU32Value(const uint32_t value, uint8_t* const target);
void WriteU16Value(const uint16_t value, uint8_t* const target);

#endif // COMMAND_H
