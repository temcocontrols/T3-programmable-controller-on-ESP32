/*
 * Fucntions to handle Endianness.
 */

#ifndef _ENDIAN_H
#define _ENDIAN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif 

#define LITTLE 0
#define BIG 	 1
extern int endianness;

/* Checks Endianness of system, returns LITTLE(0) ot BIG(1). A little-endian
   system stores the least-significant byte at the smallest address. */
int endian();

/* Converts 16-bit number from host to network byte order. */
uint16_t h2ns(uint16_t i);

/* Converts 32-bit number from host to network byte order. */
uint32_t h2nl(uint32_t i);

/* Converts 16-bit integer to byte array in network byte order. */
void h2ns_byte(uint16_t i, unsigned char *str);

/* Converts 32-bit integer to byte array in network byte order. */
void h2nl_byte(uint32_t i, unsigned char *str);

/* Converts 64-bit integer to byte array in network byte order. */
void h2nll_byte(uint64_t i, unsigned char *str);

#ifdef __cplusplus
}
#endif

#endif
