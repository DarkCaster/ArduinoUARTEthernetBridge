#include "PortWorker.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <poll.h>

PortWorker::PortWorker(std::shared_ptr<ILogger>& _logger, IMessageSender& _sender, const IConfig& _config, const PortConfig& _portConfig, RemoteBufferTracker& _remoteBufferTracker):
    logger(_logger),
    sender(_sender),
    config(_config),
    portConfig(_portConfig),
    remoteBufferTracker(_remoteBufferTracker),
    rxRingBuff(static_cast<size_t>(_config.GetIncomingDataBufferSec())*(_portConfig.speed/8))
{
    shutdownPending.store(false);
    connected.store(false);
    openPending=true;
    client=nullptr;
    sessionId=0;
    resetPending=false;
    oldSessionPkgCount=0;
}

bool PortWorker::ReadyForMessage(const MsgType msgType)
{
    return msgType==MSG_PORT_OPEN || msgType==MSG_CONNECTED;
}

void PortWorker::OnMessage(const void* const, const IMessage& message)
{
    if(message.msgType==MSG_PORT_OPEN)
        OnPortOpen(static_cast<const IPortOpenMessage&>(message));
    if(message.msgType==MSG_CONNECTED)
        OnConnected(static_cast<const IConnectedMessage&>(message));
}

void PortWorker::OnConnected(const IConnectedMessage&)
{
    connected.store(true);
}

void PortWorker::OnPortOpen(const IPortOpenMessage& message)
{
    std::lock_guard<std::mutex> clientGuard(clientLock);
    //get rid of old client if it still not closed
    if(client!=nullptr)
    {
        logger->Info()<<"Disposing previous client connection, fd: "<<client->fd;
        client->Dispose();
    }
    //setup new client
    client=message.connection;
    resetPending=portConfig.resetOnConnect;
    if(resetPending)
    {
        sessionId++;
        if(sessionId>0x7F)
            sessionId=0x01;
        remoteBufferTracker.Reset();
        std::lock_guard<std::mutex> ringBuffGuard(ringBuffLock);
        logger->Info()<<"Dumping RX ring-buffer contents";
        rxRingBuff.Reset();
    }
}

Request PortWorker::ProcessTX(uint32_t counter, uint8_t* txBuff)
{
    //do not start processing until receiving first connect-confirmation
    if(!connected.load())
        return Request{ReqType::NoCommand,0,0};

    //port will be opened at first call
    if(openPending)
    {
        openPending=false;
        //write port speed to txBuff;
        txBuff[0]=portConfig.speed&0xFF;
        txBuff[1]=(portConfig.speed>>8)&0xFF;
        txBuff[2]=(portConfig.speed>>16)&0xFF;
        logger->Info()<<"Sending port open request, speed: "<<portConfig.speed<<"; mode: "<< static_cast<int>(portConfig.mode);
        return Request{ReqType::Open,static_cast<uint8_t>(portConfig.mode),3};
    }

    //client operations must be interlocked, client's FD must be in non-blocking mode
    std::lock_guard<std::mutex> clientGuard(clientLock);

    if(resetPending)
    {
        resetPending=false;
        logger->Info()<<"Sending reset request";
        return Request{ReqType::Reset,static_cast<uint8_t>(sessionId),0};
    }

    //logger->Info()<<"Remote buffer fillup: "<<remoteBufferFillup;

    //calculate how much data we want to read from client
    auto dataToRead=static_cast<size_t>(config.GetNetworkPayloadSz());
    if(dataToRead>remoteBufferTracker.GetAvailSpace())
        dataToRead=remoteBufferTracker.GetAvailSpace();

    //send ReqType::NoCommand if we cannot read data from client
    if(dataToRead<=0 || client==nullptr)
        return Request{ReqType::NoCommand,0,0};

    //poll data from client, client->fd must be marked nonblocking (O_NONBLOCK)
    auto dataRead=read(client->fd,txBuff,static_cast<size_t>(dataToRead));
    if(dataRead<=0)
    {
        auto error=errno;
        if(dataRead==0 || (error!=EINTR && error!=EWOULDBLOCK))
        {
            if(dataRead==0)
                logger->Info()<<"Client was disconnected";
            else
                logger->Info()<<"Client read was failed with: "<<strerror(error);
            client->Dispose();
            client=nullptr;
        }
        return Request{ReqType::NoCommand,0,0};
    }

    //logger->Info()<<"Client bytes send: "<<dataRead;
    remoteBufferTracker.AddPackage(static_cast<size_t>(dataRead),counter);
    return Request{ReqType::Data,0,static_cast<uint8_t>(dataRead)};
}

void PortWorker::ProcessRX(const Response& response, const uint8_t* rxBuff)
{
    //client operations must be interlocked
    {
        std::lock_guard<std::mutex> clientGuard(clientLock);
        remoteBufferTracker.ConfirmPackage(response.counter,(response.arg&0x80)!=0);
        if(client==nullptr)
            return;
        if(sessionId!=(response.arg&0x7F))
        {
            oldSessionPkgCount++;
            return;
        }
    }

    if(oldSessionPkgCount>0)
    {
        logger->Info()<<"Dropped "<<oldSessionPkgCount<<" incoming packages from previous session";
        oldSessionPkgCount=0;
    }

    if(response.type==RespType::NoCommand)
        return;
    //write data to ring-buffer
    {
        std::lock_guard<std::mutex> ringBuffGuard(ringBuffLock);
        auto head=rxRingBuff.GetHead();
        size_t szToWrite=response.plSz;
        while (szToWrite>0 && head.maxSz>0)
        {
            auto sz=szToWrite>head.maxSz?head.maxSz:szToWrite;
            memcpy(head.buffer,rxBuff+response.plSz-szToWrite,sz);
            szToWrite-=sz;
            rxRingBuff.Commit(head,sz);
            //logger->Info()<<"Ring buffer bytes written: "<<sz;
            head=rxRingBuff.GetHead();
        }
        if(szToWrite>0)
            logger->Warning()<<"Ring buffer overrun detected, bytes lost: "<<szToWrite;
    }
    //signal worker to proceed
    ringBuffTrigger.notify_one();
}

void PortWorker::Worker()
{
    logger->Info()<<"Starting PortWorker";
    pollfd pfd={};
    while(!shutdownPending.load())
    {
        std::unique_lock<std::mutex> lock(ringBuffLock);
        while(rxRingBuff.UsedSize()<1 && !shutdownPending.load())
            ringBuffTrigger.wait(lock);
        auto tail=rxRingBuff.GetTail();
        lock.unlock();
        //just as precaution, may be triggered on shutdown
        if(tail.maxSz<1)
            continue;
        //get client
        std::shared_ptr<Connection> currConn=nullptr;
        {
            std::lock_guard<std::mutex> clientGuard(clientLock);
            currConn=client;
        }
        //poll for write
        size_t szToWrite=tail.maxSz;
        if(currConn!=nullptr)
        {
            pfd.fd=currConn->fd;
            pfd.events=POLLOUT;
            pfd.revents=0;
            if(poll(&pfd,1,config.GetServiceIntervalMS())<0)
            {
                logger->Error()<<"Write poll failed with error: "<<strerror(errno);
                std::this_thread::sleep_for(std::chrono::milliseconds(config.GetServiceIntervalMS()));
            }
            else if (pfd.revents & (POLLHUP|POLLNVAL|POLLERR))
            {
                logger->Error()<<"Write poll failed, client being disconnected";
                std::this_thread::sleep_for(std::chrono::milliseconds(config.GetServiceIntervalMS()));
            }
            else if (pfd.revents & POLLOUT)
            {
                auto dw=write(client->fd,tail.buffer,szToWrite);
                if(dw<0)
                {
                    logger->Error()<<"Write failed with error: "<<strerror(errno);
                    client->Dispose();
                    szToWrite=0;
                }
                else
                    szToWrite=static_cast<size_t>(dw);
            }
        }
        else
        {
            logger->Warning()<<"Write failed - client is not connected, bytes lost: "<<szToWrite;
            std::this_thread::sleep_for(std::chrono::milliseconds(config.GetServiceIntervalMS()));
        }
        //free data that was read from ringBuffer
        {
            std::lock_guard<std::mutex> guard(ringBuffLock);
            rxRingBuff.Commit(tail,szToWrite);
        }
    }
    logger->Info()<<"PortWorker was shutdown";
}

void PortWorker::OnShutdown()
{
    shutdownPending.store(true);
    ringBuffTrigger.notify_one();
}
