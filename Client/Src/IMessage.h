#ifndef IMESSAGE_H
#define IMESSAGE_H

#include "RemoteConfig.h"
#include "Connection.h"
#include <memory>

enum MsgType
{
    MSG_SHUTDOWN,
    MSG_NEW_CLIENT,
    MSG_PATH_ESTABLISHED,
    MSG_PATH_COLLAPSED,

    //TODO:
    MSG_CONNECTED,
    MSG_INCOMING_PACKAGE,
    MSG_TIMER,
    //MSG_SERVER_CONN_LOST

};

class IMessage
{
    protected:
        IMessage(const MsgType _msgType):msgType(_msgType){};
    public:
        const MsgType msgType;
};

class IConnectedMessage : public IMessage
{
    protected:
        IConnectedMessage():IMessage(MSG_CONNECTED){}
};

class IIncomingPackageMessage : public IMessage
{
    protected:
        IIncomingPackageMessage(const uint8_t* const _package):
            IMessage(MSG_INCOMING_PACKAGE),package(_package){}
    public:
        const uint8_t * const package;
};

class ITimerMessage : public IMessage
{
    protected:
        ITimerMessage(int64_t _interval):
            IMessage(MSG_TIMER),interval(_interval){}
    public:
        const int64_t interval;
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

class IPathEstablishedMessage : public IMessage
{
    protected:
        IPathEstablishedMessage(const std::shared_ptr<Connection> &_local, const std::shared_ptr<Connection> &_remote, const int _pathID):
            IMessage(MSG_PATH_ESTABLISHED),local(_local),remote(_remote),pathID(_pathID){}
    public:
        const std::shared_ptr<Connection> local;
        const std::shared_ptr<Connection> remote;
        const int pathID;
};

class IPathCollapsedMessage : public IMessage
{
    protected:
        IPathCollapsedMessage(const std::shared_ptr<Connection> &_local, const std::shared_ptr<Connection> &_remote, const int _pathID):
            IMessage(MSG_PATH_COLLAPSED),local(_local),remote(_remote),pathID(_pathID){}
    public:
        const std::shared_ptr<Connection> local;
        const std::shared_ptr<Connection> remote;
        const int pathID;
};

#endif // IMESSAGE_H
