#ifndef DATABUFFER_H
#define DATABUFFER_H

#include <cstdint>
#include <cstddef>
#include <memory>

struct Handle
{
    uint8_t *buffer;
    size_t maxSz;
};

class DataBuffer
{
    private:
        std::unique_ptr<uint8_t[]> storage;
        const size_t size;
        uint8_t * const begin;
        uint8_t * const end;
        uint8_t *head;
        uint8_t *tail;
    public:
        DataBuffer(size_t size);
        Handle GetHead();
        Handle GetTail();
        void Commit(const Handle &handle, size_t usedSz);
        size_t UsedSize();
        bool IsHalfUsed();
        void Reset();
};

#endif // DATABUFFER_H
