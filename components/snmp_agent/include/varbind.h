/*
 * Data structures and functions to construct and traverse a sequence of
 * varaible bindings.
 */

#ifndef _VARBIND_H
#define _VARBIND_H

#include "endian.h"
#include "mib.h"

#ifdef __cplusplus
extern "C" {
#endif 

struct messageStruct {
	unsigned char *buffer;
	int size;
	int len;
	int index;
};

typedef struct {
	int start;		/* Absolute Index of the TLV */
	int len;			/* The L value of the TLV */
	int vstart; 	/* Absolute Index of this TLV's Value */
	int nstart; 	/* Absolute Index of the next TLV */
} tlvStructType;

/* Computes the length field of a TLV and returns the size of this Length field. */
int parseLength(unsigned char *msg, int *len);

/* Given a length, builds the length field of a TLV and returns the size of this field. */
int buildLength(unsigned char *buffer, int len);

/* Given a request TLV and a new size, this inserts the size argument into the
   L element of the response TLV and returns the size of this length field. */
int insertRespLen(struct messageStruct *request, int reqStart, struct messageStruct *response, int respStart, int size);

/* Compacts BER-encoded integer and returns the compacted size. */ 
int compactInt(unsigned char *tlv);

/* Extracts integer (signed), counter, gauge or timetick value. */ 
uint32_t getValue(unsigned char *vptr, int vlen, unsigned char datatype);

/* Extracts a TLV from msg starting at index. Return Success(0) or error code (<0). */ 
int parseTLV(unsigned char *msg, int index, tlvStructType *tlv);

/* Resets a varbind list to empty. */
void vblistReset(struct messageStruct *vblist);

/* Adds a varbind into a varbind list. Returns the total length of the resultant list. */
int vblistAdd(struct messageStruct *vblist, char *oidstr, unsigned char dataType, void *val, int vlen );

/* Traverses a varbind list where opt=0 for first varbind, non-zero for next.
   Returns the nth order of the extracted varbind. */
int vblistGet(struct messageStruct *vblist, MIB *vb, unsigned char opt);

#ifdef __cplusplus
}
#endif

#endif
