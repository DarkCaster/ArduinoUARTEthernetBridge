#include "OptionsParser.h"
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
//  -ra ENC28J65E366.lan -tp 50000 -up 1 -us 32 -nm 2 -rbs 50 -pmin 8600 -pmax 8600 -pc 3 -lp1 65001 -lp2 65002 -lp3 65003 -ps1 250000 -pm1 6 -ps2 250000 -pm2 6 -ps3 250000 -pm3 6 -rst1 0 -rst2 0 -rst3 0


//TODO:
//implement providing remote and local poll interval in useconds, set remote poll interval in the first package sent
//remove -nm multiplier from client, provide full remote io-size with -us param

void usage(const std::string &self)
{
    std::cerr<<"Usage: "<<self<<" [parameters]"<<std::endl;
    std::cerr<<"  mandatory parameters, must match the values hardcoded at server firmware:"<<std::endl;
    std::cerr<<"    -ra <ip address, or host name> remote address to connect"<<std::endl;
    std::cerr<<"    -tp <port> remote TCP port"<<std::endl;
    std::cerr<<"    -pc <count> UART port count configured at remote side, required to match for operation"<<std::endl;
    std::cerr<<"    -pls <bytes> network payload size for single port in bytes, required to match for operation"<<std::endl;
    std::cerr<<"    -rbs <bytes> remote ring-buffer size for incoming data, should match to prevent data loss"<<std::endl;


    std::cerr<<"   local:"<<std::endl;
    std::cerr<<"    -lp{n} <port> local TCP port number OR file path for creating PTS symlink, example -lp1 40000 -lp2 40001 -lp2 /tmp/port2.sock"<<std::endl;
    std::cerr<<"    -ps{n} <speed in bits-per-second> remote uart port speeds"<<std::endl;
    std::cerr<<"    -pm{n} <mode number> remote uart port modes, '6' equals to SERIAL_8N1 arduino-define, '255' - loopback mode for testing"<<std::endl;
    std::cerr<<"  optional parameters:"<<std::endl;
    std::cerr<<"    -up <0,1> 1 - enable use of less reliable UDP transport with lower latency and jitter, default: 0 - disabled"<<std::endl;
    std::cerr<<"    -rst{n} <0,1> perform reset on connection, default: 0 - do not perform reset"<<std::endl;
    std::cerr<<"    -la <ip-addr> local IP to listen for TCP channels enabled by -lp{n} option, default: 127.0.0.1"<<std::endl;

    std::cerr<<"    -pmin <time, us> minimal local port poll interval, default: 4096 usec (with hw uart-buffer size of 64bytes - equals to 125kbit/s throughput per port)"<<std::endl;
    std::cerr<<"    -pmax <time, us> maximum local port poll interval, default: 16384 usec"<<std::endl;

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
    //parse command-line options, fillup config
    OptionsParser options(argc,argv,usage);
    options.CheckEmpty(true,"Mandatory parameters are missing!");

    Config config;

    options.CheckParamPresent("rbs",true,"remote ring-buffer size is missing");
    options.CheckIsInteger("rbs",1,65535,true,"remote ring-buffer size is invalid!");
    config.SetRemoteRingBuffSize(options.GetInteger("rbs"));

    options.CheckParamPresent("ra",true,"remote address or DNS-name is missing");
    config.SetRemoteAddr(options.GetString("ra"));

    options.CheckParamPresent("pc",true,"port count must be provided");
    options.CheckIsInteger("pc",1,32,true,"port count value is invalid");
    config.SetPortCount(options.GetInteger("pc"));

    options.CheckParamPresent("tp",true,"TCP port must be provided");
    options.CheckIsInteger("tp",1,65535,true,"TCP port is invalid");
    config.SetTCPPort(static_cast<uint16_t>(options.GetInteger("tp")));

    if(!options.CheckParamPresent("up",false,""))
        config.SetUDPEnabled(false);
    else
    {
        options.CheckIsBoolean("up",true,"UDP mode parameter is invalid");
        config.SetUDPEnabled(options.GetBoolean("up"));
    }

    options.CheckParamPresent("pls",true,"Network payload size must be provided");
    options.CheckIsInteger("pls",1,255,true,"Network payload size must be provided");
    config.SetPortPayloadSize(options.GetInteger("pls"));

    ImmutableStorage<IPAddress> localAddr(IPAddress("127.0.0.1"));
    if(options.CheckParamPresent("la",false,""))
    {
        options.CheckIsIPAddress("la",true,"listen IP address is invalid!");
        localAddr.Set(IPAddress(options.GetString("la")));
    }

    int pmin=4096;
    if(options.CheckParamPresent("pmin",false,""))
    {
        options.CheckIsInteger("pmin",256,1000000,true,"Min poll time is invalid");
        pmin=options.GetInteger("pmin");
    }

    int pmax=16384;
    if(options.CheckParamPresent("pmax",false,""))
    {
        options.CheckIsInteger("pmax",pmin,1000000,true,"Max poll time is invalid");
        pmin=options.GetInteger("pmax");
    }

    std::vector<int> localPorts;
    std::vector<std::string> localFiles;
    std::vector<int> uartSpeeds;
    std::vector<int> uartModes;
    std::vector<bool> rstFlags;
    bool timingProfilingEnabled=false;
    for(size_t i=0;i<static_cast<size_t>(config.GetPortCount());++i)
    {
        auto strIdx=std::to_string(i+1);

        //lp - local port, sock or port number
        if(options.CheckParamPresent("lp"+strIdx,false,""))
        {
            if(options.CheckIsInteger("lp"+strIdx,1,65535,false,""))
            {
                localPorts.push_back(options.GetInteger("lp"+strIdx));
                localFiles.push_back("");
            }
            else
            {
                localPorts.push_back(0);
                localFiles.push_back(options.GetString("lp"+strIdx));
            }
        }
        else
        {
            localPorts.push_back(0);
            localFiles.push_back("");
        }

        //ps - port speed
        if(options.CheckParamPresent("ps"+strIdx,false,""))
        {
            options.CheckIsInteger("ps"+strIdx,1,1000000,true,"uart speed is invalid!");
            uartSpeeds.push_back(options.GetInteger("ps"+strIdx));
        }
        else
            uartSpeeds.push_back(0);

        //pm - port mode
        if(options.CheckParamPresent("pm"+strIdx,false,"") && uartSpeeds[i]>0)
        {
            options.CheckIsInteger("pm"+strIdx,0,255,true,"uart mode is invalid!");
            uartModes.push_back(options.GetInteger("pm"+strIdx));
            if(options.GetInteger("pm"+strIdx)==255)
                timingProfilingEnabled=true;
        }
        else
            uartModes.push_back(255);

        //rst - reset
        if(options.CheckParamPresent("rst"+strIdx,false,""))
        {
            options.CheckIsBoolean("rst"+strIdx,true,"reset-needed value is invalid!");
            rstFlags.push_back(options.GetBoolean("rst"+strIdx));
        }
        else
            rstFlags.push_back(false);
    }

    config.SetTCPBuffSz(65536);
    config.SetServiceIntervalMS(500); //management interval
    config.SetLingerSec(30); //linger

    //timeout for main thread waiting for external signals
    const timespec sigTs={2,0};

    //create remote-config objects
    std::vector<PortConfig> portConfigs;
    for(size_t i=0; i<static_cast<size_t>(config.GetPortCount()); ++i)
        portConfigs.push_back(
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
    for(size_t i=0;i<static_cast<size_t>(config.GetPortCount());++i)
    {
        auto testTime=calculate_poll_interval(uartSpeeds[i],config.GetPortPayloadSz(),pmin,pmax);
        if(testTime<minPollTime)
            minPollTime=testTime;
    }
    Timer pollTimer(timerLogger,messageBroker,config,minPollTime,timingProfilingEnabled);
    messageBroker.AddSubscriber(pollTimer);

    std::vector<std::shared_ptr<TCPListener>> tcpListeners;
    std::vector<std::shared_ptr<PTYListener>> ptyListeners;
    bool listenersCreated=false;
    for(size_t i=0;i<portConfigs.size();++i)
    {
        if(portConfigs[i].speed==0 || portConfigs[i].mode==SerialMode::SERIAL_CLOSED)
        {
            mainLogger->Info()<<"Port "<<i<<" will not be used, remote uart speed or mode parameters are not configured properly";
            continue;
        }
        if(!portConfigs[i].ptsListener.empty())
        {
            listenersCreated=true;
            auto logger=logFactory.CreateLogger("PTYListener:"+std::to_string(i));
            ptyListeners.push_back(std::make_shared<PTYListener>(logger,messageBroker,config,portConfigs[i]));
        }
        else if(portConfigs[i].listener.address.isValid && portConfigs[i].listener.port>0)
        {
            listenersCreated=true;
            auto logger=logFactory.CreateLogger("TCPListener:"+std::to_string(i));
            tcpListeners.push_back(std::make_shared<TCPListener>(logger,messageBroker,config,portConfigs[i]));
        }
        else
        {
            mainLogger->Info()<<"Port "<<i<<" will not be used, local ipaddr/port or pty parameters are not configured properly";
            continue;
        }
    }

    if(!listenersCreated)
    {
        mainLogger->Error()<<"No valid port listeners defined!";
        return 1;
    }

    std::vector<std::shared_ptr<PortWorker>> portWorkers;
    std::vector<std::shared_ptr<RemoteBufferTracker>> buffTrackers;
    for(size_t i=0;i<portConfigs.size();++i)
    {
        auto rTrackerLogger=logFactory.CreateLogger("BuffTracker:"+std::to_string(i));
        auto rTracker=std::make_shared<RemoteBufferTracker>(rTrackerLogger,config,config.GetRemoteRingBuffSize());
        auto portLogger=logFactory.CreateLogger("PortWorker:"+std::to_string(i));
        auto portWorker=std::make_shared<PortWorker>(portLogger,messageBroker,config,portConfigs[i],*(rTracker));
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
        mainLogger->Error()<<"Failed to setup signal-handling";
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
