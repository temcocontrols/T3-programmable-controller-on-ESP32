#ifndef MBEDTLS_ERROR_H
#define MBEDTLS_ERROR_H

#include <stddef.h>

void mbedtls_strerror(int errnum, char *buffer, size_t buflen);

#endif // MBEDTLS_ERROR_H
