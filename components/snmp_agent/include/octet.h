/*
 * Functions to translate octet string and display string.
 *
 */

#ifndef _OCTET_H
#define _OCTET_H

#ifdef __cplusplus
extern "C" {
#endif 

/* Convert a dash-delimited hexadecimal display string to octet string,
   returns length of the octet string. */
int str2oct(char *str, unsigned char *oct);

/* Convert octet string to a dash-delimited hexadecimal display string,
   returns length of the hexa string. */
int oct2str(unsigned char *oct, int len, char *str);

/* Test if the octet string is printable; return 1 if yes, 0 if not. */
int octisprint(unsigned char *oct, int len);

#ifdef __cplusplus
}
#endif

#endif
