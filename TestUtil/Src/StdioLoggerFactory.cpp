#include "StdioLoggerFactory.h"

#include "StdioLogger.h"

class FinalStdioLogger final : public StdioLogger
{
    public:
        FinalStdioLogger(const std::string &_name, const double &_time, std::atomic<unsigned int> &_nameWD, std::mutex &_extLock): StdioLogger(_name, _time, _nameWD, _extLock) {};
};

StdioLoggerFactory::StdioLoggerFactory()
{
    maxNameWD.store(1);
    timespec time={};
    clock_gettime(CLOCK_MONOTONIC,&time);
    creationTime=static_cast<double>(time.tv_sec)+static_cast<double>(time.tv_nsec)/1000000000.;
}

std::shared_ptr<ILogger> StdioLoggerFactory::CreateLogger(const std::string& name)
{
    if(name.length()>maxNameWD.load())
        maxNameWD.store(static_cast<unsigned int>(name.length()));
    return std::make_shared<FinalStdioLogger>(name,creationTime,maxNameWD,stdioLock);
}
