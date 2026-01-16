/*
 * Implements core functions of an uSNMP manager to
 * 1. build a SNMPv1 command
 * 2. send, receive and parse SNMPv2 and SNMPv1 packet
 */

#ifndef SNMPMGR_H
#define SNMPMGR_H

#include "mibutil.h"

#ifdef __cplusplus
extern "C" {
#endif 

extern char hostIpAddr[], remoteIpAddr[], remoteCommunity[];
extern struct messageStruct request, response, vblist;
extern unsigned char requestBuffer[], responseBuffer[], vbBuffer[];
extern unsigned char errorStatus, errorIndex;
extern unsigned int reqId;
extern Boolean debug;

/* Initialise SNMP manager to listen at port. Returns socket fd or -1 if fail. */
int initSnmpMgr( int port );

void exitSnmpMgr( void );

/* Builds a request PDU and returns its constructed length. */
int reqBuild(struct messageStruct *req, unsigned char reqType, unsigned int reqId,
	struct messageStruct *vblist);

/* Sends a SNMP request and wait for a response. Returns Success(0) or Fail(-1). */
int reqSend(struct messageStruct *req, struct messageStruct *resp,
	char *dst, uint16_t port_no, char *comm_str, int time_out);

/* Parses a SNMP response. Returns Success(0) or Fail(-1). */
int parseResponse(struct messageStruct *resp, char *comm_str, unsigned int *reqId,
	unsigned char *errorStatus, unsigned char *errorIndex, struct messageStruct *vblist);

/* Parses a SNMP trap. Returns Success(0) or Fail(-1). */
int parseTrap(struct messageStruct *resp, char *comm_str, OID *entoid, char *agentaddr,
	unsigned int *gen, unsigned int *spec, unsigned int *timestamp, struct messageStruct *vblist);
	
#ifdef __cplusplus
}
#endif

#endif
