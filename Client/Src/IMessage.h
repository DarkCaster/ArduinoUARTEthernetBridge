#ifndef IMESSAGE_H
#define IMESSAGE_H

enum MsgType
{
    MSG_SHUTDOWN,
    MSG_STARTUP_READY,
    MSG_STARTUP_CONTINUE,
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

class IStartupReadyMessage : public IMessage
{
    protected:
        IStartupReadyMessage():IMessage(MSG_STARTUP_READY){}
};

class IStartupContinueMessage : public IMessage
{
    protected:
        IStartupContinueMessage():IMessage(MSG_STARTUP_CONTINUE){}
};

#endif // IMESSAGE_H
