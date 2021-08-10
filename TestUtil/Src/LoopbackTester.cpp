#include "LoopbackTester.h"

LoopbackTester::LoopbackTester(std::shared_ptr<ILogger>& _logger, Connection& _target, size_t _testBlockSize):
    logger(_logger),
    target(_target),
    testBlockSize(_testBlockSize)
{
    shutdownPending.store(false);
}

void LoopbackTester::Worker()
{
    logger->Info()<<"Reader started";
    while(!shutdownPending.load())
    {

    }
    if(shutdownPending.load())
        logger->Warning()<<"Reader stopped by external shutdown request!";
}

void LoopbackTester::OnShutdown()
{
    shutdownPending.store(true);
}
