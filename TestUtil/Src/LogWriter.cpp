#include "LogWriter.h"

#include <iomanip>

LogWriter::LogWriter(std::ostream &_output, std::mutex &_extLock, const double &time, const std::string &type, const int& typeWD, const std::string &name, const int& nameWD):
    output(_output),
    extLock(_extLock)
{
    extLock.lock();
    endl=false;
    //write header
    output<<"["<<std::fixed<<std::setprecision(2)<<std::setfill('0')<<std::setw(6)<<time<<\
    "|"<<std::setw(typeWD)<<std::setfill(' ')<<type<<"|"<<std::setw(nameWD)<<std::setfill(' ')<<name<<"]: ";
    //std::cout<<"!!!create!!!"<<std::endl;
}

LogWriter::~LogWriter()
{
    if(!endl)
        output<<std::endl;
    extLock.unlock();
}

LogWriter& LogWriter::operator<<(std::ostream& (*manip)(std::ostream&))
{
   if(manip==static_cast<std::ostream&(*)(std::ostream&)>(std::endl))
       endl=true;
    output<<manip;
    return *this;
}
