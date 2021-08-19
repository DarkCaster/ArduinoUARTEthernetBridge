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
    curMode=0xFE;
    sessionId=0;
    txUsedSz=0;
}

void UARTWorker::ProcessRequest(const Request &request)
{
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
            rxRingBuff.Reset();
            break;
        case ReqType::ResetOpen:
            resetHelper->StartReset(RESET_TIME_MS);
            [[fallthrough]];
        case ReqType::Open:
            if(IS_OPEN(curMode))
            {
                uart->end();
                rxRingBuff.Reset();
            }
            sessionId=0; //used only on client start, so reset session id
            curMode=request.arg;
            if(IS_OPEN(curMode))
                uart->begin(static_cast<unsigned long>(rxDataBuff[0])|static_cast<unsigned long>(rxDataBuff[1])<<8|static_cast<unsigned long>(rxDataBuff[2])<<16,curMode);
            break;
        case ReqType::Close:
            if(IS_OPEN(curMode))
            {
                uart->end();
                rxRingBuff.Reset();
            }
            curMode=MODE_CLOSED;
            break;
        case ReqType::NoCommand:
        default:
            break;
    }
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

void UARTWorker::FillTXBuff(bool reset)
{
    if(reset)
        txUsedSz=0;
    if(IS_OPEN(curMode))
    {
        size_t sz=uart->available();
        if(sz>(DATA_PAYLOAD_SIZE-txUsedSz))
            sz=DATA_PAYLOAD_SIZE-txUsedSz;
        if(sz<1)
            return;
        txUsedSz+=uart->readBytes(txDataBuff+txUsedSz,sz);
    }
    if(curMode==MODE_LOOPBACK)
    {
        auto tail=rxRingBuff.GetTail();
        size_t sz=tail.maxSz;
        if(sz>(DATA_PAYLOAD_SIZE-txUsedSz))
            sz=DATA_PAYLOAD_SIZE-txUsedSz;
        if(sz<1)
            return;
        memcpy(txDataBuff+txUsedSz,tail.buffer,sz);
        rxRingBuff.Commit(tail,sz);
        txUsedSz+=sz;
    }
}

Response UARTWorker::ProcessTX()
{
    if(txUsedSz>0)
       return Response{RespType::Data,static_cast<uint8_t>(rxRingBuff.IsHalfUsed()<<7|(sessionId&0x7F)),static_cast<uint8_t>(txUsedSz)};
    return Response{RespType::NoCommand,static_cast<uint8_t>(rxRingBuff.IsHalfUsed()<<7|(sessionId&0x7F)),0};
}
