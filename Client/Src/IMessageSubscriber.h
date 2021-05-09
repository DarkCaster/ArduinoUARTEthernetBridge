#ifndef IMESSAGESUBSCRIBER_H
#define IMESSAGESUBSCRIBER_H

#include "IMessage.h"

class IMessageSubscriber
{
    public:
        virtual bool ReadyForMessage(const MsgType msgType) = 0;
        virtual void OnMessage(const void* const source, const IMessage &message) = 0;
};

#endif // IMESSAGESUBSCRIBER_H
