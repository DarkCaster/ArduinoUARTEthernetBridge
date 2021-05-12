#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "RemoteConfig.h"
#include "IConfig.h"
#include "WorkerBase.h"
#include "ILogger.h"
#include "IMessageSender.h"
#include "IMessageSubscriber.h"
#include "IPEndpoint.h"

#include <string>
#include <atomic>
#include <sys/time.h>

class TCPClient final : public WorkerBase, public IMessageSubscriber
{
    private:
        std::shared_ptr<ILogger> logger;
        IMessageSender &sender;
        const bool connectOnStart;
        const IConfig &config;
        const RemoteConfig &remoteConfig;
        const int pathID;
        std::atomic<bool> shutdownPending;
        void HandleError(const std::string& message);
        void HandleError(int ec, const std::string& message);
    public:
        TCPClient(std::shared_ptr<ILogger> &logger, IMessageSender &sender, const bool connectOnStart, const IConfig &config,  const RemoteConfig &remoteConfig, const int pathID);
        //IMessageSubscriber
        bool ReadyForMessage(const MsgType msgType) final;
        void OnMessage(const void* const source, const IMessage &message) final;
    protected:
        //WorkerBase
        void Worker() final;
        void OnShutdown() final;
};

#endif //TCP_CLIENT_H
