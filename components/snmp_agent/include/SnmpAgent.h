/*
 * Implements core functions of uSnmpAgent to
 * 1. initialise, traverse and access a MIB tree
 * 2. receive, parse and transmit SNMPv1 packet
 * 3. Parse a varbind list
 * 4. send a SNMPv1 trap
 * 5. send SNMPv2c trap and inform
 */

#ifndef SNMPAGENT_H
#define SNMPAGENT_H

#include <time.h>
#include "mibutil.h"
#include "miblist.h"
#include "varbind.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Global variables */
extern char hostIpAddr[], remoteIpAddr[];
extern uint16_t remotePort;
extern char *enterpriseOID, *roCommunity, *rwCommunity, remoteCommunity[];
extern Boolean (*checkCommnuity)(char *commstr, int reqType);

extern LIST *mibTree; 		// Holds the MIB tree for this agent
extern struct messageStruct request, response;
extern unsigned char requestBuffer[], responseBuffer[];
extern unsigned char errorStatus, errorIndex;
extern Boolean debug;

/* Prototypes */

/* Initialise SNMP agent to listen at port. Returns Success(0) or Fail(-1). The
   pointers to entoid and the community strings are copied to the global
   variables enterpriseOID, roCommunity, rwCommunity and trapCommunity
   respectively (not the contents they point to). */
int initSnmpAgent( int port, char *entoid, char *rocommstr, char *rwcommstr );

/* Sets the function used to validate the requester's community string. The
   agent validates it against rwCommunity by default.
   remoteCommunity holds the requester's community string, and is useful for
   implementing a multiplex agent. */
void setCheckCommunity ( Boolean (*func)(char *commstr, int reqtype) );

/* Process request and construct the response. Returns response length or Fail(-1). */
int processSNMP( void );

void exitSnmpAgent( void );

uint32_t sysUpTime( void );

/* Parses a varbind string into the global response buffer and returns its length. */
int vblistParse(int reqType, struct messageStruct *vblist);

/* Builds a trap and returns its length. */
int trapBuild(struct messageStruct *trap, char *entoid, char *agentaddr, int gen, int spec, struct messageStruct *vblist, int version);

/* Sends a trap */
void trapSend(struct messageStruct *trap, char *dst, uint16_t port_no, char *commstr, int version);

/* Sends a trap v2c */
void snmp_send_v2c_trap(const char *dest_ip,
                        const char *community,
                        const uint32_t *trap_oid,
                        uint8_t trap_oid_len,
                        uint32_t sysuptime_ticks);
/* Sends a inform v2c */
int snmp_send_v2c_inform(const char *dest_ip,
                         const char *community,
                         const uint32_t *trap_oid,
                         uint8_t trap_oid_len,
                         uint32_t sysuptime_ticks);


#ifdef __cplusplus
}
#endif

#endif
