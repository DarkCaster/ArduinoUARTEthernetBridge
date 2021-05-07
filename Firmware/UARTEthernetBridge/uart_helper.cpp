#include "uart_helper.h"
#include "configuration.h"

#define GET_UART_COLLECT_TIME(speed, bsz) (((uint)1000000000/(uint)(speed/8)*(uint)bsz)/(uint)1000000)

void UARTHelper::PrepareRXPin()
{
    pinMode(rxPin,INPUT_PULLUP);
}

void UARTHelper::Setup(HardwareSerial* const _uart, const uint8_t _rxPin, const uint16_t netPort)
{
    uart=_uart;
    rxPin=_rxPin;
    PrepareRXPin();
    //TODO: set start state
    server.begin(netPort);
}

bool UARTHelper::RXStep1()
{
    return false;
}

void UARTHelper::RXStep2()
{

}

void UARTHelper::TXStep1(unsigned long curTime)
{

}

void UARTHelper::TXStep2()
{

}

/*
 *
 * uint8_t read_request()
{
    //we are still processing current request and cannot read another one
    if(reqCurrent!=REQ_NONE)
        return reqCurrent;

    auto avail=remote.available();

    //if no data available for reading - check connection
    if(avail<1)
    {
        if(!remote.connected())
            return remote_fail();
        return REQ_NONE;
    }

    //start reading new package if not in the middle of another package
    //if(avail>0 && )
    //{
        //read header
        if(remote.read(&reqPending,1)<1)
            return remote_fail();
        //set remaining length
        switch (reqPending)
        {
            case REQ_CONNECT:
                reqLen=REQ_CONNECT_LEN;
                break;
            case REQ_DISCONNECT:
                reqLen=REQ_DISCONNECT_LEN;
                break;
            case REQ_DATA:
                reqLen=REQ_DATA_LEN;
                break;
            case REQ_PING:
                reqLen=REQ_PING_LEN;
                break;
            case REQ_WD:
                reqLen=REQ_WD_LEN;
                break;
            default:
                return remote_fail();
        }
        reqLeft=reqLen;
        avail--;
    //}

    if(avail>reqLeft)
        avail=reqLeft;

    if(reqLeft>0)
        reqLeft-=remote.read(reqBuff+reqLen-reqLeft,avail);

    if(reqLeft<1)
    {
        auto result=reqPending;
        reqPending=REQ_NONE;
        return result;
    }

    return REQ_NONE;
}
*/
