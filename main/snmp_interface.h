#ifndef SNMP_APP_H
#define SNMP_APP_H

#include "snmp_traps.h"

// ---------------------------------------------------
// Public defines (Configs)
// B denotes Mgmt-Mib2           - 1.3.6.1.2.1
// E denotes Experimental        - 1.3.6.1.3
// P denotes Private-Enterprises - 1.3.6.1.4.1
// ---------------------------------------------------
#define OFFICIAL_IANA_PAN   "50523"                 // official IANA number
#define PRODUCT_OID         "P."OFFICIAL_IANA_PAN   // used in private MIB OIDs
#define ENTERPRISE_OID       PRODUCT_OID".1"        // used as sysObjectID and in trap
#define SYSTEM_OBJECT_ID     ENTERPRISE_OID".1"     // sysObjectID for this device

#define TRAP_DST_ADDR       "10.80.17.104"          // Destination address for SNMP traps

// ---------------------------------------------------
// Public functions
// ---------------------------------------------------

/*
 * Initialize the SNMP agent
 */
void snmp_app_init(void);

/*
 * Update SNMP system information (MIB values)
 */
void update_system_ip_address(void);

/*
 * Update SNMP system information (MIB values)
 */
void update_system_location(const char* location);

/*
 * Update SNMP system information (MIB values)
 */
void update_system_name(const char* name);

/*
 * Update SNMP system information (MIB values)
 */
void update_system_description(const char* description);

/*
 * Update SNMP system information (MIB values)
 */
void update_system_contact(const char* contact);

/*
 * Update SNMP trap destination address
 */
void update_system_version(const char* version);

/*
 * Update SNMP trap destination address
 */
void update_trap_destination(const char* dest_ip);

/**
 * Send Master Trap (Autonomous Notification) with a bundle of varbinds
 * Trap OID: 1.3.6.1.4.1.50523.1.0.1 (t3AlarmNotification)
 * 
 * @param local_ip: Source IP address
 * @param dst_ip: Destination SNMP manager IP
 * @param site_location: Physical location (e.g., "Building B, Floor 4")
 * @param event_id: Unique tracking ID (matches Set and Clear events)
 * @param event_set: 1 = Alarm Active, 0 = Alarm Cleared
 * @param alarm_type: Contextual string (e.g., "AI1 Temperature High")
 * @param alarm_value: The numeric value at time of trigger
 */
void send_master_trap_autonomous_notification(const char* local_ip, const char* dst_ip,
                                              const char* site_location,
                                              uint32_t event_id,
                                              uint8_t event_set,
                                              const char* alarm_type,
                                              int32_t alarm_value);

#endif // SNMP_APP_H