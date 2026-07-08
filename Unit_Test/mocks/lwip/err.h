#ifndef LWIP_ERR_H
#define LWIP_ERR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int err_t;
#define ERR_OK 0
#define ERR_MEM -1
#define ERR_TIMEOUT -2

// LwIP shorthand types
typedef uint8_t u8_t;
typedef uint16_t u16_t;
typedef uint32_t u32_t;

#ifdef __cplusplus
}
#endif

#endif // LWIP_ERR_H
