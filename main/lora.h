#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Parsed data from:
 * AT+SENSOR=UID,T_x100,RH_x100,Vcap_mV,CO2_ppm,TVOC_ppb,RSSI,SNR\r\n
 */
typedef struct {
    char     uid[9];        /* 8 hex chars + NUL */
    int32_t  t_x100;        /* temperature * 100, signed */
    uint32_t rh_x100;       /* humidity * 100 */
    uint32_t vcap_mV;       /* capacitor voltage in mV */
    uint32_t co2_ppm;       /* CO2 ppm */
    uint32_t tvoc_ppb;      /* TVOC ppb */
    int32_t  rssi;          /* dBm */
    int32_t  snr;           /* dB */
    float    temperature_c; /* derived: t_x100 / 100.0 */
    float    humidity_rh;   /* derived: rh_x100 / 100.0 */
    float    vcap_v;        /* derived: vcap_mV / 1000.0 */
    bool  valid;
} lora_sensor_data_t;

esp_err_t lora_start(void);

void lora_send_find_command(int addr, int duration);

bool lora_send_payload_reliable(const char *payload, size_t len, int max_retries, uint32_t ack_timeout_ms);

/* Get latest AT+SENSOR data for a given UID. Returns false if not found. */
bool lora_get_sensor_data(const char *uid, lora_sensor_data_t *out);

#ifdef __cplusplus
}
#endif
