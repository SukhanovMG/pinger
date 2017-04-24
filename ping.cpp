#include "ping.h"

#include <iostream>

#include <memory>
#include <netdb.h> // for getaddrinfo, etc..
#include <netinet/in.h> // IPPROTO_ICMP
#include <netinet/ip_icmp.h>
#include <sys/socket.h> // socket
#include <unistd.h> // close

#include <stdexcept>

using namespace std;

struct struct_addrinfo_deleter
{
    void operator () (struct addrinfo* ai)
    {
        freeaddrinfo(ai);
    }
};

typedef unique_ptr<struct addrinfo, struct_addrinfo_deleter> up_addrinfo;

Socket::Socket(int domain, int type, int protocol, unsigned snd_timeout, unsigned rcv_timeout) :
    m_socket_fd(socket(domain, type, protocol))
{
    if (m_socket_fd < 0)
        throw runtime_error("Failed to create socket.");

    struct timeval to;
    to.tv_sec = snd_timeout;
    to.tv_usec = 0;
    if (setsockopt(m_socket_fd, SOL_SOCKET, SO_SNDTIMEO, (void*)&to, sizeof(to)) != 0)
        throw runtime_error("Failed to set send timeout.");

    to.tv_sec = rcv_timeout;
    to.tv_usec = 0;
    if (setsockopt(m_socket_fd, SOL_SOCKET, SO_RCVTIMEO, (void*)&to, sizeof(to)) != 0)
        throw runtime_error("Failed to set receive timeout.");
}

Socket::~Socket()
{
    close(m_socket_fd);
}


uint16_t checksum(uint16_t *buf, size_t len)
{
    int32_t sum = 0; // sum accumulator

    // add sequent 16-bit numbers to accumulator
    while (len > 1)
    {
        sum += *buf++;
        len -= 2;
    }

    // add last byte without a pair if present
    if (len > 0)
        sum += *(uint8_t*)buf;

    // add carry from hi 16-bits to low 16-bits
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    return ~sum; // return complement
}

up_addrinfo host_serv(const char *host, const char *serv, int family, int socktype)
{
    struct addrinfo hints = { 0 };
    struct addrinfo *ai = nullptr;

    hints.ai_flags    = AI_CANONNAME;   /* always return canonical name */
    hints.ai_family   = family;         /* AF_UNSPEC, AF_INET, AF_INET6, etc. */
    hints.ai_socktype = socktype;       /* 0, SOCK_STREAM, SOCK_DGRAM, etc. */

    getaddrinfo(host, serv, &hints, &ai);
    return up_addrinfo(ai); /* return pointer to first on linked list */
}

Pinger::Pinger(const std::string& address, unsigned snd_timeout, unsigned rcv_timeout) :
    m_sock(AF_INET, SOCK_RAW, IPPROTO_ICMP, snd_timeout, rcv_timeout),  // create raw socket
    m_address(address),
    m_transmitted(0),
    m_id(getpid())
{
}

Pinger::~Pinger()
{
}


void Pinger::ping()
{
    char buf[kBufSize] = {0};

    // get address info
    up_addrinfo ai = host_serv(m_address.c_str(), NULL, AF_INET, 0);
    if (nullptr == ai)
        throw runtime_error("Bad ping target");

    // fill icmp message
    struct icmp* icmp = (struct icmp *)buf;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_seq = 0;
    icmp->icmp_id = m_id;

    char payload[] = { '\xDE', '\xAD', '\xBE', '\xAF', '\xAA' };
    int data_len = sizeof(payload);
    int size_need_to_send = ICMP_MINLEN + data_len;
    int size_need_to_receive = sizeof(struct ip) + ICMP_MINLEN + data_len;

    for (size_t i = 0; i < data_len; i++)
        icmp->icmp_data[i] = payload[i];

    icmp->icmp_cksum = 0;
    icmp->icmp_cksum = checksum((uint16_t*)icmp, size_need_to_send);

    // send
    int send_size = sendto(m_sock.get(), buf, size_need_to_send, 0, ai->ai_addr, ai->ai_addrlen);
    if (send_size < size_need_to_send)
        throw runtime_error("Not enough data sent");

    // receive
    int recv_size = recvfrom(m_sock.get(), buf, size_need_to_receive, 0, NULL, NULL);
    if (recv_size < size_need_to_receive)
        throw runtime_error("Not enough data received");

    // check response
    struct ip *ip = (struct ip*) buf;
    int ip_header_len = ip->ip_hl << 2;
    icmp = (struct icmp*)(buf + ip_header_len);

    if (icmp->icmp_type != ICMP_ECHOREPLY)
        throw runtime_error("Response is not ECHO REPLY.");

    if (icmp->icmp_id != m_id)
        throw runtime_error("Wrong id in reply.");

    for (size_t i = 0; i < data_len; i++)
    {
        if ((char)icmp->icmp_data[i] != (char)payload[i])
            throw runtime_error("Data in reply is wrong.");
    }
}
