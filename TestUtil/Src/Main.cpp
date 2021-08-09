#include <iostream>
#include <string>
#include <csignal>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>
#include <sys/time.h>

void usage(const std::string &self)
{
    std::cerr<<"Usage: "<<self<<" [parameters]"<<std::endl;
    std::cerr<<"  mandatory parameters:"<<std::endl;
    std::cerr<<"    -pc <count> UART port count"<<std::endl;
    std::cerr<<"    -lp{n} <port> TCP ports UARTClient util is listening at"<<std::endl;
    std::cerr<<"  experimental and optimization parameters:"<<std::endl;
    std::cerr<<"    -bsz <bytes> size of TCP buffer used for transferring data, default: 64k"<<std::endl;
    std::cerr<<"    -mt <time, ms> management interval used for some internal routines, default: 500"<<std::endl;
    std::cerr<<"    -cf <seconds> timeout for flushing data when closing sockets, -1 to disable, 0 - close without flushing, default: 30"<<std::endl;
}

int param_error(const std::string &self, const std::string &message)
{
    std::cerr<<message<<std::endl;
    usage(self);
    return 1;
}

int main (int argc, char *argv[])
{
    //timeout for main thread waiting for external signals
    const timespec sigTs={2,0};

    std::unordered_map<std::string,std::string> args;
    bool isArgValue=false;
    for(auto i=1;i<argc;++i)
    {
        if(isArgValue)
        {
            args[argv[i-1]]=argv[i];
            isArgValue=false;
            continue;
        }
        if(std::string(argv[i]).length()<2||std::string(argv[i]).front()!='-')
        {
            std::cerr<<"Invalid cmdline argument: "<<argv[i]<<std::endl;
            usage(argv[0]);
            return 1;
        }
        isArgValue=true;
    }

    if(args.empty())
        return param_error(argv[0],"Mandatory parameters are missing!");

    size_t portCount=0;
    if(args.find("-pc")!=args.end())
    {
        auto pc=std::atoi(args["-pc"].c_str());
        if(pc<1||pc>32)
            return param_error(argv[0],"port count value is invalid");
        portCount=static_cast<size_t>(pc);
    }
    else
        return param_error(argv[0],"port count must be provided");

    std::vector<int> localPorts;
    std::vector<std::string> localFiles;
    while(args.find("-lp"+std::to_string(localPorts.size()+1))!=args.end())
    {
        auto sockFile=args["-lp"+std::to_string(localPorts.size()+1)];
        auto port=std::atoi(sockFile.c_str());
        if(port<1||port>65535)
            port=0;
        else
            sockFile="";
        localPorts.push_back(port);
        localFiles.push_back(sockFile);
    }
    if(localPorts.size()<portCount)
        return param_error(argv[0],"local ports count must be equals to remote ports count");

    //tcp buff size
    int tcpBuffSize=65536;
    if(args.find("-bsz")!=args.end())
    {
        auto bsz=std::atoi(args["-bsz"].c_str());
        if(bsz<128||bsz>524288)
            return param_error(argv[0],"TCP buffer size is invalid");
        tcpBuffSize=bsz;
    }

    //management interval
    int mgInterval=500;
    if(args.find("-mt")!=args.end())
    {
        int cnt=std::atoi(args["-mt"].c_str());
        if(cnt<100 || cnt>10000)
            return param_error(argv[0],"workers management interval is invalid!");
        mgInterval=cnt;
    }

    //linger
     int linger=30;
    if(args.find("-cf")!=args.end())
    {
        auto time=std::atoi(args["-cf"].c_str());
        if(time<-1||time>600)
            return param_error(argv[0],"Flush timeout value is invalid");
        linger=time;
    }

    return 0;
}
