#ifndef LWIP_INET_H
#define LWIP_INET_H

#include <stdint.h>
#include <stdio.h>
#include "lwip/sockets.h" // for struct in_addr

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t addr;
} ip4_addr_t;

typedef struct {
    union {
        ip4_addr_t ip4;
    } u_addr;
    uint8_t type;
} ip_addr_t;

#define ip_2_ip4(ipaddr) (&((ipaddr)->u_addr.ip4))
#define inet_addr_to_ip4addr(target_ip4addr, source_in_addr) ((target_ip4addr)->addr = (source_in_addr)->s_addr)

const char *ipaddr_ntoa(const ip_addr_t *ipaddr);

static inline int inet_aton(const char *cp, struct in_addr *addr)
{
    unsigned a = 0, b = 0, c = 0, d = 0;
    if (cp == NULL || addr == NULL) {
        return 0;
    }
    if (sscanf(cp, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) {
        return 0;
    }
    if (a > 255 || b > 255 || c > 255 || d > 255) {
        return 0;
    }
    addr->s_addr = a | (b << 8) | (c << 16) | (d << 24);
    return 1;
}

static inline char *inet_ntoa(struct in_addr addr)
{
    static char buf[16];
    unsigned v = addr.s_addr;
    snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
             v & 255u, (v >> 8) & 255u, (v >> 16) & 255u, (v >> 24) & 255u);
    return buf;
}

#ifdef __cplusplus
}
#endif

#endif // LWIP_INET_H
