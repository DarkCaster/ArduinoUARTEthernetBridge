#ifndef IWORKER_H
#define IWORKER_H

#include "thread"
#include "mutex"
#include <memory>

class WorkerBase
{
    private:
        std::mutex workerLock;
        std::shared_ptr<std::thread> worker = std::shared_ptr<std::thread>();
        bool workerStarted = false;
    protected:
        virtual void Worker() = 0; // main worker's logic, by default will be started in a separate thread created by Startup method
        virtual void OnShutdown() = 0; // must be threadsafe and notify Worker to stop, by default it will be called from Shutdown that itself may be called from any thread
    public:
        virtual bool Startup(); //start separate thread from Worker method. Startup may be called from any thread
        virtual bool Shutdown(); //stop previously started thread, by invoking OnShutdown and awaiting Worker thread to complete
        virtual bool RequestShutdown(); //same as Shutdown, but only invoke OnShutdown and do not wait. Shutdown should be called next to collect thread
};

#endif // IWORKER_H
