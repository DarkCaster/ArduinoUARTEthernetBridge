#include "LoopbackTester.h"
#include <random>

LoopbackTester::LoopbackTester(std::shared_ptr<ILogger>& _logger, Connection& _target, size_t _testBlockSize, uint64_t _timeoutMS):
    logger(_logger),
    target(_target),
    testBlockSize(_testBlockSize),
    timeoutMS(_timeoutMS)
{
    source=std::make_unique<uint8_t[]>(testBlockSize);
    test=std::make_unique<uint8_t[]>(testBlockSize);
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint8_t> dist(0, 255);
    for (size_t i=0; i<testBlockSize; ++i)
    {
        source[i]=dist(mt);
        test[i]=0;
    }
    shutdownPending.store(false);
}

bool LoopbackTester::ProcessTX()
{
    //make time mark
    //notify worker about start with time_mark
    //counter how much to send
    //read until it done, or until timed_out
    return false;
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
