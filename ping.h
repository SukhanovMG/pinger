#ifndef PINGER_H
#define PINGER_H

#include <cstdint> // fixed integers
#include <cstddef> // size_t
#include <sys/socket.h> // socket

class Pinger
{
public:
	enum defaults { kBufSize = 1500 };
	Pinger();
	~Pinger();
private:
	uint16_t checksum(uint16_t *buf, size_t len);
	struct addrinfo * host_serv(const char *host, const char *serv, int family, int socktype);

	int m_socketfd;
	struct sock_addr *dest_addr;
	socklen_t dest_addr_len;

	char m_sendbuf[kBufSize];
	int m_transmitted;
	int m_id;
	int m_data_len;
};

#endif