#ifndef MBEDTLS_BASE64_H
#define MBEDTLS_BASE64_H

#include <stddef.h>

int mbedtls_base64_encode(unsigned char *dst, size_t dlen, size_t *olen,
                          const unsigned char *src, size_t slen);

#endif // MBEDTLS_BASE64_H
