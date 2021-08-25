#include "OptionsParser.h"

#include <iostream>
#include <utility>

OptionsParser::OptionsParser(int argc, char *argv[], std::function<void (std::string)> _usage):
    argv0(argv[0]),
    usage(std::move(_usage))
{
    bool isArgValue=false;
    std::string curArg;

    for(auto i=1;i<argc;++i)
    {
        if(isArgValue)
        {
            args[argv[i-1]]=argv[i];
            isArgValue=false;
            continue;
        }
        else if(std::string(argv[i]).length()>1 && (std::string(argv[i]).front()=='-' || std::string(argv[i]).front()=='/'))
        {
            curArg=std::string(argv[i]).substr(1,std::string(argv[i]).length()-1);
            isArgValue=true;
        }
        else
        {
            std::cerr<<"Invalid cmdline argument: "<<argv[i]<<std::endl;
            usage(argv0);
            exit(1);
        }
    }
}

bool OptionsParser::CheckEmpty(bool stopOnEmpty, const std::string& errorMsg)
{
    if(args.empty())
    {
        if(!errorMsg.empty())
            std::cerr<<errorMsg<<std::endl;
        if(stopOnEmpty)
        {
            usage(argv0);
            exit(1);
        }
        return true;
    }
    return false;
}

bool OptionsParser::CheckParamPresent(const std::string& param, bool stopIfNotPresent, const std::string& errorMsg)
{
    auto present=args.find(param)!=args.end() && !args[param].empty();
    if(!present)
    {
        if(!errorMsg.empty())
            std::cerr<<errorMsg<<std::endl;
        if(stopIfNotPresent)
        {
            usage(argv0);
            exit(1);
        }
        return false;
    }
    return true;
}

bool OptionsParser::CheckIsInteger(const std::string& param, int minValue, int maxValue, bool stopOnError, const std::string& errorMsg, int base)
{
    bool error=args.find(param)==args.end();
    int value=minValue;
    try
    {
        if(!error)
        {
            value=std::stoi(args[param],nullptr,base);
            if(value<minValue||value>maxValue)
                error=true;
        }
    }
    catch (const std::invalid_argument& /*ex*/) { error=true; }
    catch (const std::out_of_range& /*ex*/) { error=true; }
    if(error)
    {
        if(!errorMsg.empty())
            std::cerr<<errorMsg<<std::endl;
        if(stopOnError)
        {
            usage(argv0);
            exit(1);
        }
        return false;
    }
    return true;
}

bool OptionsParser::CheckIsBoolean(const std::string& param, bool stopOnError, const std::string& errorMsg)
{
    if(args.find(param)==args.end()||
            !(args[param]=="1"||args[param]=="0"||
              args[param]=="y"||args[param]=="n"||
              args[param]=="yes"||args[param]=="no"||
              args[param]=="t"||args[param]=="f"||
              args[param]=="true"||args[param]=="false"))
    {
        if(!errorMsg.empty())
            std::cerr<<errorMsg<<std::endl;
        if(stopOnError)
        {
            usage(argv0);
            exit(1);
        }
        return false;
    }
    return true;
}

bool OptionsParser::CheckIsIPAddress(const std::string& param, bool stopOnError, const std::string& errorMsg)
{
    if(args.find(param)==args.end()||!IPAddress(args[param]).isValid)
    {
        if(!errorMsg.empty())
            std::cerr<<errorMsg<<std::endl;
        if(stopOnError)
        {
            usage(argv0);
            exit(1);
        }
        return false;
    }
    return true;
}

int OptionsParser::GetInteger(const std::string& param, int base)
{
    if(args.find(param)!=args.end())
        return std::stoi(args[param],nullptr,base);
    return 0;
}

bool OptionsParser::GetBoolean(const std::string& param)
{
    if(args.find(param)==args.end()||!(args[param]=="1"||args[param]=="y"||args[param]=="yes"||args[param]=="t"||args[param]=="true"))
        return false;
    return true;
}

std::string OptionsParser::GetString(const std::string& param)
{
    if(args.find(param)!=args.end())
        return args[param];
    return std::string();
}
