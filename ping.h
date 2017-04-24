#ifndef PINGER_H
#define PINGER_H

#include <cstdint> // fixed integers
#include <cstddef> // size_t
#include <string>

class Socket
{
public:
    Socket(int domain, int type, int protocol);
    ~Socket();
    int get() const { return m_socket_fd; }
private:
    int m_socket_fd;
};

class Pinger
{
public:
    enum defaults { kBufSize = 1500 };
    Pinger(const std::string& address);
    ~Pinger();

    void set_address(const std::string& address) { m_address = address; }

    void ping();
private:
    Socket m_sock;
    std::string m_address;

    int m_transmitted;
    int m_id;
};

#endif