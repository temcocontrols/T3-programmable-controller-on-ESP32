/*
 * Defines the OID data structure and functions to converts OID notations.
 */

#ifndef _OID_H
#define _OID_H

#include "usnmp.h"
#include "misc.h"

#ifdef __cplusplus
extern "C" {
#endif 

/* OID array size; not too big that its TLV is > 128 bytes long.
   array[0] is a character to denote OID prefixes
     B denotes Mgmt-Mib2 - 1.3.6.1.2.1
     E denotes Experimental - 1.3.6.1.3
     P denotes Private-Enterprises - 1.3.6.1.4.1
     U denotes unknown, and a ill-formed OID
   Each subsequent array element corresponds to a dot-separated
   number in the OID. In systems of 16-bit integer, this number
   should not exceed 65535. This should not usually be a concern
   until the Private Enterprise Number exceeds that.
*/
typedef struct {
	unsigned char len;
	unsigned int array[OID_SIZE];
} OID;

/* Converts string to OID arrary, returns length of array. */
int str2oid(char *str, OID *oid);

/* Converts OID arrary to BER, returns length of encoded BER string. */
int oid2ber(OID *oid, unsigned char *str);

/* Converts BER to OID arrary, returns length of array. */
int ber2oid(unsigned char *str, int len, OID *oid);

/* Converts string to BER, returns length of BER-encoded string. */
int str2ber(char *str, unsigned char *ber);

/* Compares two OID arrays, return 0 if equal, >0 if oid1>oid2, <0 if oid1<oid2 */
int oidcmp(OID *oid1, OID *oid2);

#ifndef ARDUINO
/* Converts OID arrary to string, returns length of string. */
int oid2str(OID *oid, char *str);

/* Converts BER to string, returns length of string. */
int ber2str(unsigned char *ber, int len, char *str);
#endif

#ifdef __cplusplus
}
#endif

#endif
