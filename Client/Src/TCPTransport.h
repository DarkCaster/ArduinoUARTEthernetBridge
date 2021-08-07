#ifndef TCPTRANSPORT_H
#define TCPTRANSPORT_H

#include "TCPConnection.h"
#include "IConfig.h"
#include "WorkerBase.h"
#include "ILogger.h"
#include "IMessageSender.h"
#include "IMessageSubscriber.h"

#include <memory>
#include <cstdint>
#include <atomic>

class TCPTransport final : public WorkerBase, public IMessageSubscriber
{
    private: //fields setup via constructor
        std::shared_ptr<ILogger> logger;
        IMessageSender& sender;
        const IConfig& config;
        std::unique_ptr<uint8_t[]> rxBuff;
    private:
        std::atomic<bool> shutdownPending;
        //remote connection with it's management lock
        std::mutex remoteConnLock;
        std::shared_ptr<TCPConnection> remoteConn;
        uint16_t udpPort;
        //service methods
        std::shared_ptr<TCPConnection> GetConnection();
        void HandleError(const std::string& message);
        void HandleError(int ec, const std::string& message);
        void OnSendPackage(const ISendPackageMessage& message);
    public:
        TCPTransport(std::shared_ptr<ILogger>& logger, IMessageSender& sender, const IConfig& config);
        //methods for ISubscriber
        bool ReadyForMessage(const MsgType msgType) final;
        void OnMessage(const void* const source, const IMessage& message) final;
    protected:
        //WorkerBase
        void Worker() final;
        void OnShutdown() final;
};

#endif // TCPTRANSPORT_H
