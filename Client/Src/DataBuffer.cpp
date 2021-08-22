#include "DataBuffer.h"

DataBuffer::DataBuffer(size_t _size):
    storage(std::make_unique<uint8_t[]>(_size)),
    size(_size),
    begin(storage.get()),
    end(storage.get()+_size)
{
    Reset();
}

size_t DataBuffer::GetSize()
{
    return size;
}

Handle DataBuffer::GetHead()
{
    Handle result={head,static_cast<size_t>(head<tail?tail-head-1:end-head-(tail>begin?0:1))};
    return result;
}

Handle DataBuffer::GetTail()
{
    Handle result={tail,static_cast<size_t>((head>=tail)?head-tail:end-tail)};
    return result;
}

void DataBuffer::Commit(const Handle& handle, size_t usedSz)
{
    uint8_t ** const target=handle.buffer==head?&head:&tail;
    *target+=usedSz;
    if(*target>=end)
        *target=begin;
}

size_t DataBuffer::UsedSize()
{
    return static_cast<size_t>(head>=tail?static_cast<size_t>(head-tail):size-static_cast<size_t>(tail-head-1));
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
