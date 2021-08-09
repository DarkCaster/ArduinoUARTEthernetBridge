#ifndef IMESSAGESENDER_H
#define IMESSAGESENDER_H

#include "IMessage.h"

class IMessageSender
{
    public:
        virtual void SendMessage(const void * const source, const IMessage &message) = 0;
};

#endif // IMESSAGESENDER_H

