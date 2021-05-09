#ifndef LOGWRITER_H
#define LOGWRITER_H

#include <iostream>
#include <string>
#include <mutex>

class LogWriter
{
    private:
        std::ostream &output;
        std::mutex &extLock;
        bool endl;
    public:
        //constructor aquires shared extLock and write formated header to output from remaining paramters
        LogWriter(std::ostream& output, std::mutex &extLock, const double& time, const std::string& type, const int& typeWD, const std::string& name, const int& nameWD);
        //destructor releases shared extLock allowing other LogWriters to run
        ~LogWriter();
        //other constructors
        LogWriter (LogWriter&) = delete;
        LogWriter (LogWriter&&) = default;
        //<< override for manipulators, detects use of excess std::endl
        LogWriter& operator<<(std::ostream& (*manip)(std::ostream&));
        //main << override for all other stuff
        template <class T> LogWriter& operator<<(T&& x) { endl=false; output<<std::forward<T>(x); return *this; }
};

#endif // LOGWRITER_H
