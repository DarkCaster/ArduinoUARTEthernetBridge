#include "IPEndpoint.h"
#include <arpa/inet.h>
#include <cstring>

IPEndpoint::IPEndpoint():
    address(IPAddress()),
    port(0)
{
}

IPEndpoint::IPEndpoint(const IPAddress &_address, const uint16_t _port):
    address(_address),
    port(_port)
{
}

uint16_t DecodePort(const sockaddr * const sa)
{
    if(sa->sa_family==AF_INET6)
    {
        sockaddr_in6 sa_in6;
        std::memcpy(reinterpret_cast<void*>(&sa_in6),reinterpret_cast<const void*>(sa),sizeof(sockaddr_in6));
        return sa_in6.sin6_port;
    }
    sockaddr_in sa_in;
    std::memcpy(reinterpret_cast<void*>(&sa_in),reinterpret_cast<const void*>(sa),sizeof(sockaddr_in));
    return sa_in.sin_port;
}

IPEndpoint::IPEndpoint(const sockaddr* const sa):
    address(sa),
    port(DecodePort(sa))
{
}

IPEndpoint::IPEndpoint(const IPEndpoint& other):
    address(other.address),
    port(other.port)
{
}

int IPEndpoint::ToRawBuff(void* const target) const
{
    auto addrLen=address.isV6?IPV6_ADDR_LEN:IPV4_ADDR_LEN;
    address.ToRawBuff(target);
    auto nsPort=htons(port);
    memcpy(reinterpret_cast<char*>(target)+addrLen,reinterpret_cast<void*>(&nsPort),sizeof(nsPort));
    return static_cast<int>(addrLen+sizeof(nsPort));
}

size_t IPEndpoint::GetHashCode() const
{
    return address.GetHashCode() ^ static_cast<size_t>(port);
}

bool IPEndpoint::Equals(const IPEndpoint& other) const
{
    return address.Equals(other.address) && port==other.port;
}

bool IPEndpoint::Less(const IPEndpoint& other) const
{
    if(address.Less(other.address))
        return true;
    if(port<other.port)
        return true;
    return false;
}

bool IPEndpoint::Greater(const IPEndpoint& other) const
{
    if(address.Greater(other.address))
        return true;
    if(port>other.port)
        return true;
    return false;
}

bool IPEndpoint::operator<(const IPEndpoint& other) const
{
    return Less(other);
}

bool IPEndpoint::operator==(const IPEndpoint& other) const
{
    return Equals(other);
}

bool IPEndpoint::operator>(const IPEndpoint& other) const
{
    return Greater(other);
}

bool IPEndpoint::operator>=(const IPEndpoint& other) const
{
    return Greater(other)||Equals(other);
}

bool IPEndpoint::operator<=(const IPEndpoint& other) const
{
    return Less(other)||Equals(other);
}

std::ostream& operator<<(std::ostream& stream, const IPEndpoint& target)
{
    stream << target.address << ":" << target.port;
    return stream;
}

