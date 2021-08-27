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

// examples

// Arduino Mega 2560 (atmega 2560) with 3 uart ports:
//  1) throughput of ~60 kbitps full-duplex on 3 ports (Serial1,Serial2,Serial3) simultaneously, uart-port interface speed set to 250000 8N1, triggering of reset pins is disabled:
//   -ra ENC28J65E366.lan -tp 50000 -up 1 -pc 3 -pls 64 -rbs 1600 -ptl 8600 -ptr 8600 -lp1 65001 -lp2 65002 -lp3 65003 -ps1 250000 -pm1 6 -ps2 250000 -pm2 6 -ps3 250000 -pm3 6 -rst1 0 -rst2 0 -rst3 0
//  2) throughput of ~100 kbitps full-duplex only on 1 port at time, uart-port interface speed set to 115200 8N1, triggering of reset pins is enabled:
//  sutable for flashing arduino atmega firmware over uart with optiboot+avrdude
//   -ra ENC28J65E366.lan -tp 50000 -up 1 -pc 3 -pls 64 -rbs 1600 -ptl 5150 -ptr 5150 -lp1 /tmp/ttyETH1 -lp2 /tmp/ttyETH2 -lp3 /tmp/ttyETH3 -ps1 115200 -pm1 6 -ps2 115200 -pm2 6 -ps3 115200 -pm3 6 -rst1 1 -rst2 1 -rst3 1

void usage(const std::string &self)
{
    std::cerr<<"Usage: "<<self<<" [parameters]"<<std::endl;
    std::cerr<<"  mandatory parameters, must match the values hardcoded at server firmware:"<<std::endl;
    std::cerr<<"    -ra <ip address, or host name> remote address to connect"<<std::endl;
    std::cerr<<"    -tp <port> remote TCP port"<<std::endl;
    std::cerr<<"    -pc <count> UART port count configured at remote side, required to match for operation"<<std::endl;
    std::cerr<<"    -pls <bytes> network payload size for single port in bytes, required to match for operation"<<std::endl;
    std::cerr<<"    -rbs <bytes> remote ring-buffer size for incoming data, should match to prevent data loss"<<std::endl;
    std::cerr<<"  uart port related parameters:"<<std::endl;
    std::cerr<<"    -ps{n} <speed in bits-per-second> open remote uart port #n at provided speed, example: -ps1 57600"<<std::endl;
    std::cerr<<"    -pm{n} <mode number> set mode for remote uart port #n, example: -pm1 6 (equals to SERIAL_8N1 arduino-define)"<<std::endl;
    std::cerr<<"    -rst{n} <0,1> perform reset on connection to port #n, default: 0 - do not perform reset"<<std::endl;
    std::cerr<<"    -lp{n} <port> local TCP port number OR file path for creating PTS symlink, example -lp1 40001 -lp2 40002 -lp3 /tmp/usbETH3"<<std::endl;
    std::cerr<<"  optional parameters:"<<std::endl;
    std::cerr<<"    -up <0,1> 1 - enable use of less reliable UDP transport with lower latency and jitter, default: 0 - disabled"<<std::endl;
    std::cerr<<"    -la <ip-addr> local IP to listen for TCP channels enabled by -lp{n} option, default: 127.0.0.1"<<std::endl;
    std::cerr<<"    -ptl <time, us> interval in micro-seconds between polling+sending data operations, limits outgoing throughput, default: 8192, invalid values will result in data loss"<<std::endl;
    std::cerr<<"    -ptr <time, us> remote poll interval, limits incoming throughput, default: 8192, invalid values will result in data loss"<<std::endl;

}

int param_error(const std::string &self, const std::string &message)
{
    std::cerr<<message<<std::endl;
    usage(self);
    return 1;
}

int main (int argc, char *argv[])
{
    //parse command-line options, fillup config
    OptionsParser options(argc,argv,usage);
    options.CheckEmpty(true,"Mandatory parameters are missing!");

    Config config;

    options.CheckParamPresent("ra",true,"remote address or DNS-name is missing");
    config.SetRemoteAddr(options.GetString("ra"));

    options.CheckParamPresent("tp",true,"TCP port must be provided");
    options.CheckIsInteger("tp",1,65535,true,"TCP port is invalid");
    config.SetTCPPort(static_cast<uint16_t>(options.GetInteger("tp")));

    options.CheckParamPresent("pc",true,"port count must be provided");
    options.CheckIsInteger("pc",1,32,true,"port count value is invalid");
    config.SetPortCount(options.GetInteger("pc"));

    options.CheckParamPresent("pls",true,"Network payload size must be provided");
    options.CheckIsInteger("pls",1,255,true,"Network payload size must be provided");
    config.SetPortPayloadSize(options.GetInteger("pls"));

    options.CheckParamPresent("rbs",true,"remote ring-buffer size is missing");
    options.CheckIsInteger("rbs",1,65535,true,"remote ring-buffer size is invalid!");
    config.SetRemoteRingBuffSize(options.GetInteger("rbs"));

    //ps, pm, rst, lp params parsed below

    if(!options.CheckParamPresent("up",false,""))
        config.SetUDPEnabled(false);
    else
    {
        options.CheckIsBoolean("up",true,"UDP mode parameter is invalid");
        config.SetUDPEnabled(options.GetBoolean("up"));
    }

    ImmutableStorage<IPAddress> localAddr(IPAddress("127.0.0.1"));
    if(options.CheckParamPresent("la",false,""))
    {
        options.CheckIsIPAddress("la",true,"listen IP address is invalid!");
        localAddr.Set(IPAddress(options.GetString("la")));
    }

    int ptl=8192;
    if(options.CheckParamPresent("ptl",false,""))
    {
        options.CheckIsInteger("ptl",256,1000000,true,"Local poll time is invalid");
        ptl=options.GetInteger("ptl");
    }

    config.SetRemotePollIntervalUS(8192);
    if(options.CheckParamPresent("ptr",false,""))
    {
        options.CheckIsInteger("ptr",256,1000000,true,"Remote poll time is invalid");
        config.SetRemotePollIntervalUS(options.GetInteger("ptr"));
    }

    std::vector<int> localPorts;
    std::vector<std::string> localFiles;
    std::vector<int> uartSpeeds;
    std::vector<int> uartModes;
    std::vector<bool> rstFlags;
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
    Timer pollTimer(timerLogger,messageBroker,config,ptl);
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

    auto outSpeed=static_cast<int>(1000000.0/static_cast<double>(ptl)*static_cast<double>(config.GetPortPayloadSz())*8.0);
    auto inSpeed=static_cast<int>(1000000.0/static_cast<double>(config.GetRemotePollIntervalUS())*static_cast<double>(config.GetPortPayloadSz())*8.0);

    mainLogger->Info()<<"Maximum calculated TX speed: "<<outSpeed<<" bps";
    mainLogger->Info()<<"Maximum calculated RX speed: "<<inSpeed<<" bps";

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
