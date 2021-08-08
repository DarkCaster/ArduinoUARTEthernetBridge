#include "RemoteBufferTracker.h"
#include "IMessage.h"

RemoteBufferTracker::RemoteBufferTracker(std::shared_ptr<ILogger>& _logger, const IConfig& _config, const size_t _bufferLimit):
    logger(_logger),
    config(_config),
    bufferLimit(_bufferLimit)
{
    counter=0;
    isBlocked=false;
}

void RemoteBufferTracker::AddPackage(size_t size)
{
    dataSent.push_back(RemoteBufferPkg{counter,size});
}

void RemoteBufferTracker::ConfirmPackage(uint32_t refCounter, bool writeBlocked)
{
    isBlocked=writeBlocked;
    while (!dataSent.empty() && dataSent.front().counter<=refCounter)
        dataSent.pop_front();
}

size_t RemoteBufferTracker::GetAvailSpace()
{
    if(isBlocked)
        return 0;
    size_t totalSent=0;
    for(const auto &pkg:dataSent)
        totalSent+=pkg.dataSize;
    if(totalSent>bufferLimit)
        return 0;
    return bufferLimit-totalSent;
}

void RemoteBufferTracker::Reset()
{
    dataSent.clear();
}

bool RemoteBufferTracker::ReadyForMessage(const MsgType msgType)
{
    return msgType==MSG_TIMER;
}

void RemoteBufferTracker::OnMessage(const void* const, const IMessage& message)
{
    counter=static_cast<const ITimerMessage&>(message).counter;
}
