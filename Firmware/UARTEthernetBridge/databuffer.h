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
        uint8_t * const begin;
        uint8_t * const end;
        uint8_t *head;
        uint8_t *tail;
    public:
        DataBuffer();
        Handle GetHead();
        Handle GetTail();
        void Commit(const Handle &handle, uint16_t usedSz);
        uint16_t UsedSize();
        bool IsHalfUsed();
};

#endif // DATABUFFER_H
