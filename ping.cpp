#include "ping.h"

#include <memory>
#include <netdb.h> // for getaddrinfo, etc..
#include <netinet/in.h> // IPPROTO_ICMP
#include <netinet/ip_icmp.h>
#include <sys/socket.h> // socket
#include <unistd.h> // close

#include <stdexcept>

using namespace std;

Socket::Socket(int domain, int type, int protocol) :
	m_socket_fd(socket(domain, type, protocol))
{
	if (m_socket_fd < 0)
		throw runtime_error("Failed to create socket.");
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

struct addrinfo *host_serv(const char *host, const char *serv, int family, int socktype)
{
	int n;
	struct addrinfo hints = { 0 };
	struct addrinfo *res = NULL;

	hints.ai_flags    = AI_CANONNAME;	/* always return canonical name */
	hints.ai_family   = family;			/* AF_UNSPEC, AF_INET, AF_INET6, etc. */
	hints.ai_socktype = socktype;		/* 0, SOCK_STREAM, SOCK_DGRAM, etc. */

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
		return NULL ;

	return res ;	/* return pointer to first on linked list */
}

Pinger::Pinger() :
	m_sock(AF_INET, SOCK_RAW, IPPROTO_ICMP),  // create raw socket
	m_transmitted(0),
	m_id(getpid())
{
	// drop root priveleges if it is possible
	setuid(getuid());

	char sendbuf[kBufSize] = {0};

	// fill icmp message
	struct icmp* icmp = (struct icmp *)sendbuf;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_seq = m_transmitted++;
	icmp->icmp_id = m_id;

	int data_len = 5;
	icmp->icmp_data[0] = 0xDE;
	icmp->icmp_data[1] = 0xAD;
	icmp->icmp_data[2] = 0xBE;
	icmp->icmp_data[3] = 0xAF;
	icmp->icmp_data[4] = 0xAA;

	icmp->icmp_cksum = 0;
	icmp->icmp_cksum = checksum((uint16_t*)icmp, 8 + data_len);

	unique_ptr<struct addrinfo, void(*)(struct addrinfo*)> ai(host_serv("8.8.8.8", NULL, AF_INET, 0), &freeaddrinfo);

	sendto(m_sock.get(), sendbuf, 8 + data_len, 0, ai->ai_addr, ai->ai_addrlen);
}

Pinger::~Pinger()
{
}