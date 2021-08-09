#ifndef IPADDRESS_H
#define IPADDRESS_H

#include <iostream>
#include <string>

#include <linux/rtnetlink.h>
#include <sys/socket.h>

//TODO: re-use defines from system include files
#define IPV4_ADDR_LEN 4
#define IPV6_ADDR_LEN 16
#define IP_ADDR_LEN IPV6_ADDR_LEN

class IPAddress
{
    public:
        IPAddress();
        IPAddress(const sockaddr * const sa);
        IPAddress(const IPAddress &other);
        IPAddress(const rtattr * const rta);
        IPAddress(const void * const raw, const size_t len);
        IPAddress(const std::string &string);

        void ToSA(void * const targetSA) const;
        void ToRawBuff(void * const target) const;
        const void * RawData() const;
        std::string ToString() const;

        size_t GetHashCode() const;
        bool Equals(const IPAddress &other) const;
        bool Less(const IPAddress &other) const;
        bool Greater(const IPAddress &other) const;
        bool operator<(const IPAddress &other) const;
        bool operator==(const IPAddress &other) const;
        bool operator>(const IPAddress &other) const;
        bool operator>=(const IPAddress &other) const;
        bool operator<=(const IPAddress &other) const;

        friend std::ostream& operator<<(std::ostream& stream, const IPAddress& target);

        const bool isValid;
        const bool isV6;
    private:
        struct RawIP
        {
            RawIP();
            RawIP(const sockaddr* const sa);
            RawIP(const void * const source, const size_t len);
            RawIP(int af, const char * const ip);
            unsigned char data[IP_ADDR_LEN];
        };
        const RawIP ip;
};

namespace std { template<> struct hash<IPAddress>{ size_t operator()(const IPAddress &target) const {return target.GetHashCode();}}; }

#endif // IPADDRESS_H
