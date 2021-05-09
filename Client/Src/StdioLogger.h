#ifndef STDIOLOGGER_H
#define STDIOLOGGER_H

#include "ILogger.h"
#include "LogWriter.h"

#include <mutex>
#include <atomic>
#include <string>

class StdioLogger : public ILogger
{
    private:
        const std::string name;
        const double &initialTime;
        std::atomic<unsigned int> &nameWD;
        std::mutex &extLock;
    protected:
        StdioLogger(const std::string &name, const double &initialTime, std::atomic<unsigned int> &nameWD, std::mutex &extLock);
    public:
        LogWriter Info() final;
        LogWriter Warning() final;
        LogWriter Error() final;
};

#endif // STDIOLOGGER_H
