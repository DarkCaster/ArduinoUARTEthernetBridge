#include "PortWorker.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>

PortWorker::PortWorker(std::shared_ptr<ILogger>& _logger, IMessageSender& _sender, const IConfig& _config, const RemoteConfig& _portConfig, const int _portId):
    logger(_logger),
    sender(_sender),
    config(_config),
    portConfig(_portConfig),
    portId(_portId),
    bufferLimit(config.GetUARTBuffSz()*config.GetRingBuffSegCount()/2)
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
            logger->Info()<<"Client for port "<<portId<<" was closed";
        auto error=errno;
        if(error!=EINTR && error!=EWOULDBLOCK)
        {
            logger->Info()<<"Client read for port "<<portId<<" was failed with: "<<strerror(error);
            client->Dispose();
            client=nullptr;
        }
        return Request{ReqType::NoCommand,0,0};
    }

    remoteBufferFillup+=static_cast<int>(dataRead);
    return Request{ReqType::Data,0,static_cast<uint8_t>(dataRead)};
}

void PortWorker::ProcessRX()
{

}

void PortWorker::Worker()
{

}

void PortWorker::OnShutdown()
{
    shutdownPending.store(true);
}
