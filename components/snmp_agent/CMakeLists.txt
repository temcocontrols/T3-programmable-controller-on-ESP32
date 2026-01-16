file(GLOB SNMP_AGENT_SRCS "${CMAKE_CURRENT_LIST_DIR}/src/*.c")

idf_component_register(SRCS ${SNMP_AGENT_SRCS}
                    INCLUDE_DIRS "include"
                    REQUIRES mbedtls esp_netif lwip esp_timer)
