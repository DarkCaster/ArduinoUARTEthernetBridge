#ifndef TESTWORKER_H
#define TESTWORKER_H

#include "ILogger.h"
#include "WorkerBase.h"
#include "LoopbackTester.h"

#include <memory>
#include <atomic>

class TestWorker final : public WorkerBase
{
    private:
        std::shared_ptr<ILogger> logger;
        LoopbackTester& tester;
        std::atomic<bool> shutdownPending;
    public:
        TestWorker(std::shared_ptr<ILogger>& logger, LoopbackTester& tester);
    protected:
        //WorkerBase
        void Worker() final;
        void OnShutdown() final;
};

#endif // TESTWORKER_H
