#ifndef SNMP_TRAPS_H
#define SNMP_TRAPS_H

#include <stdint.h>

// Data types for flexible varbind values
#define VARBIND_TYPE_OCTET_STRING  0x04
#define VARBIND_TYPE_INTEGER       0x02
#define VARBIND_TYPE_OID           0x06

typedef struct
{
    uint32_t* oid;
    int oid_len;
    char* value;
} ExtraVarbind;

// Extended varbind structure for mixed data types
typedef struct
{
    uint32_t* oid;
    int oid_len;
    uint8_t data_type;        // VARBIND_TYPE_* constants
    void* value;              // Points to the value (string or integer)
    int value_len;            // Length for strings, unused for integers
} FlexibleVarbind;

void send_snmp_trap_cold_start(const char* l_ip, const char* d_ip);
void send_snmp_trap_link_up(const char* l_ip, const char* d_ip);
void send_snmp_trap_link_down(const char* l_ip, const char* d_ip);
void send_snmp_trap_warm_start(const char* l_ip, const char* d_ip);
void snmp_trap_auth_failure(const char* l_ip, const char* d_ip);
void snmp_trap_enterprise(const char* l_ip, const char* d_ip, const char* msg);

void snmp_send_v2_flexible(const char* local_ip, const char* dst_ip, uint32_t* trap_oid, int trap_oid_len, 
                          FlexibleVarbind* extras, int extra_count, int is_inform);
#endif /* SNMP_TRAPS_H */