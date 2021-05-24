#ifndef PTYLISTENER_H
#define PTYLISTENER_H

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

class PTYListener final : public WorkerBase
{
    private:
        std::shared_ptr<ILogger> logger;
        IMessageSender &sender;
        const IConfig &config;
        const RemoteConfig &remoteConfig;
        const int pathID;
        std::atomic<bool> shutdownPending;
        int ptm, pts;
        void HandleError(const std::string &message);
        void HandleError(int ec, const std::string &message);
    public:
        PTYListener(std::shared_ptr<ILogger> &logger, IMessageSender &sender, const IConfig &config, const RemoteConfig &remoteConfig, int pathID);
        //WorkerBase
        bool Startup() final;
    protected:
        void Worker() final;
        void OnShutdown() final;
};
#endif // PTYLISTENER_H
