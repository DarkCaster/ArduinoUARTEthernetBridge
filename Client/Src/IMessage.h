#ifndef IMESSAGE_H
#define IMESSAGE_H

#include "RemoteConfig.h"
#include "Connection.h"
#include <memory>

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
        INewClientMessage(const std::shared_ptr<Connection> &_client, const int _pathID):
            IMessage(MSG_NEW_CLIENT),client(_client),pathID(_pathID){}
    public:
        const std::shared_ptr<Connection> client;
        const int pathID;
};

#endif // IMESSAGE_H
