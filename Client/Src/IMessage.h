#ifndef IMESSAGE_H
#define IMESSAGE_H

#include "RemoteConfig.h"
#include "Connection.h"
#include <memory>

enum MsgType
{
    MSG_SHUTDOWN,
    MSG_CONNECTED,
    MSG_INCOMING_PACKAGE,
    MSG_TIMER,
    MSG_SEND_PACKAGE,
    MSG_PORT_OPEN,
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

class ISendPackageMessage : public IMessage
{
    protected:
        ISendPackageMessage(const bool _useTCP, uint8_t* const _package):
            IMessage(MSG_SEND_PACKAGE),useTCP(_useTCP),package(_package){}
    public:
        const bool useTCP;
        uint8_t * const package;
};

class IPortOpenMessage : public IMessage
{
    protected:
        IPortOpenMessage(std::shared_ptr<Connection>& _connection):
            IMessage(MSG_PORT_OPEN),connection(_connection){}
    public:
        std::shared_ptr<Connection>& connection;
};

#endif // IMESSAGE_H
