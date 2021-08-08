#ifndef IMESSAGE_H
#define IMESSAGE_H

#include "PortConfig.h"
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
        IConnectedMessage(const uint16_t _udpPort):IMessage(MSG_CONNECTED),udpPort(_udpPort){}
    public:
        const uint16_t udpPort;
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
        ITimerMessage(uint32_t _counter):
            IMessage(MSG_TIMER),counter(_counter){}
    public:
        const uint32_t counter;
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
