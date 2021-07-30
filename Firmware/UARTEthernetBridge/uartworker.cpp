#include "uartworker.h"
#include "configuration.h"

#define MODE_CLOSED 0xFE
#define MODE_LOOPBACK 0xFF
#define IS_OPEN(mode) (mode<MODE_CLOSED)

void UARTWorker::Setup(ResetHelper* const _resetHelper, HardwareSerial* const _uart, uint8_t* _rxDataBuff, uint8_t* _txDataBuff)
{
    resetHelper=_resetHelper;
    uart=_uart;
    rxDataBuff=_rxDataBuff;
    txDataBuff=_txDataBuff;
    pollInterval=IDLE_POLL_INTERVAL_US;
    curMode=0xFE;
    sessionId=0;
}

unsigned long UARTWorker::ProcessRequest(const Request &request)
{
    unsigned long speed=0;
    uint8_t szLeft;
    switch (request.type)
    {
        case ReqType::Data:
            //try to copy data to the ring-buffer
            szLeft=request.plSz;
            while(szLeft>0)
            {
                auto head=rxRingBuff.GetHead();
                uint8_t szToWrite=szLeft>head.maxSz?head.maxSz:szLeft;
                if(szToWrite<1)
                    break; //no space left for storing data at ring-buffer, data will be lost
                memcpy(head.buffer,rxDataBuff+request.plSz-szLeft,szToWrite);
                rxRingBuff.Commit(head,szToWrite);
                szLeft-=szToWrite;
            }
            break;
        case ReqType::Reset:
            resetHelper->StartReset(RESET_TIME_MS);
            sessionId=request.arg;
            //TODO: drop current data in the ring-buffer on reset?
            break;
        case ReqType::ResetOpen:
            resetHelper->StartReset(RESET_TIME_MS);
            [[fallthrough]];
        case ReqType::Open:
            if(IS_OPEN(curMode))
            {
                uart->end();
                //TODO: drop current data in the ring-buffer on open?
            }
            sessionId=0; //used only on client start, so reset session id
            curMode=request.arg;
            speed=static_cast<unsigned long>(rxDataBuff[0])|static_cast<unsigned long>(rxDataBuff[1])<<8|static_cast<unsigned long>(rxDataBuff[2])<<16;
            if(IS_OPEN(curMode))
                uart->begin(speed,curMode);
            //re-calculate poll interval for selected data-transfer speed
            pollInterval=static_cast<unsigned long>(1000000.0f/static_cast<float>(speed)*8.0f*static_cast<float>(UART_BUFFER_SIZE));
            break;
        case ReqType::Close:
            if(IS_OPEN(curMode))
            {
                uart->end();
                //TODO: drop current data in the ring-buffer on close?
            }
            curMode=MODE_CLOSED;
            pollInterval=IDLE_POLL_INTERVAL_US;
            break;
        case ReqType::NoCommand:
        default:
            break;
    }
    return pollInterval;
}

void UARTWorker::ProcessRX()
{
    if(!resetHelper->ResetComplete() || !IS_OPEN(curMode))
        return;
    //write data to uart-port from ring-buffer
    auto szLeft=uart->availableForWrite();
    while(szLeft>0)
    {
        auto tail=rxRingBuff.GetTail();
        auto szToWrite=static_cast<unsigned>(szLeft)>tail.maxSz?tail.maxSz:szLeft;
        if(szToWrite<1)
            return; //nothing to write
        szToWrite=uart->write(tail.buffer,szToWrite);
        rxRingBuff.Commit(tail,szToWrite);
        szLeft-=szToWrite;
    }
}

Response UARTWorker::ProcessTX()
{
    if(IS_OPEN(curMode))
    {
        //copy data from UART to txDataBuff for sending
        auto sz=uart->available();
        if(sz>0)
        {
            sz=uart->readBytes(txDataBuff,sz>UART_BUFFER_SIZE?UART_BUFFER_SIZE:sz);
            return Response{RespType::Data,static_cast<uint8_t>(rxRingBuff.IsHalfUsed()),static_cast<uint8_t>(sz)};
        }
        return Response{RespType::NoCommand,static_cast<uint8_t>(rxRingBuff.IsHalfUsed()),0};
    }

    if(curMode==MODE_LOOPBACK)
    {
        auto tail=rxRingBuff.GetTail();
        auto sz=tail.maxSz>UART_BUFFER_SIZE?UART_BUFFER_SIZE:tail.maxSz;
        if(sz>0)
        {
            memcpy(txDataBuff,tail.buffer,sz);
            rxRingBuff.Commit(tail,sz);
            return Response{RespType::Data,static_cast<uint8_t>(rxRingBuff.IsHalfUsed()<<7|(sessionId&0x7F)),static_cast<uint8_t>(sz)};
        }
    }

    return Response{RespType::NoCommand,static_cast<uint8_t>(rxRingBuff.IsHalfUsed()<<7|(sessionId&0x7F)),0};
}
