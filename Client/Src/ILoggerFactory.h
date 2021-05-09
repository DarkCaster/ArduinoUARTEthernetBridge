#ifndef ILOGGER_FACTORY_H
#define ILOGGER_FACTORY_H

#include "ILogger.h"
#include "memory"

class ILoggerFactory
{
    public:
        virtual std::shared_ptr<ILogger> CreateLogger(const std::string &name) = 0;
};

#endif // ILOGGER_FACTORY_H

