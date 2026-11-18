#ifndef HUB_GSM_STATUS_H
#define HUB_GSM_STATUS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Flag bits in gsm_status_point.reg.flags (Modbus 2253) */
#define GSM_FLAG_SIM_READY      (1u << 0)
#define GSM_FLAG_REGISTERED     (1u << 1)
#define GSM_FLAG_ATTACHED       (1u << 2)
#define GSM_FLAG_CONNECTED      (1u << 3)
#define GSM_FLAG_AT_READY       (1u << 4)
#define GSM_FLAG_SIM1_PRESENT   (1u << 5)
#define GSM_FLAG_SIM2_PRESENT   (1u << 6)
#define GSM_FLAG_STATUS_VALID   (1u << 7)

/**
 * Refresh gsm_status_point from A7608 + LTE PPPOS live status.
 * Safe to call from Modbus and BACnet private-transfer paths.
 */
void hub_gsm_status_refresh(void);

/**
 * Read one Modbus holding register in the GSM block (2251-2300).
 * Refreshes status before returning.
 */
uint16_t gsm_status_read_by_block(uint16_t addr);

#ifdef __cplusplus
}
#endif

#endif /* HUB_GSM_STATUS_H */
