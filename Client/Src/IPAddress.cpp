#include "IPAddress.h"

#include <cstring>
#include <arpa/inet.h>

static int DetectIPVer(const char* const ip)
{
    unsigned char tmp[IP_ADDR_LEN]={};
    auto check=inet_pton(AF_INET,ip,reinterpret_cast<void*>(tmp));
    if(check>0)
        return 1;
    check=inet_pton(AF_INET6,ip,reinterpret_cast<void*>(tmp));
    if(check>0)
        return 2;
    return 0;
}

IPAddress::RawIP::RawIP()
{
    std::memset(reinterpret_cast<void*>(data),0,IP_ADDR_LEN);
}

IPAddress::RawIP::RawIP(const sockaddr* const sa) : RawIP()
{
    if(sa->sa_family==AF_INET6)
    {
        sockaddr_in6 sa_in6;
        std::memcpy(reinterpret_cast<void*>(&sa_in6),reinterpret_cast<const void*>(sa),sizeof(sockaddr_in6));
        std::memcpy(reinterpret_cast<void*>(data),reinterpret_cast<const void*>(&(sa_in6.sin6_addr)),IPV6_ADDR_LEN);
    }
    else
    {
        sockaddr_in sa_in;
        std::memcpy(reinterpret_cast<void*>(&sa_in),reinterpret_cast<const void*>(sa),sizeof(sockaddr_in));
        std::memcpy(reinterpret_cast<void*>(data),reinterpret_cast<const void*>(&(sa_in.sin_addr)),IPV4_ADDR_LEN);
    }
}

IPAddress::RawIP::RawIP(const void* const source, const size_t len) : RawIP()
{
    std::memcpy(reinterpret_cast<void*>(data),source,len>IP_ADDR_LEN?IP_ADDR_LEN:len);
}

IPAddress::RawIP::RawIP(int af, const char * const ip) : RawIP()
{
    inet_pton(af,ip,reinterpret_cast<void*>(data));
}

IPAddress::IPAddress():
    isValid(false),
    isV6(false),
    ip(RawIP())
{
}

IPAddress::IPAddress(const sockaddr* const sa):
    isValid(sa->sa_family==AF_INET||sa->sa_family==AF_INET6),
    isV6(sa->sa_family==AF_INET6),
    ip(!isValid?RawIP():RawIP(sa))
{
}

IPAddress::IPAddress(const IPAddress& other):
    isValid(other.isValid),
    isV6(other.isV6),
    ip(other.ip)
{
}

IPAddress::IPAddress(const rtattr* const rta):
    isValid(RTA_PAYLOAD(rta)==IPV6_ADDR_LEN || RTA_PAYLOAD(rta)==IPV4_ADDR_LEN),
    isV6(RTA_PAYLOAD(rta)==IPV6_ADDR_LEN),
    ip(!isValid?RawIP():RawIP(RTA_DATA(rta),RTA_PAYLOAD(rta)))
{
}

IPAddress::IPAddress(const void * const raw, const size_t len):
    isValid(len==IPV6_ADDR_LEN || len==IPV4_ADDR_LEN),
    isV6(len==IPV6_ADDR_LEN),
    ip(!isValid?RawIP():RawIP(raw,len))
{
}

IPAddress::IPAddress(const std::string &string):
    isValid(DetectIPVer(string.c_str())>0),
    isV6(isValid?DetectIPVer(string.c_str())==2:false),
    ip(!isValid?RawIP():RawIP(isV6?AF_INET6:AF_INET,string.c_str()))
{
}

void IPAddress::ToSA(void* const targetSA) const
{
    if(!isValid)
        return;
    if(isV6)
    {
        auto target=reinterpret_cast<sockaddr_in6*>(targetSA);
        target->sin6_family=AF_INET6;
        std::memcpy(reinterpret_cast<void*>(&(target->sin6_addr)),reinterpret_cast<const void*>(ip.data),IPV6_ADDR_LEN);
    }
    else
    {
        auto target=reinterpret_cast<sockaddr_in*>(targetSA);
        target->sin_family=AF_INET;
        std::memcpy(reinterpret_cast<void*>(&(target->sin_addr)),reinterpret_cast<const void*>(ip.data),IPV4_ADDR_LEN);
    }
}

void IPAddress::ToRawBuff(void* const target) const
{
    std::memcpy(target,reinterpret_cast<const void*>(ip.data),isV6?IPV6_ADDR_LEN:IPV4_ADDR_LEN);
}

const void * IPAddress::RawData() const
{
    return ip.data;
}

std::string IPAddress::ToString() const
{
    auto resultLen=isV6?INET6_ADDRSTRLEN:INET_ADDRSTRLEN;
    char result[INET6_ADDRSTRLEN>INET_ADDRSTRLEN?INET6_ADDRSTRLEN:INET_ADDRSTRLEN];
    inet_ntop(isV6?AF_INET6:AF_INET, ip.data, result, resultLen);
    return std::string(result);
}

size_t IPAddress::GetHashCode() const
{
    int rSz=sizeof(size_t);
    size_t result={};
    result^=isValid;
    result^=(isV6<<8);
    int shift=rSz-1;
    for(auto i=0;i<IP_ADDR_LEN;++i)
    {
        result^=((static_cast<size_t>(ip.data[i]))<<(8*shift));
        shift--;
        if(shift<0)
            shift=rSz-1;
    }
    return result;
}

bool IPAddress::Equals(const IPAddress& other) const
{
    return isValid==other.isValid && isV6==other.isV6 && std::memcmp(ip.data,other.ip.data,IP_ADDR_LEN)==0;
}

bool IPAddress::Less(const IPAddress& other) const
{
    auto firstFlags=10*isValid+1*isV6;
    auto secondFlags=10*other.isValid+1*other.isV6;
    //first, compare the flags
    if(firstFlags<secondFlags)
        return true;
    //second, compare IP
    if(firstFlags==secondFlags&&std::memcmp(ip.data,other.ip.data,IP_ADDR_LEN)<0)
        return true;
    return false;
}

bool IPAddress::Greater(const IPAddress& other) const
{
    auto firstFlags=10*isValid+1*isV6;
    auto secondFlags=10*other.isValid+1*other.isV6;
    //first, compare the flags
    if(firstFlags>secondFlags)
        return true;
    //second, compare IP
    if(firstFlags==secondFlags&&std::memcmp(ip.data,other.ip.data,IP_ADDR_LEN)>0)
        return true;
    return false;
}

bool IPAddress::operator<(const IPAddress &other) const
{
    return Less(other);
}

bool IPAddress::operator==(const IPAddress& other) const
{
    return Equals(other);
}

bool IPAddress::operator>(const IPAddress& other) const
{
    return Greater(other);
}

bool IPAddress::operator>=(const IPAddress& other) const
{
    return Greater(other)||Equals(other);
}

bool IPAddress::operator<=(const IPAddress& other) const
{
    return Less(other)||Equals(other);
}

std::ostream& operator<<(std::ostream& stream, const IPAddress& target)
{
    auto resultLen=target.isV6?INET6_ADDRSTRLEN:INET_ADDRSTRLEN;
    char result[INET6_ADDRSTRLEN>INET_ADDRSTRLEN?INET6_ADDRSTRLEN:INET_ADDRSTRLEN];
    inet_ntop(target.isV6?AF_INET6:AF_INET, target.ip.data, result, resultLen);
    stream << result;
    return stream;
}

