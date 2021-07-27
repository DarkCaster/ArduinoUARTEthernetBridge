#include "Command.h"

#include "Config.h"

Request Request::Map(const int portIndex, const uint8_t* const rawBuffer)
{
    const auto offset=PKG_HDR_SZ+portIndex*CMD_HDR_SIZE;
    return Request{static_cast<ReqType>(*(rawBuffer+offset)),*(rawBuffer+offset+1),*(rawBuffer+offset+2)};
}

void Response::Write(const Response& source, const int portIndex, uint8_t* const rawBuffer)
{
    const auto offset=PKG_HDR_SZ+portIndex*CMD_HDR_SIZE;
    *(rawBuffer+offset)=static_cast<uint8_t>(source.type);
    *(rawBuffer+offset+1)=source.arg;
    *(rawBuffer+offset+2)=source.plSz;
}
