#ifndef LWIP_INET_H
#define LWIP_INET_H

#include <stdint.h>
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

#ifdef __cplusplus
}
#endif

#endif // LWIP_INET_H
