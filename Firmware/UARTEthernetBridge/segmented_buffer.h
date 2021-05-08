#ifndef SEGMENTED_BUFFER_H
#define SEGMENTED_BUFFER_H

#include <Arduino.h>
#include "configuration.h"

struct Segment
{
    uint8_t buffer[UART_BUFFER_SIZE];
    size_t usedSize;
    Segment* nextSegment;
};

class SegmentedBuffer
{
    private:
        Segment storage[BACK_BUFFER_SEGMENTS];
        Segment* head; //for free segments
        Segment* tail; //for used segments
    public:
        SegmentedBuffer();
        Segment* GetFreeSegment();
        Segment* GetUsedSegment();
        void CommitSegment(Segment* segment);
};

#endif // SEGMENTED_BUFFER_H
