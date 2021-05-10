#include "ILogger.h"
#include "StdioLoggerFactory.h"
#include "ImmutableStorage.h"
#include "IPAddress.h"
#include "MessageBroker.h"
#include "ShutdownHandler.h"

//#include "TCPServerListener.h"
#include "Config.h"

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstring>
#include <csignal>
#include <climits>
#include <sys/time.h>

#include <unistd.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void usage(const std::string &self)
{
    std::cerr<<"Usage: "<<self<<" [parameters]"<<std::endl;
    std::cerr<<"  mandatory parameters:"<<std::endl;
    std::cerr<<"    -p <port-num> TCP port number to listen at"<<std::endl;
    std::cerr<<"  optional parameters:"<<std::endl;
    std::cerr<<"    -l <ip-addr> IP to listen at, default: 127.0.0.1"<<std::endl;
    std::cerr<<"  experimental and optimization parameters:"<<std::endl;
    std::cerr<<"    -cmax <seconds> max total time for establishing connection, default: 20"<<std::endl;
    std::cerr<<"    -bsz <bytes> size of TCP buffer used for transferring data, default: 65536"<<std::endl;
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

    Config config;

    //parse port number
    if(args.find("-p")==args.end())
        return param_error(argv[0],"TCP port number is missing!");
    auto port=std::atoi(args["-p"].c_str());
    if(port<1||port>65535)
        return param_error(argv[0],"TCP port number is invalid!");

    //TODO: support for providing multiple ip-addresses
    //parse listen address
    ImmutableStorage<IPEndpoint> localEP(IPEndpoint(IPAddress("127.0.0.1"),static_cast<uint16_t>(port)));
    if(args.find("-l")!=args.end())
    {
        if(!IPAddress(args["-l"]).isValid||IPAddress(args["-l"]).isV6)
            return param_error(argv[0],"listen IP address is invalid!");
        localEP.Set(IPEndpoint(IPAddress(args["-l"]),static_cast<uint16_t>(port)));
    }

    config.SetServiceIntervalMS(500);
    if(args.find("-mt")!=args.end())
    {
        int cnt=std::atoi(args["-mt"].c_str());
        if(cnt<100 || cnt>10000)
            return param_error(argv[0],"workers management interval is invalid!");
        config.SetServiceIntervalMS(cnt);
    }

    //tcp buff size
    config.SetTCPBuffSz(65536);
    if(args.find("-bsz")!=args.end())
    {
        auto bsz=std::atoi(args["-bsz"].c_str());
        if(bsz<128||bsz>524288)
            return param_error(argv[0],"TCP buffer size is invalid");
        config.SetTCPBuffSz(bsz);
    }

    //linger
    config.SetLingerSec(30);
    if(args.find("-cf")!=args.end())
    {
        auto time=std::atoi(args["-cf"].c_str());
        if(time<-1||time>600)
            return param_error(argv[0],"Flush timeout value is invalid");
        config.SetLingerSec(time);
    }

    //connection timeouts
    config.SetMaxCTimeSec(20);
    if(args.find("-cmax")!=args.end())
    {
        auto time=std::atoi(args["-cmax"].c_str());
        if(time>120)
            return param_error(argv[0],"Maximum connection timeout is invalid");
        config.SetMaxCTimeSec(time);
    }

    StdioLoggerFactory logFactory;
    auto mainLogger=logFactory.CreateLogger("Main");
    auto dispLogger=logFactory.CreateLogger("Dispatcher");
    auto jobFactoryLogger=logFactory.CreateLogger("JobFactory");
    auto listenerLogger=logFactory.CreateLogger("Listener");
    auto tcpServiceLogger=logFactory.CreateLogger("TCPCommSvc");
    auto messageBrokerLogger=logFactory.CreateLogger("MSGBroker");

    mainLogger->Info()<<"Starting up";

    //configure the most essential stuff
    MessageBroker messageBroker(messageBrokerLogger);
    ShutdownHandler shutdownHandler;
    messageBroker.AddSubscriber(shutdownHandler);

    //create instances for main logic
    /*JobWorkerFactory jobWorkerFactory;
    TCPCommService tcpCommService(tcpServiceLogger,messageBroker,config);
    JobFactory jobFactory(jobFactoryLogger,config,tcpCommService,tcpCommService);
    JobDispatcher jobDispatcher(dispLogger,logFactory,jobWorkerFactory,jobFactory,messageBroker,config);
    messageBroker.AddSubscriber(jobDispatcher);
    std::vector<std::shared_ptr<TCPServerListener>> serverListeners;
    for(auto &addr:config.GetListenAddrs())
        serverListeners.push_back(std::make_shared<TCPServerListener>(listenerLogger,messageBroker,tcpCommService,config,addr));
    for(auto &listener:serverListeners)
    {
        messageBroker.AddSubscriber(*(listener));
        startupHandler.AddTarget(listener.get());
    }*/

    //create sigset_t struct with signals
    sigset_t sigset;
    sigemptyset(&sigset);
    if(sigaddset(&sigset,SIGHUP)!=0||sigaddset(&sigset,SIGTERM)!=0||sigaddset(&sigset,SIGUSR1)!=0||sigaddset(&sigset,SIGUSR2)!=0||sigaddset(&sigset,SIGPIPE)!=0||pthread_sigmask(SIG_BLOCK,&sigset,nullptr)!=0)
    {
        mainLogger->Error()<<"Failed to setup signal-handling"<<std::endl;
        return 1;
    }

    //startup for priveleged operations
    /*for(auto &listener:serverListeners)
        listener->Startup();

    //start background workers, or perform post-setup init
    jobDispatcher.Startup();
    tcpCommService.Startup();*/

    //main loop, awaiting for signal
    while(true)
    {
        auto signal=sigtimedwait(&sigset,nullptr,&sigTs);
        auto error=errno;
        if(signal<0 && error!=EAGAIN && error!=EINTR)
        {
            mainLogger->Error()<<"Error while handling incoming signal: "<<strerror(error)<<std::endl;
            break;
        }
        else if(signal>0 && signal!=SIGUSR2 && signal!=SIGINT) //SIGUSR2 triggered by shutdownhandler to unblock sigtimedwait
        {
            mainLogger->Info()<< "Pending shutdown by receiving signal: "<<signal<<"->"<<strsignal(signal)<<std::endl;
            break;
        }

        if(shutdownHandler.IsShutdownRequested())
        {
            if(shutdownHandler.GetEC()!=0)
                mainLogger->Error() << "One of background worker was failed, shuting down" << std::endl;
            else
                mainLogger->Info() << "Shuting down gracefully by request from background worker" << std::endl;
            break;
        }
    }

    //request shutdown of background workers
    /*for(auto &listener:serverListeners) //server TCP listeners will be shutdown first
        listener->RequestShutdown();

    //wait for background workers shutdown complete
    for(auto &listener:serverListeners)
        listener->Shutdown();
    tcpCommService.Shutdown();
    jobDispatcher.Shutdown();*/

    return  0;
}
