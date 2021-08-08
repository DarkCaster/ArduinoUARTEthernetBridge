#ifndef REMOTEBUFFERTRACKER_H
#define REMOTEBUFFERTRACKER_H

#include "ILogger.h"
#include "IMessageSubscriber.h"
#include "IConfig.h"

#include <cstdint>
#include <memory>
#include <deque>

struct RemoteBufferPkg
{
    uint32_t counter;
    size_t dataSize;
};

class RemoteBufferTracker final : public IMessageSubscriber
{
    private:
        std::shared_ptr<ILogger> logger;
        const IConfig& config;
        const size_t bufferLimit;
        uint32_t counter;
        bool isBlocked;
        std::deque<RemoteBufferPkg> dataSent;
    public:
        RemoteBufferTracker(std::shared_ptr<ILogger>& logger, const IConfig& config, const size_t bufferLimit);
        void AddPackage(size_t size);
        void ConfirmPackage(uint32_t refCounter, bool writeBlocked);
        size_t GetAvailSpace();
        void Reset();
        //methods for ISubscriber
        bool ReadyForMessage(const MsgType msgType) final;
        void OnMessage(const void* const source, const IMessage& message) final;
};

#endif // REMOTEBUFFERTRACKER_H
