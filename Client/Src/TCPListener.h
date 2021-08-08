#ifndef TCP_LISTENER_H
#define TCP_LISTENER_H

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

class TCPListener final : public WorkerBase
{
    private:
        std::shared_ptr<ILogger> logger;
        IMessageSender &sender;
        const IConfig &config;
        const PortConfig &remoteConfig;
        std::atomic<bool> shutdownPending;
        void HandleError(const std::string& message);
        void HandleError(int ec, const std::string& message);
    public:
        TCPListener(std::shared_ptr<ILogger> &logger, IMessageSender &sender, const IConfig &config,  const PortConfig &remoteConfig);
    protected:
        //WorkerBase
        void Worker() final;
        void OnShutdown() final;
};

#endif //TCP_LISTENER_H
