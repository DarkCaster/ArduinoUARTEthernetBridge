#ifndef TCPTRANSPORT_H
#define TCPTRANSPORT_H

#include "IConfig.h"
#include "WorkerBase.h"
#include "ILogger.h"
#include "IMessageSender.h"
#include "IMessageSubscriber.h"

#include <memory>
#include <cstdint>

class TCPTransport
{
    private:
        std::shared_ptr<ILogger> logger;
        IMessageSender &sender;
        const IConfig &config; //endpoint, udp port for UDP mode
    public:
        TCPTransport(std::shared_ptr<ILogger> &logger, IMessageSender &sender, const IConfig &config);

        //establish connection with UART-brige over TCP, start reader-task, signal success
        //on reader fail - signal disconnect, schedule connection reesyablish
        //send package on request + with header and CRC setup
        //signal on receiving new package
};

#endif // TCPTRANSPORT_H
