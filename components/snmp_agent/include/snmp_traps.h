#ifndef SNMP_TRAPS_H
#define SNMP_TRAPS_H

void send_snmp_trap_cold_start(const char* l_ip, const char* d_ip);
void send_snmp_trap_link_up(const char* l_ip, const char* d_ip);
void send_snmp_trap_link_down(const char* l_ip, const char* d_ip);
void send_snmp_trap_warm_start(const char* l_ip, const char* d_ip);
void snmp_trap_auth_failure(const char* l_ip, const char* d_ip);
void snmp_trap_enterprise(const char* l_ip, const char* d_ip, const char* msg);

#endif /* SNMP_TRAPS_H */