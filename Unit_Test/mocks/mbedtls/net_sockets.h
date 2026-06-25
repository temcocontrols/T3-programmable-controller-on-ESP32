#ifndef MBEDTLS_NET_SOCKETS_H
#define MBEDTLS_NET_SOCKETS_H

#include <stddef.h>

#define MBEDTLS_NET_PROTO_TCP 0

typedef struct {
    int fd;
} mbedtls_net_context;

void mbedtls_net_init(mbedtls_net_context *ctx);
int mbedtls_net_connect(mbedtls_net_context *ctx, const char *host, const char *port, int proto);
int mbedtls_net_send(void *ctx, const unsigned char *buf, size_t len);
int mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len);
void mbedtls_net_free(mbedtls_net_context *ctx);

#endif // MBEDTLS_NET_SOCKETS_H
