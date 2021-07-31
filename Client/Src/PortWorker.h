#ifndef PORTWORKER_H
#define PORTWORKER_H

#include "ILogger.h"
#include "WorkerBase.h"
#include "IMessageSender.h"
#include "IMessageSubscriber.h"
#include "Config.h"
#include "RemoteConfig.h"
#include "Command.h"
#include "Connection.h"

#include <memory>
#include <atomic>
#include <mutex>

class PortWorker :  public WorkerBase, public IMessageSubscriber
{
    private:
        std::shared_ptr<ILogger> logger;
        IMessageSender& sender;
        const IConfig& config;
        const RemoteConfig& portConfig;
        const int bufferLimit;
    private:
        std::atomic<bool> shutdownPending;
        std::atomic<bool> connected;
        bool openPending;
        //params shared between OnPortOpen, ProcessTX, ProcessRX, and Worker threads
        std::mutex clientLock;
        std::shared_ptr<Connection> client;
        bool resetPending;
        uint8_t sessionId;
        int remoteBufferFillup;
    public:
        PortWorker(std::shared_ptr<ILogger>& logger, IMessageSender& sender, const IConfig& config, const RemoteConfig& portConfig);
        Request ProcessTX(uint8_t * txBuff);
        void ProcessRX(const Response& response, const uint8_t* rxBuff);
        //methods for ISubscriber
        bool ReadyForMessage(const MsgType msgType) final;
        void OnMessage(const void* const source, const IMessage& message) final;
        void OnPortOpen(const IPortOpenMessage& message);
        void OnConnected(const IConnectedMessage&);
    protected:
        //WorkerBase
        void Worker() final;
        void OnShutdown() final;
};

#endif // PORTWORKER_H
