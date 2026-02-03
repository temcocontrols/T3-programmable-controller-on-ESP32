#ifndef SNMP_APP_H
#define SNMP_APP_H

#include "snmp_traps.h"

// ---------------------------------------------------
// Public defines (Configs)
// ---------------------------------------------------
#define ENTERPRISE_OID  "P.64991.30"    // used as sysObjectID and in trap
#define TRAP_DST_ADDR   "192.168.1.14"  // Destination address for SNMP traps

// ---------------------------------------------------
// Public functions
// ---------------------------------------------------
void snmp_app_init(void);

#endif // SNMP_APP_H