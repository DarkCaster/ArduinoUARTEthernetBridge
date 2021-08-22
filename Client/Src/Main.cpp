#include "ILogger.h"
#include "StdioLoggerFactory.h"
#include "ImmutableStorage.h"
#include "IPAddress.h"
#include "MessageBroker.h"
#include "ShutdownHandler.h"

#include "Config.h"
#include "PortConfig.h"
#include "Timer.h"
#include "PTYListener.h"
#include "TCPListener.h"
#include "TCPTransport.h"
#include "UDPTransport.h"
#include "DataProcessor.h"
#include "PortWorker.h"
#include "RemoteBufferTracker.h"

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

// example command lines:
// Arduino Pro Mini (atmega328p) with 1 uart port, data aggregation multiplier = 2, remote buffer size = 8 segments:
//  -ra ENC28J65E366.lan -tp 50000 -up 1 -rbs 8 -nm 2 -pc 1 -lp1 65001 -ps1 250000 -pm1 255 -rst1 1
// Arduino Mega 2560 (atmega 2560) with 3 uart ports, data aggregation multiplier = 4, remote buffer size = 100 segments, uart-segment size 16, local poll speed limited to 8192 usec:
//  -ra ENC28J65E366.lan -tp 50000 -up 1 -us 16 -nm 4 -rbs 100 -pmin 8192 -pmax 8192 -pc 3 -lp1 65001 -lp2 65002 -lp3 65003 -ps1 250000 -pm1 6 -ps2 250000 -pm2 6 -ps3 250000 -pm3 6 -rst1 0 -rst2 0 -rst3 0

void usage(const std::string &self)
{
    std::cerr<<"Usage: "<<self<<" [parameters]"<<std::endl;
    std::cerr<<"  mandatory parameters:"<<std::endl;
    std::cerr<<"   remote:"<<std::endl;
    std::cerr<<"    -ra <ip address, or host name> remote address to connect"<<std::endl;
    std::cerr<<"    -tp <port> remote TCP port"<<std::endl;
    std::cerr<<"    -pc <count> UART port count configured at remote side, this param is cruical for normal operation"<<std::endl;
    std::cerr<<"    -rbs <1-256> remote ring-buffer size as multiplier to hw-buffer size, used for scheduling outgoing packages - cruical for stable operation"<<std::endl;
    std::cerr<<"   local:"<<std::endl;
    std::cerr<<"    -lp{n} <port> local TCP port number OR file path for creating PTS symlink, example -lp1 40000 -lp2 40001 -lp2 /tmp/port2.sock"<<std::endl;
    std::cerr<<"    -ps{n} <speed in bits-per-second> remote uart port speeds"<<std::endl;
    std::cerr<<"    -pm{n} <mode number> remote uart port modes, '6' equals to SERIAL_8N1 arduino-define, '255' - loopback mode for testing"<<std::endl;
    std::cerr<<"  optional parameters:"<<std::endl;
    std::cerr<<"    -up <0,1> 1 - enable use of less reliable UDP transport with lower latency and jitter, default: 0 - disabled"<<std::endl;
    std::cerr<<"    -rst{n} <0,1> perform reset on connection, default: 0 - do not perform reset"<<std::endl;
    std::cerr<<"    -la <ip-addr> local IP to listen for TCP channels enabled by -lp{n} option, default: 127.0.0.1"<<std::endl;
    std::cerr<<"    -us <1-65535> uart-buffer/segment size in bytes used at server, cruical for timings and network payload size calculation, default: 64"<<std::endl;
    std::cerr<<"    -nm <1-10> multiplier to uart-buffer size, used to aggregate data before sending it over network, cruical for operation, default: 1"<<std::endl;
    std::cerr<<"    -pmin <time, us> minimal local port poll interval, default: 4096 usec (with hw uart-buffer size of 64bytes - equals to 125kbit/s throughput per port)"<<std::endl;
    std::cerr<<"    -pmax <time, us> maximum local port poll interval, default: 16384 usec"<<std::endl;
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

int64_t calculate_poll_interval(const int portSpeed, const int uartBuffSize, const int min, const int max)
{
    const auto interval=static_cast<int64_t>((1000000.0/static_cast<double>(portSpeed))*8.0*static_cast<double>(uartBuffSize))/2;
    return interval<min?min:interval>max?max:interval;
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

    if(args.find("-rbs")==args.end())
        return param_error(argv[0],"remote ring-buffer size is missing");
    config.SetRingBuffSegCount(std::atoi(args["-rbs"].c_str()));
    if(config.GetRingBuffSegCount()<1||config.GetRingBuffSegCount()>256)
        return param_error(argv[0],"remote ring-buffer size is invalid!");

    if(args.find("-ra")==args.end())
        return param_error(argv[0],"remote address or DNS-name is missing");
    config.SetRemoteAddr(args["-ra"]);

    size_t portCount=0;
    if(args.find("-pc")!=args.end())
    {
        auto pc=std::atoi(args["-pc"].c_str());
        if(pc<1||pc>32)
            return param_error(argv[0],"port count value is invalid");
        portCount=static_cast<size_t>(pc);
        config.SetPortCount(pc);
    }
    else
        return param_error(argv[0],"port count must be provided");

    if(args.find("-tp")!=args.end())
    {
        auto tp=std::atoi(args["-tp"].c_str());
        if(tp<1||tp>65535)
            return param_error(argv[0],"TCP port is invalid");
        config.SetTCPPort(static_cast<uint16_t>(tp));
    }
    else
        return param_error(argv[0],"TCP port must be provided");

    config.SetUDPEnabled(false);
    if(args.find("-up")!=args.end())
        config.SetUDPEnabled(std::atoi(args["-up"].c_str())==1);

    //parse local ports numbers
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

    //parse uart speeds
    std::vector<int> uartSpeeds;
    while(args.find("-ps"+std::to_string(uartSpeeds.size()+1))!=args.end())
    {
        auto port=std::atoi(args["-ps"+std::to_string(uartSpeeds.size()+1)].c_str());
        if(port<1)
            return param_error(argv[0],"uart speed is invalid!");
        uartSpeeds.push_back(port);
    }
    if(uartSpeeds.size()<portCount)
        return param_error(argv[0],"provided uart speeds count must be equal to remote ports count");

    //parse uart modes
    std::vector<int> uartModes;
    bool timingProfilingEnabled=false;
    while(args.find("-pm"+std::to_string(uartModes.size()+1))!=args.end())
    {
        auto mode=std::atoi(args["-pm"+std::to_string(uartModes.size()+1)].c_str());
        if(mode<0)
            return param_error(argv[0],"uart mode is invalid!");
        if(mode==255)
            timingProfilingEnabled=true;
        uartModes.push_back(mode);
    }
    if(uartModes.size()<portCount)
        return param_error(argv[0],"provided uart modes count must be equal to remote ports count");

    //parse reset-needed flag
    std::vector<bool> rstFlags;
    while(args.find("-rst"+std::to_string(rstFlags.size()+1))!=args.end())
        rstFlags.push_back(std::atoi(args["-rst"+std::to_string(rstFlags.size()+1)].c_str())==1);
    if(rstFlags.size()<portCount)
        return param_error(argv[0],"provided reset-flags count must be equal to remote ports count");

    ImmutableStorage<IPAddress> localAddr(IPAddress("127.0.0.1"));
    if(args.find("-la")!=args.end())
    {
        if(!IPAddress(args["-la"]).isValid||IPAddress(args["-la"]).isV6)
            return param_error(argv[0],"listen IP address is invalid!");
        localAddr.Set(IPAddress(args["-la"]));
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

    config.SetHwUARTSz(64);
    if(args.find("-us")!=args.end())
    {
        auto bsz=std::atoi(args["-us"].c_str());
        if(bsz<1||bsz>65535)
            return param_error(argv[0],"HW UART buffer size is invalid");
        config.SetHwUARTSz(bsz);
    }

    config.SetNwMult(1);
    if(args.find("-nm")!=args.end())
    {
        auto nsz=std::atoi(args["-nm"].c_str());
        if(nsz<1||nsz>10)
            return param_error(argv[0],"Package aggregation multiplier is invalid");
        config.SetNwMult(nsz);
    }

    //management interval
    config.SetServiceIntervalMS(500);
    if(args.find("-mt")!=args.end())
    {
        int cnt=std::atoi(args["-mt"].c_str());
        if(cnt<100 || cnt>10000)
            return param_error(argv[0],"workers management interval is invalid!");
        config.SetServiceIntervalMS(cnt);
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

    int pmin=4096;
    if(args.find("-pmin")!=args.end())
    {
        auto time=std::atoi(args["-pmin"].c_str());
        if(time<1||time>1000000)
            return param_error(argv[0],"Min poll time is invalid");
        pmin=time;
    }

    int pmax=16384;
    if(args.find("-pmax")!=args.end())
    {
        auto time=std::atoi(args["-pmax"].c_str());
        if(time<1||time>1000000||pmax<pmin)
            return param_error(argv[0],"Max poll time is invalid");
        pmax=time;
    }

    //check network payload size, currently it cannot exceed 255 bytes
    if(config.GetNetworkPayloadSz()>255)
        return param_error(argv[0],"Provided UART buffer size or it's multiplier is too big, total payload size (uart size * mult) exceed 255 bytes");

    //create remote-config objects
    std::vector<PortConfig> remoteConfigs;
    for(size_t i=0; i<portCount; ++i)
        remoteConfigs.push_back(
                    PortConfig(static_cast<uint32_t>(uartSpeeds[i]),
                               static_cast<SerialMode>(uartModes[i]),
                               rstFlags[i],
                               //calculate_poll_interval(uartSpeeds[i],config.GetNetworkPayloadSz()),
                               IPEndpoint(localAddr.Get(),static_cast<uint16_t>(localPorts[i])),
                               localFiles[i],i));

    StdioLoggerFactory logFactory;
    auto mainLogger=logFactory.CreateLogger("Main");
    auto messageBrokerLogger=logFactory.CreateLogger("MSGBroker");
    auto tcpTransportLogger=logFactory.CreateLogger("TCPTransport");
    auto udpTransportLogger=logFactory.CreateLogger("UDPTransport");
    auto timerLogger=logFactory.CreateLogger("PollTimer");
    auto dpLogger=logFactory.CreateLogger("DataProcessor");

    //configure the most essential stuff
    MessageBroker messageBroker(messageBrokerLogger);

    //shutdown handler
    ShutdownHandler shutdownHandler;
    messageBroker.AddSubscriber(shutdownHandler);

    //create instances for main logic

    //TCP transport
    TCPTransport tcpTransport(tcpTransportLogger,messageBroker,config);
    messageBroker.AddSubscriber(tcpTransport);

    //UDP transport
    UDPTransport udpTransport(udpTransportLogger,messageBroker,config);
    messageBroker.AddSubscriber(udpTransport);

    //Port polling timer
    int64_t minPollTime=INT64_MAX;
    for(size_t i=0;i<portCount;++i)
    {
        auto testTime=calculate_poll_interval(uartSpeeds[i],config.GetNetworkPayloadSz(),pmin,pmax);
        if(testTime<minPollTime)
            minPollTime=testTime;
    }
    Timer pollTimer(timerLogger,messageBroker,config,minPollTime,timingProfilingEnabled);
    messageBroker.AddSubscriber(pollTimer);

    std::vector<std::shared_ptr<TCPListener>> tcpListeners;
    for(size_t i=0;i<remoteConfigs.size();++i)
    {
        if(remoteConfigs[i].listener.port==0)
            continue;
        auto logger=logFactory.CreateLogger("TCPListener:"+std::to_string(i));
        tcpListeners.push_back(std::make_shared<TCPListener>(logger,messageBroker,config,remoteConfigs[i]));
    }

    std::vector<std::shared_ptr<PTYListener>> ptyListeners;
    for(size_t i=0;i<remoteConfigs.size();++i)
    {
        if(remoteConfigs[i].ptsListener.empty())
            continue;
        auto logger=logFactory.CreateLogger("PTYListener:"+std::to_string(i));
        ptyListeners.push_back(std::make_shared<PTYListener>(logger,messageBroker,config,remoteConfigs[i]));
    }

    std::vector<std::shared_ptr<PortWorker>> portWorkers;
    std::vector<std::shared_ptr<RemoteBufferTracker>> buffTrackers;
    for(size_t i=0;i<remoteConfigs.size();++i)
    {
        auto rTrackerLogger=logFactory.CreateLogger("BuffTracker:"+std::to_string(i));
        auto rTracker=std::make_shared<RemoteBufferTracker>(rTrackerLogger,config,config.GetHwUARTSz()*config.GetRingBuffSegCount());
        auto portLogger=logFactory.CreateLogger("PortWorker:"+std::to_string(i));
        auto portWorker=std::make_shared<PortWorker>(portLogger,messageBroker,config,remoteConfigs[i],*(rTracker));
        messageBroker.AddSubscriber(portWorker);
        portWorkers.push_back(portWorker);
        buffTrackers.push_back(rTracker);
    }

    //Data processor
    DataProcessor dataProcessor(dpLogger,messageBroker,config,portWorkers);
    messageBroker.AddSubscriber(dataProcessor);

    //create sigset_t struct with signals
    sigset_t sigset;
    sigemptyset(&sigset);
    if(sigaddset(&sigset,SIGHUP)!=0||sigaddset(&sigset,SIGTERM)!=0||sigaddset(&sigset,SIGUSR1)!=0||sigaddset(&sigset,SIGUSR2)!=0||sigaddset(&sigset,SIGPIPE)!=0||pthread_sigmask(SIG_BLOCK,&sigset,nullptr)!=0)
    {
        mainLogger->Error()<<"Failed to setup signal-handling"<<std::endl;
        return 1;
    }

    //startup
    for(auto &portWorker:portWorkers)
        portWorker->Startup();
    tcpTransport.Startup();
    udpTransport.Startup();
    pollTimer.Startup();
    for(auto &listener:tcpListeners)
        listener->Startup();
    for(auto &listener:ptyListeners)
        listener->Startup();

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
    for(auto &listener:tcpListeners) //server TCP listeners will be shutdown first
        listener->RequestShutdown();
    for(auto &listener:ptyListeners)
        listener->RequestShutdown();
    pollTimer.RequestShutdown();
    udpTransport.RequestShutdown();
    tcpTransport.RequestShutdown();
    for(auto &portWorker:portWorkers)
        portWorker->RequestShutdown();

    //wait for background workers shutdown complete
    for(auto &listener:tcpListeners)
        listener->Shutdown();
    for(auto &listener:ptyListeners)
        listener->Shutdown();
    pollTimer.Shutdown();
    udpTransport.Shutdown();
    tcpTransport.Shutdown();
    for(auto &portWorker:portWorkers)
        portWorker->Shutdown();

    mainLogger->Info()<<"Clean shutdown"<<std::endl;
    return 0;
}
