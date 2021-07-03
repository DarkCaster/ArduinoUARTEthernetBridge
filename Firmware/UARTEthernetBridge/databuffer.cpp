#include "databuffer.h"

DataBuffer::DataBuffer():
    begin(&(storage[0])),
    end(&(storage[DATA_BUFFER_SIZE]))
{
    head=&(storage[0]);
    tail=&(storage[0]);
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

void DataBuffer::CommitHead(uint16_t sz)
{
    //move head to the right
    head+=sz;
    if(head>=end)
        head=0;
}

void DataBuffer::CommitTail(uint16_t sz)
{
    //move tail to the right
    tail+=sz;
    if(tail>=end)
        tail=0;
}
