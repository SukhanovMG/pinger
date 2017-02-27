#include "ping.h"

#include <memory>
#include <netdb.h> // for getaddrinfo, etc..
#include <netinet/in.h> // IPPROTO_ICMP
#include <netinet/ip_icmp.h>
#include <unistd.h> // close

#include <stdexcept>

using namespace std;

Pinger::Pinger() :
	m_socketfd(-1),
	m_transmitted(0),
	m_id(getpid()),
	m_data_len(0)
{
	// create raw socket
	m_socketfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (m_socketfd < 0)
		throw runtime_error("Failed to create raw socket.");

	// drop root priveleges if it is possible
	setuid(getuid());

	// fill icmp message
	struct icmp* icmp = (struct icmp *)m_sendbuf;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_seq = m_transmitted++;
	icmp->icmp_id = m_id;

	icmp->icmp_cksum = 0;
	icmp->icmp_cksum = checksum((uint16_t*)icmp, 8 + m_data_len);

	unique_ptr<struct addrinfo, void(*)(struct addrinfo*)> ai(host_serv("8.8.8.8", NULL, AF_INET, 0), &freeaddrinfo);

	sendto(m_socketfd, m_sendbuf, 8 + m_data_len, 0, ai->ai_addr, ai->ai_addrlen);
}

Pinger::~Pinger()
{
	if (m_socketfd >= 0)
	{
		close(m_socketfd);
	}
}

uint16_t Pinger::checksum(uint16_t *buf, size_t len)
{
	uint16_t answer = 0;

	int32_t sum = 0;
	while (len > 1)
	{
		sum += *buf++;
		len -=2;
	}

	if (len > 0)
		sum += *(uint8_t*)buf;

	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	answer = ~sum;
	return answer;
}

struct addrinfo *Pinger::host_serv(const char *host, const char *serv, int family, int socktype)
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