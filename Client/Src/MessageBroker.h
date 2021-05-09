#ifndef MESSAGEBROKER_H
#define MESSAGEBROKER_H

#include "IMessageSender.h"
#include "IMessageSubscriber.h"
#include "IMessage.h"
#include "ILogger.h"

#ifdef NDEBUG
#include "ImmutableStorage.h"
#else
#include <thread>
#include <map>
#endif

#include <mutex>
#include <set>
#include <memory>

class MessageBroker : public IMessageSender
{
    private:
        std::mutex opLock;
#ifdef NDEBUG
        ImmutableStorage<std::set<IMessageSubscriber*>> curSubs;
#else
        std::set<IMessageSubscriber*> subscribers;
        std::map<std::thread::id,std::set<const void*>*> callers;
#endif
        std::shared_ptr<ILogger> logger;
    public:
        MessageBroker(std::shared_ptr<ILogger> &_logger);
        void AddSubscriber(IMessageSubscriber &subscriber);
        void SendMessage(const void* const source, const IMessage &message) final;
};

#endif // MESSAGEBROKER_H
