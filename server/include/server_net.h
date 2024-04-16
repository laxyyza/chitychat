#ifndef _SERVER_NET_H_
#define _SERVER_NET_H_

#include "common.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <netdb.h>

enum ip_version
{
    IPv4,
    IPv6,
};

typedef struct 
{
    i32 sock;
    enum ip_version version;
    struct sockaddr* addr_ptr;
    union {
        struct sockaddr_in ipv4;
        struct sockaddr_in6 ipv6;
    };
    socklen_t len;
    char ip_str[INET6_ADDRSTRLEN];
    char host[NI_MAXHOST];
    char serv[NI_MAXSERV];
} net_addr_t;

#endif // _SERVER_NET_H_
