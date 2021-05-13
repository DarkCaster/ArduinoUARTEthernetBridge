#ifndef CONNECTIONWORKER_H
#define CONNECTIONWORKER_H

#include "ILogger.h"
#include "IConfig.h"
#include "RemoteConfig.h"
#include "IMessageSender.h"
#include "IMessageSubscriber.h"
#include "WorkerBase.h"
#include "Connection.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>

class ConnectionWorker final : public WorkerBase, public IMessageSubscriber
{
    private:
        std::shared_ptr<ILogger> logger;
        IMessageSender &sender;
        const IConfig &config;
        const RemoteConfig &remoteConfig;
        const int pathID;
        const int isReader;
        std::condition_variable newPathTrigger;
        std::mutex opLock;
        std::atomic<bool> shutdownPending;
        std::shared_ptr<Connection> remote;
        std::shared_ptr<Connection> local;
        void HandleError(const std::string& message);
        void HandleError(int ec, const std::string& message);
    public:
        ConnectionWorker(std::shared_ptr<ILogger> &logger, IMessageSender &sender, const IConfig &config, const RemoteConfig &remoteConfig, int pathID, bool reader);
        //IMessageSubscriber
        bool ReadyForMessage(const MsgType msgType) final;
        void OnMessage(const void* const source, const IMessage &message) final;
    protected:
        //WorkerBase
        void Worker() final;
        void OnShutdown() final;
};

#endif // CONNECTIONWORKER_H
