#ifndef LOOPBACKTESTER_H
#define LOOPBACKTESTER_H

#include "ILogger.h"
#include "WorkerBase.h"
#include "Connection.h"
#include <memory>
#include <atomic>

class LoopbackTester final : public WorkerBase
{
    private: //fields setup via constructor
        std::shared_ptr<ILogger> logger;
        Connection& target;
        std::atomic<bool> shutdownPending;
        size_t testBlockSize;
    public:
        LoopbackTester(std::shared_ptr<ILogger>& logger, Connection& target, size_t testBlockSize);
    protected:
        //WorkerBase
        void Worker() final;
        void OnShutdown() final;
};

#endif // LOOPBACKTESTER_H
