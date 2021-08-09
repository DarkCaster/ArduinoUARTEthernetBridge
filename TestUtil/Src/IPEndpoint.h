#ifndef IPENDPOINT_H
#define IPENDPOINT_H

#include "IPAddress.h"

#include <cstdint>
#include <iostream>
#include <sys/socket.h>

class IPEndpoint
{
    public:
        IPEndpoint();
        IPEndpoint(const IPAddress &address, const uint16_t port);
        IPEndpoint(const sockaddr * const sa);
        IPEndpoint(const IPEndpoint &other);

        const IPAddress address;
        const uint16_t port;

        int ToRawBuff(void * const target) const;
        size_t GetHashCode() const;
        bool Equals(const IPEndpoint &other) const;
        bool Less(const IPEndpoint &other) const;
        bool Greater(const IPEndpoint &other) const;
        bool operator<(const IPEndpoint &other) const;
        bool operator==(const IPEndpoint &other) const;
        bool operator>(const IPEndpoint &other) const;
        bool operator>=(const IPEndpoint &other) const;
        bool operator<=(const IPEndpoint &other) const;
        friend std::ostream& operator<<(std::ostream& stream, const IPEndpoint& target);
};

namespace std { template<> struct hash<IPEndpoint>{ size_t operator()(const IPEndpoint &target) const {return target.GetHashCode();}}; }

#endif //IPENDPOINT_H
