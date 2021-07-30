#ifndef PORTWORKER_H
#define PORTWORKER_H

#include "ILogger.h"
#include "IMessageSender.h"
#include "IMessageSubscriber.h"
#include "Config.h"
#include "RemoteConfig.h"

class PortWorker : public IMessageSubscriber
{
    private:
        std::shared_ptr<ILogger> logger;
        IMessageSender& sender;
        const IConfig& config;
        const int portId;
        const bool resetOnConnect;
    public:
        PortWorker(std::shared_ptr<ILogger>& logger, IMessageSender& sender, const IConfig& config, const int portId, const bool resetOnConnect);
        //methods for ISubscriber
        bool ReadyForMessage(const MsgType msgType) final;
        void OnMessage(const void* const source, const IMessage& message) final;
};

#endif // PORTWORKER_H
