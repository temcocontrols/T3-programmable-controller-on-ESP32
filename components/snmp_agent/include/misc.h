/*
 * A set of miscellaneous functions for the uSNMP library.
 */

#ifndef _MISC_H
#define _MISC_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Overlap-safe byte copy for uSNMP.
 * Named usnmp_memcopy to avoid clashing with BACnet's memcopy()
 * (different signature: dest, src, offset, len, max).
 */
void usnmp_memcopy(unsigned char *dst, unsigned char *src, int size);

#ifdef __cplusplus
}
#endif

#endif
