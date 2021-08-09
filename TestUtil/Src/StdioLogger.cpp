#include "StdioLogger.h"

#include "LogWriter.h"

StdioLogger::StdioLogger(const std::string &_name, const double &_initialTime, std::atomic<unsigned int> &_nameWD, std::mutex &_extLock):
    name(_name),
    initialTime(_initialTime),
    nameWD(_nameWD),
    extLock(_extLock)
{
}

static double GetTimeMark()
{
    timespec time={};
    clock_gettime(CLOCK_MONOTONIC,&time);
    return static_cast<double>(time.tv_sec)+static_cast<double>(time.tv_nsec)/1000000000.;
}

LogWriter StdioLogger::Info()
{
    return LogWriter(std::cout,extLock,GetTimeMark()-initialTime,"INFO",4,name,nameWD.load());
}

LogWriter StdioLogger::Warning()
{
    return LogWriter(std::cout,extLock,GetTimeMark()-initialTime,"WARN",4,name,nameWD.load());
}

LogWriter StdioLogger::Error()
{
    return LogWriter(std::cerr,extLock,GetTimeMark()-initialTime,"ERR",4,name,nameWD.load());
}
