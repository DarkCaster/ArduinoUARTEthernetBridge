#include "segmented_buffer.h"

SegmentedBuffer::SegmentedBuffer()
{
    for(int i=0; i<BACK_BUFFER_SEGMENTS-1; ++i)
    {
        storage[i].usedSize=0;
        storage[i].nextSegment=&(storage[i+1]);
    }
    storage[BACK_BUFFER_SEGMENTS-1].usedSize=0;
    storage[BACK_BUFFER_SEGMENTS-1].nextSegment=&(storage[0]);
    head=&(storage[0]);
    tail=&(storage[0]);
}

Segment* SegmentedBuffer::GetFreeSegment()
{
    if(head->usedSize>0)
        return nullptr;
    return head;
}

Segment* SegmentedBuffer::GetUsedSegment()
{
    if(tail->usedSize<1)
        return nullptr;
    return tail;
}

void SegmentedBuffer::CommitSegment(Segment* segment)
{
    if(segment==head) //trying to commit head
        head=segment->nextSegment;
    else if(segment==tail) //trying to commit tail
        tail=segment->nextSegment;
}
