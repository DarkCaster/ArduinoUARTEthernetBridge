#include <cstring>
#include <cerrno>
#include <iostream>
#include <string>
#include <csignal>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>
#include <sys/time.h>

#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <netdb.h>

#include "IPAddress.h"
#include "TCPConnection.h"
#include "ILogger.h"
#include "StdioLoggerFactory.h"
#include "IMessage.h"
#include "MessageBroker.h"
#include "ShutdownHandler.h"
#include "LoopbackTester.h"
#include "TestWorker.h"

static void usage(const std::string &self)
{
    std::cerr<<"Usage: "<<self<<" [parameters]"<<std::endl;
    std::cerr<<"  mandatory parameters:"<<std::endl;
    std::cerr<<"    -pc <count> UART port count"<<std::endl;
    std::cerr<<"    -lp{n} <port> TCP ports UARTClient util is listening at"<<std::endl;
    std::cerr<<"    -tsz <bytes> test block size, default: 4096 bytes"<<std::endl;
    std::cerr<<"    -tto <ms> timeout for sending the whole block, and for receiving answer, default: 5000 ms"<<std::endl;
    std::cerr<<"  experimental and optimization parameters:"<<std::endl;
    std::cerr<<"    -bsz <bytes> size of TCP buffer used for transferring data, default: 64k"<<std::endl;
    std::cerr<<"    -mt <time, ms> management interval used for some internal routines, default: 500"<<std::endl;
    std::cerr<<"    -cf <seconds> timeout for flushing data when closing sockets, -1 to disable, 0 - close without flushing, default: 30"<<std::endl;
}

static int param_error(const std::string &self, const std::string &message)
{
    std::cerr<<message<<std::endl;
    usage(self);
    return 1;
}

static void TuneSocketBaseParams(std::shared_ptr<ILogger> &logger, int fd, const int lingerTime,const int tcpBuffSize)
{
    //set linger
    linger cLinger={0,0};
    cLinger.l_onoff=lingerTime>=0;
    cLinger.l_linger=cLinger.l_onoff?lingerTime:0;
    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &cLinger, sizeof(linger))!=0)
        logger->Warning()<<"Failed to set SO_LINGER option to socket: "<<strerror(errno);
    //set buffer size
    auto bsz=tcpBuffSize;
    if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bsz, sizeof(bsz)))
        logger->Warning()<<"Failed to set SO_SNDBUF option to socket: "<<strerror(errno);
    bsz=tcpBuffSize;
    if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bsz, sizeof(bsz)))
        logger->Warning()<<"Failed to set SO_RCVBUF option to socket: "<<strerror(errno);
    int tcpNoDelay=1;
    if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &tcpNoDelay, sizeof(tcpNoDelay)))
        logger->Warning()<<"Failed to set TCP_NODELAY option to socket: "<<strerror(errno);
    int tcpQuickAck=1;
    if(setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, &tcpQuickAck, sizeof(tcpQuickAck)))
        logger->Warning()<<"Failed to set TCP_QUICKACK option to socket: "<<strerror(errno);
}

static void SetSocketCustomTimeouts(std::shared_ptr<ILogger> &logger, int fd, const timeval &tv)
{
    timeval rtv=tv;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &rtv, sizeof(rtv))!=0)
        logger->Warning()<<"Failed to set SO_RCVTIMEO option to socket: "<<strerror(errno);
    timeval stv=tv;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &stv, sizeof(stv))!=0)
        logger->Warning()<<"Failed to set SO_SNDTIMEO option to socket: "<<strerror(errno);
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

    int testBlockSize=4096;
    if(args.find("-tsz")!=args.end())
    {
        auto bsz=std::atoi(args["-tsz"].c_str());
        if(bsz<1||bsz>1000000000)
            return param_error(argv[0],"Test block size is invalid");
        testBlockSize=bsz;
    }

    int testTimeout=5000;
    if(args.find("-tto")!=args.end())
    {
        auto to=std::atoi(args["-tto"].c_str());
        if(to<1||to>3600000)
            return param_error(argv[0],"Test timeout is too big");
        testTimeout=to;
    }

    StdioLoggerFactory logFactory;
    auto mainLogger=logFactory.CreateLogger("Main");
    auto messageBrokerLogger=logFactory.CreateLogger("MSGBroker");

    //configure the most essential stuff
    MessageBroker messageBroker(messageBrokerLogger);

    //shutdown handler
    ShutdownHandler shutdownHandler;
    messageBroker.AddSubscriber(shutdownHandler);

    //create sigset_t struct with signals
    sigset_t sigset;
    sigemptyset(&sigset);
    if(sigaddset(&sigset,SIGHUP)!=0||sigaddset(&sigset,SIGTERM)!=0||sigaddset(&sigset,SIGUSR1)!=0||sigaddset(&sigset,SIGUSR2)!=0||sigaddset(&sigset,SIGPIPE)!=0||pthread_sigmask(SIG_BLOCK,&sigset,nullptr)!=0)
    {
        mainLogger->Error()<<"Failed to setup signal-handling"<<std::endl;
        return 1;
    }

    std::vector<std::shared_ptr<LoopbackTester>> lbTesters;
    std::vector<std::shared_ptr<TestWorker>> testWorkers;

    //create target connections
    std::vector<std::shared_ptr<Connection>> connections;
    for(auto const& port: localPorts)
    {
        //create socket
        auto fd=socket(AF_INET,SOCK_STREAM,0);
        if(fd<0)
        {
            mainLogger->Error()<<"Failed to create new socket: "<<strerror(errno);
            return 1;
        }

        TuneSocketBaseParams(mainLogger,fd,linger,tcpBuffSize);
        auto interval=timeval{mgInterval/1000,(mgInterval-mgInterval/1000*1000)*1000};
        SetSocketCustomTimeouts(mainLogger,fd,interval);

        int cr=-1;
        sockaddr_in v4sa={};
        IPAddress("127.0.0.1").ToSA(&v4sa);
        v4sa.sin_port=htons(static_cast<uint16_t>(port));
        cr=connect(fd,reinterpret_cast<sockaddr*>(&v4sa), sizeof(v4sa));

        if(cr<0)
        {
            mainLogger->Error()<<"Failed to connect: "<<strerror(errno);
            if(close(fd)!=0)
            {
                mainLogger->Error()<<"Failed to perform proper socket close after connection failure: "<<strerror(errno);
                return 1;
            }
            return 1;
        }

        auto target=std::make_shared<TCPConnection>(fd,static_cast<uint16_t>(port));
        auto lbLogger=logFactory.CreateLogger("Test:"+std::to_string(port));
        auto twLogger=logFactory.CreateLogger("TestWorker:"+std::to_string(port));
        auto lbTester=std::make_shared<LoopbackTester>(lbLogger,*(target.get()),testBlockSize,testTimeout);
        auto tWorker=std::make_shared<TestWorker>(twLogger,*(lbTester.get()));
        lbTesters.push_back(lbTester);
        testWorkers.push_back(tWorker);
        connections.push_back(target);
    }

    //startup
    mainLogger->Info()<<"Test starting-up"<<std::endl;
    for(auto const& tester:lbTesters)
        tester->Startup();
    for(auto const& worker:testWorkers)
        worker->Startup();

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
    for(auto const& worker:testWorkers)
        worker->RequestShutdown();
    for(auto const& tester:lbTesters)
        tester->RequestShutdown();

    //wait for background workers shutdown complete
    for(auto const& worker:testWorkers)
        worker->Shutdown();
    for(auto const& tester:lbTesters)
        tester->Shutdown();

    mainLogger->Info()<<"Clean shutdown"<<std::endl;


    return 0;
}
