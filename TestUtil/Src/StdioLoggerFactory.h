#ifndef STDIOLOGGERFACTORY_H
#define STDIOLOGGERFACTORY_H

#include "ILogger.h"
#include "ILoggerFactory.h"

#include <mutex>
#include <atomic>
#include <memory>

class StdioLoggerFactory final : public ILoggerFactory
{
    private:
        std::atomic<unsigned int> maxNameWD;
        std::mutex stdioLock;
        double creationTime;
    public:
        StdioLoggerFactory();
        //ILoggerFactory
        std::shared_ptr<ILogger> CreateLogger(const std::string &name) final;
};

#endif // STDIOLOGGERFACTORY_H
