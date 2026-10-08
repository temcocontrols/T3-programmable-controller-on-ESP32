# uSNMP agent sources for ESP-IDF.
# Exclude snmp_manager.c (duplicate globals with snmp_agent.c) and wingetopt.c (Windows-only).
set(SNMP_AGENT_SRCS
    "${CMAKE_CURRENT_LIST_DIR}/src/endian.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/keylist.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/list.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/mib.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/miblist.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/mibutil.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/misc.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/octet.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/oid.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/snmp_agent.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/snmp_traps.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/timer.c"
    "${CMAKE_CURRENT_LIST_DIR}/src/varbind.c"
)

idf_component_register(SRCS ${SNMP_AGENT_SRCS}
                    INCLUDE_DIRS "include"
                    REQUIRES mbedtls esp_netif lwip esp_timer freertos)
