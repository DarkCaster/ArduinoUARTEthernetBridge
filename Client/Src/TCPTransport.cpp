#include "TCPTransport.h"

#include <cstring>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <netdb.h>

TCPTransport::TCPTransport(std::shared_ptr<ILogger>& _logger, IMessageSender& _sender, const IConfig& _config):
    logger(_logger),
    sender(_sender),
    config(_config)
{
}

static IPAddress Lookup(std::shared_ptr<ILogger> &logger, const std::string &target)
{
    addrinfo hints={};
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;

    addrinfo *addrResults=nullptr;
    if(getaddrinfo (target.c_str(), NULL, &hints, &addrResults)!=0)
    {
        logger->Warning()<<"Lookup of "<<target<<" is failed with: "<<strerror(errno);
        return IPAddress();
    }

    IPAddress result(addrResults->ai_addr);
    freeaddrinfo(addrResults);
    return result;
}
