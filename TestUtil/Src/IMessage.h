#ifndef IMESSAGE_H
#define IMESSAGE_H

#include <memory>

enum MsgType
{
    MSG_SHUTDOWN,
};

class IMessage
{
    protected:
        IMessage(const MsgType _msgType):msgType(_msgType){};
    public:
        const MsgType msgType;
};

class IShutdownMessage : public IMessage
{
    protected:
        IShutdownMessage(int _ec):IMessage(MSG_SHUTDOWN),ec(_ec){}
    public:
        const int ec;
};

#endif // IMESSAGE_H
