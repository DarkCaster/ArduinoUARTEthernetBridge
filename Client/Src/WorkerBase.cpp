#include "WorkerBase.h"

bool WorkerBase::Startup()
{
    const std::lock_guard<std::mutex> guard(workerLock);
    if(workerStarted)
        return false;
    worker=std::make_shared<std::thread>(&WorkerBase::Worker,this);
    workerStarted=true;
    return true;
}

bool WorkerBase::Shutdown()
{
    const std::lock_guard<std::mutex> guard(workerLock);
    if(!workerStarted)
        return false;
    OnShutdown();
    worker->join();
    workerStarted=false;
    return true;
}

bool WorkerBase::RequestShutdown()
{
    const std::lock_guard<std::mutex> guard(workerLock);
    if(!workerStarted)
        return false;
    OnShutdown();
    return true;
}
