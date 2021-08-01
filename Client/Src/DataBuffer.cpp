#include "DataBuffer.h"

DataBuffer::DataBuffer(size_t _size):
    storage(std::make_unique<uint8_t[]>(_size)),
    size(_size),
    begin(storage.get()),
    end(storage.get()+_size)
{
    Reset();
}

Handle DataBuffer::GetHead()
{
    Handle result={head,static_cast<uint16_t>(head<tail?tail-head-1:end-head-(tail>begin?0:1))};
    return result;
}

Handle DataBuffer::GetTail()
{
    Handle result={tail,static_cast<uint16_t>((head>=tail)?head-tail:end-tail)};
    return result;
}

void DataBuffer::Commit(const Handle& handle, uint16_t usedSz)
{
    uint8_t ** const target=handle.buffer==head?&head:&tail;
    *target+=usedSz;
    if(*target>=end)
        *target=begin;
}

uint16_t DataBuffer::UsedSize()
{
    return static_cast<uint16_t>(head>=tail?head-tail:(end-tail)+(head-begin));
}

bool DataBuffer::IsHalfUsed()
{
    return UsedSize()>(size/2);
}

void DataBuffer::Reset()
{
    head=begin;
    tail=begin;
}
