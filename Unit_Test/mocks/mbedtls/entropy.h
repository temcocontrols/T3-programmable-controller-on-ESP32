#ifndef MBEDTLS_ENTROPY_H
#define MBEDTLS_ENTROPY_H

#include <stddef.h>

typedef struct {
    int dummy;
} mbedtls_entropy_context;

void mbedtls_entropy_init(mbedtls_entropy_context *entropy);
int mbedtls_entropy_func(void *data, unsigned char *output, size_t len);
void mbedtls_entropy_free(mbedtls_entropy_context *entropy);

#endif // MBEDTLS_ENTROPY_H
