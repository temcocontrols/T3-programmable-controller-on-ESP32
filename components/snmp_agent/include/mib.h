/*
 * Data strructure and functions to manage a MIB leaf.
 */

#ifndef _MIB_H
#define _MIB_H

#include <stdint.h>
#include "snmpdefs.h"
#include "oid.h"

#ifdef __cplusplus
extern "C" {
#endif 

typedef struct mib {
	OID oid;
	unsigned char dataType;
	int dataLen;
	union {
		unsigned char *octetstring;
		uint32_t intval;
		uint64_t intval64;
	} u;
	char access;
	int (*get)(struct mib *);
	int (*set)(struct mib *, void *, int);
} MIB;

/* Octet String and OID are copied as-is, assumed as octet and BER-encoded
   respectively. Set size=0 for numeric values. */
void mibsetvalue(MIB *thismib, void *u, int size);

/* (*get)() is expected to compute or fetch, then fill in the new data in *mib.
   (*set)() should actuate *data, then change the data in *mib.
   Both return a SNMP Operations function return codes defined in snmpdefs.h
*/
void mibsetcallback(MIB *thismib, int (*get)(MIB *mib), int (*set)(MIB *mib, void *data, int len));

#ifdef __cplusplus
}
#endif

#endif
