#include "uartworker.h"
#include "configuration.h"

#define MODE_CLOSED 0xFF
#define IS_OPEN(mode) (mode<MODE_CLOSED)

void UARTWorker::Setup(ResetHelper* const _resetHelper, HardwareSerial* const _uart, uint8_t* _rxDataBuff, uint8_t* _txDataBuff)
{
    resetHelper=_resetHelper;
    uart=_uart;
    rxDataBuff=_rxDataBuff;
    txDataBuff=_txDataBuff;
    curMode=MODE_CLOSED;
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
                uint8_t szToWrite=szLeft>head.maxSz?static_cast<uint8_t>(head.maxSz):szLeft;
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
            {
                auto speed=static_cast<unsigned long>(rxDataBuff[0])|static_cast<unsigned long>(rxDataBuff[1])<<8|static_cast<unsigned long>(rxDataBuff[2])<<16|static_cast<unsigned long>(rxDataBuff[3])<<24;
                if(speed<1)
                    curMode=MODE_CLOSED;
                else
                {
                    uart->begin(speed,curMode);
                    uart->setTimeout(0);
                }
            }
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
    auto uartAvail=uart->availableForWrite();
    //limit uart-write bandwidth
    if(uartAvail>PORT_IO_SIZE)
        uartAvail=PORT_IO_SIZE;
    while(uartAvail>0)
    {
        auto tail=rxRingBuff.GetTail();
        auto szToWrite=static_cast<unsigned int>(uartAvail)>tail.maxSz?tail.maxSz:static_cast<unsigned int>(uartAvail);
        if(szToWrite<1)
            return; //nothing to write
        uart->write(tail.buffer,szToWrite);
        rxRingBuff.Commit(tail,szToWrite);
        uartAvail-=szToWrite;
    }
}

void UARTWorker::FillTXBuff(bool reset)
{
    if(reset)
        txUsedSz=0;
    if(!IS_OPEN(curMode))
        return;
    size_t sz=DATA_PAYLOAD_SIZE-txUsedSz;
    //limit uart-read bandwidth
    if(sz>PORT_IO_SIZE)
        sz=PORT_IO_SIZE;
    if(sz<1)
        return;
    txUsedSz+=uart->readBytes(txDataBuff+txUsedSz,sz);
    return;
}

Response UARTWorker::ProcessTX()
{
    if(txUsedSz>0)
        return Response{RespType::Data,static_cast<uint8_t>(rxRingBuff.IsHalfUsed()<<7|(sessionId&0x7F)),static_cast<uint8_t>(txUsedSz)};
    return Response{RespType::NoCommand,static_cast<uint8_t>(rxRingBuff.IsHalfUsed()<<7|(sessionId&0x7F)),0};
}
