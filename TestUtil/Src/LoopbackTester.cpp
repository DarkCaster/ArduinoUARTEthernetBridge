#include "LoopbackTester.h"

#include <chrono>
#include <random>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <netdb.h>

LoopbackTester::LoopbackTester(std::shared_ptr<ILogger>& _logger, Connection& _target, size_t _testBlockSize, uint64_t _timeoutMS, uint64_t _warmupMS):
    logger(_logger),
    target(_target),
    testBlockSize(_testBlockSize),
    timeoutMS(_timeoutMS),
    warmupMS(_warmupMS)
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
        //pause
        logger->Info()<<"Warming up";
        std::this_thread::sleep_for(std::chrono::milliseconds(warmupMS));
        logger->Info()<<"Starting to send data";
        std::lock_guard<std::mutex> triggerGuard(startTriggerLock);
        startPoint=std::chrono::steady_clock::now();
        testStarted=true;
        startTrigger.notify_one();
    }
    //notify worker about start with time_mark


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
    logger->Info()<<"Send complete";
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
        else
            break;
    }

    if(shutdownPending.load())
    {
        logger->Warning()<<"Reader stopped by external shutdown request!";
        return;
    }

    auto firstByteTime=std::chrono::steady_clock::now();
    auto latency=std::chrono::duration_cast<std::chrono::milliseconds>(firstByteTime-startTime);
    logger->Info()<<"Loopback latency: "<<latency.count()<<" ms";

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
        if(readLeft<1)
            break;
    }

    //try to read requested amount of data, exit if this takes too long, calculate latency, compare data,
    if(shutdownPending.load())
    {
        logger->Warning()<<"Reader stopped by external shutdown request!";
        logger->Warning()<<"Received bytes so far: "<<testBlockSize-readLeft;
        return;
    }

    auto endTime=std::chrono::steady_clock::now();
    auto fullTime=std::chrono::duration_cast<std::chrono::milliseconds>(endTime-startTime);
    logger->Info()<<"Full time to send and receive package: "<<fullTime.count()<<" ms";
    auto recvTime=std::chrono::duration_cast<std::chrono::milliseconds>(endTime-firstByteTime);
    logger->Info()<<"Time between receiving first and last byte of incoming package: "<<recvTime.count()<<" ms";
    auto speed=static_cast<double>(testBlockSize)/static_cast<double>(recvTime.count())*8.0*1000.0;
    logger->Info()<<"Calculated speed: "<<speed<<" bits/sec";
    logger->Info()<<"Test complete!";
}

void LoopbackTester::OnShutdown()
{
    shutdownPending.store(true);
}
