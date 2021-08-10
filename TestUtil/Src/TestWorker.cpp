#include "TestWorker.h"

TestWorker::TestWorker(std::shared_ptr<ILogger>& _logger, LoopbackTester& _tester):
    logger(_logger),
    tester(_tester)
{
    shutdownPending.store(false);
}

void TestWorker::Worker()
{
    while(!shutdownPending.load() && tester.ProcessTX()) { }
    if(shutdownPending.load())
        logger->Warning()<<"Aborted by shutdown request!";
}

void TestWorker::OnShutdown()
{
    shutdownPending.store(true);
}
