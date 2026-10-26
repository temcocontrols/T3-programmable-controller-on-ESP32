/**
 * @file hub_snmp.h
 * @brief Hub wrapper around the uSNMP agent component.
 *
 * Configuration lives in global snmp_point (Str_Snmp_point in user_data.h).
 */

#ifndef HUB_SNMP_H
#define HUB_SNMP_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t hub_snmp_init(void);
esp_err_t hub_snmp_apply_config(void);
esp_err_t hub_snmp_deinit(void);
bool hub_snmp_is_running(void);

uint16_t snmp_config_read_by_block(uint16_t addr);
void snmp_config_write_by_block(uint16_t addr, uint8_t HeadLen, uint8_t *pData);

#ifdef __cplusplus
}
#endif

#endif /* HUB_SNMP_H */
