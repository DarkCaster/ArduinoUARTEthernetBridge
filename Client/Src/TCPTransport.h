#ifndef TCPTRANSPORT_H
#define TCPTRANSPORT_H

#include "Connection.h"
#include "IConfig.h"
#include "WorkerBase.h"
#include "ILogger.h"
#include "IMessageSender.h"
#include "IMessageSubscriber.h"

#include <memory>
#include <cstdint>
#include <atomic>

class TCPTransport final : public WorkerBase
{
    private: //fields setup via constructor
        std::shared_ptr<ILogger> logger;
        IMessageSender& sender;
        const IConfig& config; //endpoint, udp port for UDP mode
        uint8_t* rxBuff;
        uint8_t* txBuff;
    private:
        std::atomic<bool> shutdownPending;
        //remote connection with it's management lock
        std::recursive_mutex remoteConnLock;
        std::shared_ptr<Connection> remoteConn;
        //service methods
        std::shared_ptr<Connection> GetConnection();
        void HandleError(const std::string& message);
        void HandleError(int ec, const std::string& message);
    public:
        TCPTransport(std::shared_ptr<ILogger>& logger, IMessageSender& sender, const IConfig& config, uint8_t* rxBuff, uint8_t* txBuff);
        void ProcessTX(); //called by timer, to process data sending
    protected:
        //WorkerBase
        void Worker() final;
        void OnShutdown() final;
};

#endif // TCPTRANSPORT_H
