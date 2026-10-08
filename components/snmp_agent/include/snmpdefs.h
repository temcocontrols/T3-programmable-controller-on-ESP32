/*
 * SNMP definitions
 */

#ifndef SNMPDEFS_H
#define SNMPDEFS_H

/* Local ports to listen on */
#define SNMP_PORT         161 		 
#define TRAP_DST_PORT     162
#define SNMP_V1           0
#define SNMP_V2C          1

/* SNMP PDU Types (Till v2c) */
#define GET_REQUEST       0xa0
#define GET_NEXT_REQUEST  0xa1
#define GET_RESPONSE      0xa2
#define SET_REQUEST       0xa3
#define TRAP_PACKET_V1    0xa4
#define GET_BULK_REQUEST  0xa5
#define INFORM_REQUEST    0xa6
#define TRAP_PACKET_V2C   0xa7

#define VALID_REQUEST(x)  ((x == GET_REQUEST) || \
                          (x == GET_NEXT_REQUEST) || \
                          (x == SET_REQUEST) || \
                          (x == TRAP_PACKET_V1) || \
                          (x == GET_BULK_REQUEST) || \
                          (x == INFORM_REQUEST) || \
                          (x == TRAP_PACKET_V2C))

#define INTEGER           0x02
#define OCTET_STRING      0x04
#define NULL_ITEM         0x05
#define OBJECT_IDENTIFIER 0x06
#define SEQUENCE          0x30
#define SEQUENCE_OF       SEQUENCE
#define IP_ADDRESS        0x40
#define COUNTER           0x41
#define GAUGE             0x42
#define TIMETICKS         0x43
#define OPAQUE_TYPE       0x44
#define COUNTER64         0x46
#define NO_SUCH_OBJECT    0x80
#define NO_SUCH_INSTANCE  0x81
#define END_OF_MIB_VIEW   0x82

#define RD_ONLY           'R'
#define RD_WR             'W'

#define INT_SIZE        4           /* size of Integer, Gauge and Counter type */
#define LONG_SIZE       8           /* size of Counter64 type */
#define MAX_INTEGER     2147483647  /* 2^31-1 */
#define MIN_INTEGER    -2147483648
#define MAX_COUNTER     4294967295  /* 2^32-1 */
#define MAX_GAUGE       4294967295
#define MAX_TIMETICKS   4294967295

/* SNMP Message Error Codes */
#define NO_ERR               0
#define TOO_BIG              1
#define NO_SUCH_NAME         2
#define BAD_VALUE            3
#define READ_ONLY            4
#define GEN_ERROR            5

/* SNMP Trap Generic Codes */
#define COLD_START           0
#define WARM_START           1
#define LINK_DOWN            2
#define LINK_UP              3
#define AUTHENTICATE_FAIL    4
#define EGPNEIGHBOR_LOSS     5
#define ENTERPRISE_SPECIFIC  6

/* SNMP Operations function return codes */
#define SUCCESS              0
#define FAIL                -1
#define OID_NOT_FOUND       -2
#define ILLEGAL_DATA        -3
#define ILLEGAL_LENGTH      -4
#define BUFFER_FULL         -5
#define INVALID_DATA_TYPE   -6
#define INVALID_PDU_TYPE    -7
#define REQ_ID_ERR          -8
#define ILLEGAL_ERR_STATUS  -9
#define ILLEGAL_ERR_INDEX   -10
#define RD_ONLY_ACCESS      -11
#define COMM_STR_MISMATCH   -12
#define COMM_STR_ERR        -13

#endif
