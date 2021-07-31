#include "Command.h"

#include "Config.h"

void Request::Write(const Request& source, const int portIndex, uint8_t* const rawBuffer)
{
    const auto offset=PKG_HDR_SZ+portIndex*CMD_HDR_SIZE;
    *(rawBuffer+offset)=static_cast<uint8_t>(source.type);
    *(rawBuffer+offset+1)=source.arg;
    *(rawBuffer+offset+2)=source.plSz;
}

Response Response::Map(const int portIndex, const uint8_t* const rawBuffer)
{
    const auto offset=PKG_HDR_SZ+portIndex*CMD_HDR_SIZE;
    return Response{static_cast<RespType>(*(rawBuffer+offset)),*(rawBuffer+offset+1),*(rawBuffer+offset+2)};
}
