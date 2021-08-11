#include "LoopbackTester.h"

#include <random>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <netdb.h>

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
    dataToWrite=testBlockSize;
}

bool LoopbackTester::ProcessTX()
{
    //make time mark
    if(!testStarted)
    {
        std::lock_guard<std::mutex> triggerGuard(startTriggerLock);
        startPoint=std::chrono::steady_clock::now();
        testStarted=true;
    }
    //notify worker about start with time_mark
    startTrigger.notify_one();

    auto dw=send(target.fd,source.get()+testBlockSize-dataToWrite,dataToWrite,0);
    if(dw<=0)
    {
        auto error=errno;
        if(error==EAGAIN || error==EWOULDBLOCK)
            return true;
        if(error==EINTR)
            return false;
        logger->Error()<<"Send failed: "<<strerror(error);
        target.Dispose();
        return false;
    }
    dataToWrite-=static_cast<size_t>(dw);
    if(dataToWrite>0)
        return true;

    //read until it done, or until timed_out
    return false;
}

void LoopbackTester::Worker()
{
    logger->Info()<<"Reader waiting to start";

    std::unique_lock<std::mutex> lock(startTriggerLock);
    while(!testStarted && !shutdownPending.load())
        startTrigger.wait(lock);
    auto startTime=startPoint;
    lock.unlock();

    logger->Info()<<"Reader started";
    //try to raed at least 1-st byte of data, and measure the time
    while(!shutdownPending.load())
    {
        //read package
        auto dr=recv(target.fd,test.get(),1,MSG_WAITALL);
        if(dr<=0)
        {
            auto error=errno;
            if(error==EINTR || error==EAGAIN || error==EWOULDBLOCK)
                continue;
            //socket was closed or errored, close connection from our side and stop reading
            logger->Error()<<"TCP recv failed: "<<strerror(error);
            target.Dispose();
            return;
        }
    }

    auto firstByteTime=std::chrono::steady_clock::now();
    auto latency=std::chrono::duration_cast<std::chrono::milliseconds>(firstByteTime-startTime);
    logger->Info()<<"Loopback latency: "<<latency.count()<<" ms";

    if(shutdownPending.load())
    {
        logger->Warning()<<"Reader stopped by external shutdown request!";
        return;
    }

    auto readLeft=testBlockSize-1;
    while(!shutdownPending.load())
    {
        auto dr=recv(target.fd,test.get()+testBlockSize-readLeft,readLeft,0);
        if(dr<=0)
        {
            auto error=errno;
            if(error==EINTR || error==EAGAIN || error==EWOULDBLOCK)
                continue;
            //socket was closed or errored, close connection from our side and stop reading
            logger->Error()<<"TCP recv failed: "<<strerror(error);
            target.Dispose();
            return;
        }
        readLeft-=static_cast<size_t>(dr);
    }

    auto endTime=std::chrono::steady_clock::now();
    auto fullTime=std::chrono::duration_cast<std::chrono::milliseconds>(endTime-startTime);
    logger->Info()<<"Full time to send and receive package: "<<fullTime.count()<<" ms";

    //try to read requested amount of data, exit if this takes too long, calculate latency, compare data,
    if(shutdownPending.load())
    {
        logger->Warning()<<"Reader stopped by external shutdown request!";
        logger->Warning()<<"Received bytes so far: "<<testBlockSize-readLeft;
    }
}

void LoopbackTester::OnShutdown()
{
    shutdownPending.store(true);
}
