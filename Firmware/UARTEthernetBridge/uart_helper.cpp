#include "uart_helper.h"
#include "configuration.h"
#include "debug.h"

#define STATE_NOT_CONNECTED 0
#define STATE_CONNECTED 1
#define STATE_CONFIG 2
#define STATE_UART_OPEN 3
#define STATE_RESET_BEGIN 4
#define STATE_RESET_END 5
#define STATE_READY 128

#define STATE_CONN_FAILED 129
#define STATE_DATA_FINALIZED 130
#define STATE_DISPOSED 255

#define AWAITING_CONNECTED (state<STATE_CONNECTED)
#define AWAITING_CONFIG (state<STATE_CONFIG)
#define AWAITING_UART_OPEN (state<STATE_UART_OPEN)
#define AWAITING_RESET_BEGIN (state<STATE_RESET_BEGIN)
#define AWAITING_RESET_END (state<STATE_RESET_END)
#define AWAITING_DATA_FINALIZE (state<STATE_DATA_FINALIZED)
#define CONNECT_ROUTINE (state<STATE_READY)
#define DISCONNECT_ROUTINE (state>STATE_READY&&state<STATE_DISPOSED)

static void PrepareRXPin(uint8_t rxPin)
{
    pinMode(rxPin,INPUT_PULLUP);
}

void UARTHelper::Setup(HardwareSerial* const _uart, const uint8_t _rxPin, const uint16_t netPort)
{
    uart=_uart;
    rxPin=_rxPin;
    PrepareRXPin(rxPin);
    //TODO: set start state
    server.begin(netPort);
    state=STATE_NOT_CONNECTED;
}

bool UARTHelper::Connect()
{
    client=server.accept();
    if(client)
    {
        state=STATE_CONNECTED;
        return true;
    }
    return false;
}

bool UARTHelper::ReadConfig()
{
    if(!client.connected())
    {
        client.stop();
        state=STATE_NOT_CONNECTED;
        return false;
    }
    //just wait for needed data to arrive, assuming TCP stack can collect 5 bytes in one piece
    if(client.available()<5) //3 bytes - uart speed, 1 byte - uart config, 1 byte - extra flags (TODO)
        return true;
    //read config buffer
    uint8_t tmp[5];
    if(client.read(tmp,5)!=5)
    {
        client.stop();
        state=STATE_NOT_CONNECTED;
        return false;
    }
    //fillup config;
    config.speed=((unsigned long)tmp[0])|((unsigned long)tmp[1])<<8|((unsigned long)tmp[2])<<16;
    config.mode=tmp[3];
    config.flags=tmp[4];
    config.collectIntMS=(((unsigned long)1000000000/((unsigned long)config.speed/(unsigned long)8)*(unsigned long)UART_BUFFER_SIZE)/(unsigned long)1000000);
    if(!CFG_FAKE_UART_MODE(config))
        config.collectIntMS/=2;
    state=STATE_CONFIG;
    return true;
}

bool UARTHelper::UARTOpen()
{
    if(!CFG_FAKE_UART_MODE(config))
        uart->begin(config.speed,config.mode);
    state=STATE_UART_OPEN;
    return true;
}

bool UARTHelper::ResetBegin()
{
    if(CFG_FAKE_UART_MODE(config))
    {
        state=STATE_RESET_END;
        return true;
    }
    //TODO: begin reset
    state=STATE_RESET_BEGIN;
    return true;
}

bool UARTHelper::ResetEnd()
{
    //TODO: end reset
    state=STATE_RESET_END;
    return true;
}

//receive data incoming from TCP client
bool UARTHelper::RXStep1()
{
    if(CONNECT_ROUTINE)
    {
        if(AWAITING_CONNECTED)
            return Connect();
        if(AWAITING_CONFIG)
            return ReadConfig();
        if(AWAITING_UART_OPEN)
            return UARTOpen();
        if(AWAITING_RESET_BEGIN)
            return ResetBegin();
        if(AWAITING_RESET_END)
            return ResetEnd();
        STATUS(); LOG(F("Client connected, config complete!"));
        //TODO: more configuration steps
        state=STATE_READY;
    }

    if(DISCONNECT_ROUTINE)
    {
        //just wait for all buffered data to finalize
        if(AWAITING_DATA_FINALIZE)
            return true;
        //we may try to connect again from here
        state=STATE_NOT_CONNECTED;
        return false;
    }

    //read some data to backbuffer if we hame some space
    auto segment=rxStorage.GetFreeSegment();
    if(segment!=nullptr) //nullptr if no free segments available -> awaiting for uart to be available for write
    {
        auto avail=client.available();
        if(avail>0)
        {
            if(avail>UART_BUFFER_SIZE)
                avail=UART_BUFFER_SIZE;
            segment->usedSize=(client.read(segment->buffer,avail));
            //commit segment only if we successfully read some data
            if(segment->usedSize>0)
            {
                rxStorage.CommitSegment(segment);
                return true;
            }
        }
        if(!client.connected())
        {
            state=STATE_CONN_FAILED;
            return true;
        }
    }
    return true;
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
