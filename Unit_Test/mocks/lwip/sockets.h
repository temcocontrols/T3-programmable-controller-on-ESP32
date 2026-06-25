#ifndef LWIP_SOCKETS_H
#define LWIP_SOCKETS_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int socklen_t;
typedef uint8_t sa_family_t;

struct in_addr {
    uint32_t s_addr;
};

struct sockaddr_in {
    uint8_t sin_len;
    uint8_t sin_family;
    uint16_t sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};

struct in6_addr {
    union {
        uint8_t u8_addr[16];
        uint16_t u16_addr[8];
        uint32_t u32_addr[4];
    } un;
};

struct sockaddr_in6 {
    uint8_t sin6_len;
    uint8_t sin6_family;
    uint16_t sin6_port;
    uint32_t sin6_flowinfo;
    struct in6_addr sin6_addr;
    uint32_t sin6_scope_id;
};

struct sockaddr {
    uint8_t sa_len;
    uint8_t sa_family;
    char sa_data[14];
};

struct sockaddr_storage {
    uint8_t ssa_len;
    uint8_t ssa_family;
    char ssa_data[26];
};

#include <sys/time.h>

#ifdef _WIN32

typedef struct {
    uint32_t fds_bits[32];
} fd_set;

#endif

#define FD_ZERO(s) memset(s, 0, sizeof(*s))
#define FD_SET(d, s) ((s)->fds_bits[(d)/32] |= (1 << ((d)%32)))

#define AF_INET 2
#define AF_INET6 10
#define PF_INET AF_INET
#define PF_INET6 AF_INET6

#define IPPROTO_IP 0
#define IPPROTO_TCP 6
#define IPPROTO_IPV6 41

#define SOCK_DGRAM 2
#define SOCK_STREAM 1

#define INADDR_ANY 0x00000000
#define INADDR_BROADCAST 0xffffffffUL

#define SOL_SOCKET 0xfff
#define SO_KEEPALIVE 0x0008
#define TCP_KEEPIDLE 3
#define TCP_KEEPINTVL 4
#define TCP_KEEPCNT 5

#define bzero(s, n) memset(s, 0, n)
#define CONFIG_EXAMPLE_PORT 47808

int socket(int domain, int type, int protocol);
int bind(int s, const struct sockaddr *name, socklen_t namelen);
int connect(int s, const struct sockaddr *name, socklen_t namelen);
int send(int s, const void *dataptr, size_t size, int flags);
int sendto(int s, const void *dataptr, size_t size, int flags, const struct sockaddr *to, socklen_t tolen);
int recv(int s, void *mem, size_t len, int flags);
int recvfrom(int s, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
int close(int s);
int shutdown(int s, int how);
int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
int listen(int s, int backlog);
int accept(int s, struct sockaddr *addr, socklen_t *addrlen);

#ifdef _WIN32
int select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout);
#endif

static inline uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0xff000000) >> 24) |
           ((hostlong & 0x00ff0000) >> 8)  |
           ((hostlong & 0x0000ff00) << 8)  |
           ((hostlong & 0x000000ff) << 24);
}

static inline uint16_t htons(uint16_t hostshort) {
    return ((hostshort & 0xff00) >> 8) |
           ((hostshort & 0x00ff) << 8);
}

static inline uint16_t ntohs(uint16_t netshort) {
    return htons(netshort);
}

static inline uint32_t ntohl(uint32_t netlong) {
    return htonl(netlong);
}

const char *inet_ntoa_r_in(struct in_addr addr, char *buf, int buflen);
const char *inet_ntoa_r_u32(uint32_t addr, char *buf, int buflen);

#define inet_ntoa_r(addr, buf, buflen) _Generic((addr), \
    struct in_addr: inet_ntoa_r_in, \
    default: inet_ntoa_r_u32 \
)(addr, buf, buflen)

const char *inet6_ntoa_r(struct in6_addr addr, char *buf, int buflen);

#ifdef __cplusplus
}
#endif

#endif // LWIP_SOCKETS_H
