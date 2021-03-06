#ifndef LOOPBACKTESTER_H
#define LOOPBACKTESTER_H

#include "ILogger.h"
#include "WorkerBase.h"
#include "Connection.h"

#include <memory>
#include <atomic>
#include <condition_variable>
#include <chrono>

class LoopbackTester final : public WorkerBase
{
    private:
        std::shared_ptr<ILogger> logger;
        Connection& target;
        const size_t testBlockSize;
        const uint64_t timeoutMS;
        const uint64_t warmupMS;
        std::unique_ptr<uint8_t[]> source;
        std::unique_ptr<uint8_t[]> test;
        std::atomic<bool> shutdownPending;
        size_t dataToWrite;
        std::mutex startTriggerLock;
        std::chrono::time_point<std::chrono::steady_clock> startPoint;
        std::condition_variable startTrigger;
        bool testStarted;
        bool Validate(size_t offset, size_t len);
    public:
        LoopbackTester(std::shared_ptr<ILogger>& logger, Connection& target, size_t testBlockSize, uint64_t timeoutMS, uint64_t warmupMS);
        bool ProcessTX();
    protected:
        //WorkerBase
        void Worker() final;
        void OnShutdown() final;
};

#endif // LOOPBACKTESTER_H
