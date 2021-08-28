#ifndef IMMUTABLESTORAGE_H
#define IMMUTABLESTORAGE_H

#include <queue>

template <class T> class ImmutableStorage
{
    private:
        std::queue<T> storage;
    public:
        bool isUpdated;
        ImmutableStorage(const T &payload) {storage.push(payload);storage.push(payload);isUpdated=false;}
        T Get() const {return storage.back();}
        T Prev() const {return storage.front();}
        void Set(const T &payload) {storage.pop();storage.push(payload);isUpdated=true;}

        ImmutableStorage<T>& operator=(const T &payload) {Set(payload); return *this;}
};

#endif // IMMUTABLESTORAGE_H
