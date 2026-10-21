#ifndef SNMP_TRAPS_H
#define SNMP_TRAPS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void send_snmp_trap_cold_start(const char *l_ip, const char *d_ip, uint16_t d_port, const char *community);
void send_snmp_trap_link_up(const char *l_ip, const char *d_ip, uint16_t d_port, const char *community);
void send_snmp_trap_link_down(const char *l_ip, const char *d_ip, uint16_t d_port, const char *community);
void send_snmp_trap_warm_start(const char *l_ip, const char *d_ip, uint16_t d_port, const char *community);
void snmp_trap_auth_failure(const char *l_ip, const char *d_ip, uint16_t d_port, const char *community);
void snmp_trap_enterprise(const char *l_ip, const char *d_ip, uint16_t d_port, const char *community, const char *msg);

#ifdef __cplusplus
}
#endif

#endif /* SNMP_TRAPS_H */
