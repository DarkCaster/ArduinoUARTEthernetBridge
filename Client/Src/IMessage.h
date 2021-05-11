#ifndef IMESSAGE_H
#define IMESSAGE_H

#include "RemoteConfig.h"

enum MsgType
{
    MSG_SHUTDOWN,
    MSG_NEW_CLIENT,
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

class INewClientMessage : public IMessage
{
    protected:
        INewClientMessage(int _fd, const RemoteConfig &_remoteConfig):IMessage(MSG_NEW_CLIENT),fd(_fd),remoteConfig(&_remoteConfig){}
    public:
        const int fd;
        const RemoteConfig * const remoteConfig;
};

#endif // IMESSAGE_H
