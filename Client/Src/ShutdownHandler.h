#ifndef SHUTDOWNHANDLER_H
#define SHUTDOWNHANDLER_H

#include "IMessageSubscriber.h"
#include "IMessage.h"

#include <atomic>

class ShutdownMessage: public IShutdownMessage { public: ShutdownMessage(int _ec):IShutdownMessage(_ec){} };

class ShutdownHandler final: public IMessageSubscriber
{
    private:
        std::atomic<bool> shutdownRequested;
        std::atomic<int> ec;
    public:
        ShutdownHandler();
        bool IsShutdownRequested();
        int GetEC();
        //methods for ISubscriber
        bool ReadyForMessage(const MsgType msgType) final;
        void OnMessage(const void* const source, const IMessage &message) final;
};

#endif // SHUTDOWNHANDLER_H
