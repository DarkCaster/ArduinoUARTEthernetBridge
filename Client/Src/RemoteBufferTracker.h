#ifndef REMOTEBUFFERTRACKER_H
#define REMOTEBUFFERTRACKER_H

#include "ILogger.h"
#include "IConfig.h"

#include <cstdint>
#include <memory>
#include <deque>

struct RemoteBufferPkg
{
    uint32_t counter;
    size_t dataSize;
};

class RemoteBufferTracker
{
    private:
        std::shared_ptr<ILogger> logger;
        const IConfig& config;
        const size_t bufferLimit;
        bool isBlocked;
        std::deque<RemoteBufferPkg> dataSent;
    public:
        RemoteBufferTracker(std::shared_ptr<ILogger>& logger, const IConfig& config, const size_t bufferLimit);
        void AddPackage(size_t size, uint32_t counter);
        void ConfirmPackage(uint32_t refCounter, bool writeBlocked);
        size_t GetAvailSpace();
        void Reset();
};

#endif // REMOTEBUFFERTRACKER_H
