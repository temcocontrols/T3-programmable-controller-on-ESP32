/*
 * A set of miscellaneous functions for the uSNMP library.
 */

#ifndef _MISC_H
#define _MISC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Memory copy, byte by byte, that can handle overlapping regions. */
void memcopy( unsigned char *dst, unsigned char *src, int size);

#ifdef __cplusplus
}
#endif

#endif
