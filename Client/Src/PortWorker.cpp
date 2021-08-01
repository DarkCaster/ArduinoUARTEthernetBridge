#include "PortWorker.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>

PortWorker::PortWorker(std::shared_ptr<ILogger>& _logger, IMessageSender& _sender, const IConfig& _config, const RemoteConfig& _portConfig):
    logger(_logger),
    sender(_sender),
    config(_config),
    portConfig(_portConfig),
    bufferLimit(config.GetUARTBuffSz()*config.GetRingBuffSegCount()/2),
    rxRingBuff(static_cast<size_t>(_config.GetIncomingDataBufferSec())*(_portConfig.speed/8))
{
    shutdownPending.store(false);
    connected.store(false);
    openPending=true;
    client=nullptr;
    sessionId=0;
    resetPending=false;
    remoteBufferFillup=bufferLimit;
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
        client->Dispose();
    //setup new client
    client=message.connection;
    resetPending=portConfig.resetOnConnect;
    if(resetPending)
    {
        sessionId++;
        if(sessionId>0x7F)
            sessionId=0x01;
        //TODO: reset ring-buffer for RX
        remoteBufferFillup=bufferLimit;
    }
}

Request PortWorker::ProcessTX(uint8_t* txBuff)
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
        return Request{ReqType::Open,static_cast<uint8_t>(portConfig.mode),3};
    }

    //client operations must be interlocked, client's FD must be in non-blocking mode
    std::lock_guard<std::mutex> clientGuard(clientLock);

    if(resetPending)
    {
        resetPending=false;
        return Request{ReqType::Reset,static_cast<uint8_t>(sessionId),0};
    }

    //calculate how much data we want to read from client
    auto dataToRead=config.GetUARTBuffSz();
    if(dataToRead>(bufferLimit-remoteBufferFillup))
        dataToRead=bufferLimit-remoteBufferFillup;

    //send ReqType::NoCommand if we cannot read data from client
    if(dataToRead<=0 || client==nullptr)
        return Request{ReqType::NoCommand,0,0};

    //poll data from client, client->fd must be marked nonblocking (O_NONBLOCK)
    auto dataRead=read(client->fd,txBuff,static_cast<size_t>(dataToRead));
    if(dataRead<=0)
    {
        if(dataRead==0)
            logger->Info()<<"Client was disconnected";
        auto error=errno;
        if(error!=EINTR && error!=EWOULDBLOCK)
        {
            logger->Info()<<"Client read was failed with: "<<strerror(error);
            client->Dispose();
            client=nullptr;
        }
        return Request{ReqType::NoCommand,0,0};
    }

    remoteBufferFillup+=static_cast<int>(dataRead);
    return Request{ReqType::Data,0,static_cast<uint8_t>(dataRead)};
}

void PortWorker::ProcessRX(const Response& response, const uint8_t* rxBuff)
{
    //client operations must be interlocked
    {
        std::lock_guard<std::mutex> clientGuard(clientLock);
        remoteBufferFillup=(response.arg&0x80)!=0?bufferLimit:0;
        if(client==nullptr || sessionId!=(response.arg&0x7F))
            return;
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
        //TODO: write data to client's FD
        size_t szToWrite=1;
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
