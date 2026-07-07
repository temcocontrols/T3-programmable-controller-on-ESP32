#ifndef LWIP_NETDB_H
#define LWIP_NETDB_H

#include "lwip/sockets.h"

#ifdef __cplusplus
extern "C" {
#endif

// Dummy netdb types
struct hostent {
    char *h_name;
    char **h_aliases;
    int h_addrtype;
    int h_length;
    char **h_addr_list;
};

struct addrinfo {
    int              ai_flags;
    int              ai_family;
    int              ai_socktype;
    int              ai_protocol;
    socklen_t        ai_addrlen;
    struct sockaddr *ai_addr;
    char            *ai_canonname;
    struct addrinfo *ai_next;
};

int lwip_getaddrinfo(const char *nodename,
       const char *servname,
       const struct addrinfo *hints,
       struct addrinfo **res);

void lwip_freeaddrinfo(struct addrinfo *ai);

#ifdef __cplusplus
}
#endif

#endif // LWIP_NETDB_H
