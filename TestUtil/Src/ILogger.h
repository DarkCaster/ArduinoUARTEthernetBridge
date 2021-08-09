#ifndef ILOGGER_H
#define ILOGGER_H

#include "LogWriter.h"

class ILogger
{
    public:
        virtual LogWriter Info() = 0;
        virtual LogWriter Warning() = 0;
        virtual LogWriter Error() = 0;
};

#endif // ILOGGER_H
