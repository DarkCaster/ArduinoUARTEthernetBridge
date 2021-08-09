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
    //read current counter value
    uint32_t counter=static_cast<uint32_t>(*(rawBuffer+PKG_CNT_OFFSET)|*(rawBuffer+PKG_CNT_OFFSET+1)<<8|*(rawBuffer+PKG_CNT_OFFSET+2)<<16|*(rawBuffer+PKG_CNT_OFFSET+3)<<24);
    return Response{static_cast<RespType>(*(rawBuffer+offset)),*(rawBuffer+offset+1),*(rawBuffer+offset+2),counter};
}

void WritePackageCounter(const uint32_t counter, uint8_t* const rawBuffer)
{
    *(rawBuffer+PKG_CNT_OFFSET+0)=static_cast<uint8_t>(counter&0xFF);
    *(rawBuffer+PKG_CNT_OFFSET+1)=static_cast<uint8_t>((counter>>8)&0xFF);
    *(rawBuffer+PKG_CNT_OFFSET+2)=static_cast<uint8_t>((counter>>16)&0xFF);
    *(rawBuffer+PKG_CNT_OFFSET+3)=static_cast<uint8_t>((counter>>24)&0xFF);
}
