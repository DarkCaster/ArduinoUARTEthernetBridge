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
    testStarted=false;
}

bool LoopbackTester::ProcessTX()
{
    //make time mark
    {
        std::lock_guard<std::mutex> triggerGuard(startTriggerLock);
        startPoint=std::chrono::steady_clock::now();
        testStarted=false;
    }
    //notify worker about start with time_mark
    startTrigger.notify_one();

    //counter how much to send
    //read until it done, or until timed_out
    return false;
}

void LoopbackTester::Worker()
{
    logger->Info()<<"Reader started";
    while(!shutdownPending.load())
    {
        std::unique_lock<std::mutex> lock(startTriggerLock);
        while(!testStarted && !shutdownPending.load())
            startTrigger.wait(lock);
        auto start=startPoint;
        lock.unlock();

        //try to read requested amount of data, exit if this takes too long, calculate latency, compare data,

    }
    if(shutdownPending.load())
        logger->Warning()<<"Reader stopped by external shutdown request!";
}

void LoopbackTester::OnShutdown()
{
    shutdownPending.store(true);
}
