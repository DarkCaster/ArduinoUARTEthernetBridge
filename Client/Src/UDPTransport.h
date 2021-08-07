#ifndef UDPTRANSPORT_H
#define UDPTRANSPORT_H

#include "UDPConnection.h"
#include "IConfig.h"
#include "WorkerBase.h"
#include "ILogger.h"
#include "IMessageSender.h"
#include "IMessageSubscriber.h"

#include <memory>
#include <cstdint>
#include <atomic>

class UDPTransport final : public WorkerBase, public IMessageSubscriber
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
        std::shared_ptr<UDPConnection> remoteConn;
        uint16_t udpPort;
        size_t droppedRxSeqCnt;
    private: //service methods
        std::shared_ptr<UDPConnection> GetConnection();
        void HandleError(const std::string& message);
        void HandleError(int ec, const std::string& message);
        void OnSendPackage(const ISendPackageMessage& message);
        void OnConnected(const IConnectedMessage& message);
    public:
        UDPTransport(std::shared_ptr<ILogger>& logger, IMessageSender& sender, const IConfig& config);
        //methods for ISubscriber
        bool ReadyForMessage(const MsgType msgType) final;
        void OnMessage(const void* const source, const IMessage& message) final;
    protected:
        //WorkerBase
        void Worker() final;
        void OnShutdown() final;
};

#endif // UDPTRANSPORT_H
