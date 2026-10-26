/*
 * Defines space allocations, according to target processor.
 */

#ifndef _USNMP_H
#define _USNMP_H

#define COMM_STR_SIZE 16
/* Allocated size in each MIB leaf to hold an octet string or OID */
#if defined(__AVR_ATmega328P__)
#define MIB_DATA_SIZE 32
#else
#define MIB_DATA_SIZE 128
#endif

/* OID array size; not too big that its TLV is > 128 bytes long.
   array[0] is a character to denote OID prefixes
     B denotes Mgmt-Mib2 - 1.3.6.1.2.1
     E denotes Experimental - 1.3.6.1.3
     P denotes Private-Enterprises - 1.3.6.1.4.1
     U denotes unknown, and a ill-formed OID
   Each subsequent array element corresponds to a dot-separated
   number in the OID. In systems of 16-bit integer, this number
   should not exceed 65535.
*/
#if defined(__AVR_ATmega328P__)
#define OID_SIZE 8
#else
#define OID_SIZE 16
#endif

/* Buffers to hold request and response packets, and a varbind pair. */
#if defined(__AVR_ATmega328P__)
#define REQUEST_BUFFER_SIZE 	96
#define RESPONSE_BUFFER_SIZE	128
#define VB_BUFFER_SIZE 32
#else
#define REQUEST_BUFFER_SIZE 	960
#define RESPONSE_BUFFER_SIZE	1280
#define VB_BUFFER_SIZE 256
#endif

#endif
