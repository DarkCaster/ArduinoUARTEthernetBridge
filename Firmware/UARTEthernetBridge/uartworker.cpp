#include "uartworker.h"

void UARTWorker::Setup(ResetHelper* const _resetHelper, HardwareSerial* const _uart, uint8_t const * _rxDataBuff, uint8_t const * _txDataBuff)
{
    resetHelper=_resetHelper;
    uart=_uart;
    rxDataBuff=_rxDataBuff;
    txDataBuff=_txDataBuff;
}

unsigned long UARTWorker::ProcessRequest(const Request &request)
{
    return 1000;
}

void UARTWorker::ProcessRX()
{
}

Response UARTWorker::ProcessTX()
{
    return Response();
}
