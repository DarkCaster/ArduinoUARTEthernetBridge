#ifndef DATABUFFER_H
#define DATABUFFER_H

#include <Arduino.h>
#include "configuration.h"

struct Handle
{
    uint8_t *buffer;
    uint16_t maxSz;
};

class DataBuffer
{
    private:
        uint8_t storage[DATA_BUFFER_SIZE];
        const uint8_t *begin;
        const uint8_t *end;
        uint8_t* head;
        uint8_t* tail;
    public:
        DataBuffer();
        Handle GetHead();
        Handle GetTail();
        void CommitHead(uint16_t sz);
        void CommitTail(uint16_t sz);
};

#endif // DATABUFFER_H
