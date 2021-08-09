#include "ShutdownHandler.h"

#include <csignal>
#include <unistd.h>

ShutdownHandler::ShutdownHandler()
{
    shutdownRequested.store(false);
    ec.store(0);
}

bool ShutdownHandler::IsShutdownRequested()
{
    return shutdownRequested.load();
}

int ShutdownHandler::GetEC()
{
    return ec.load();
}

bool ShutdownHandler::ReadyForMessage(const MsgType msgType)
{
    return msgType==MSG_SHUTDOWN;
}

void ShutdownHandler::OnMessage(const void* const, const IMessage& message)
{
    if(!shutdownRequested.exchange(true))
    {
        ec.store(static_cast<const IShutdownMessage&>(message).ec);
        kill(getpid(),SIGUSR2); //send SIGUSR2 signal to self, so main thread will unblock while waiting for signal
    }
}
