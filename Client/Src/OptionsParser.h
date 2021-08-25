#ifndef OPTIONSPARSER_H
#define OPTIONSPARSER_H

#include <unordered_map>
#include <string>
#include <functional>

#include "IPAddress.h"

class OptionsParser
{
    private:
        const std::string argv0;
        const std::function<void(std::string)> usage;
        std::unordered_map<std::string,std::string> args;
    public:
        OptionsParser(int argc, char *argv[], std::function<void (std::string)> usage);
        bool CheckEmpty(bool stopOnEmpty, const std::string& errorMsg);
        bool CheckParamPresent(const std::string &param, bool stopIfNotPresent, const std::string &errorMsg);
        bool CheckIsInteger(const std::string &param, int minValue, int maxValue, bool stopOnError, const std::string &errorMsg, int base=10);
        bool CheckIsBoolean(const std::string &param, bool stopOnError, const std::string &errorMsg);
        bool CheckIsIPAddress(const std::string &param, bool stopOnError, const std::string &errorMsg);
        int GetInteger(const std::string &param, int base=10);
        bool GetBoolean(const std::string &param);
        std::string GetString(const std::string &param);
};

#endif // OPTIONSPARSER_H
