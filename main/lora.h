#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LORA_MAX_SENSORS 16
/* Custom node reports every 300000 ms; allow three reporting intervals. */
#ifndef LORA_SENSOR_TIMEOUT_MS
#define LORA_SENSOR_TIMEOUT_MS 900000U
#endif

typedef enum {
    LORA_STOPPED,
    LORA_STARTING,
    LORA_READY, /* Host UART/task ready; sensor_frames confirms received reports. */
    LORA_ERROR
} lora_state_t;

typedef struct {
    lora_state_t state;
    esp_err_t last_error;
    uint32_t sensor_frames;
    uint32_t invalid_frames;
    uint32_t dropped_lines;
    uint32_t cache_full;
    uint32_t tx_failures;
    uint32_t module_drop_frames; /* AT+DROP lines reported by the RA08 gateway. */
    uint64_t last_sensor_ms;
    bool node_firmware_detected; /* Saw "Node init OK" on the gateway UART. */
} lora_status_t;

/* Parsed data from:
 * AT+SENSOR=UID,T_x100,RH_x100,Vcap_mV,CO2_ppm,TVOC_ppb,LUX,RSSI,SNR\r\n
 */
typedef struct {
    char     uid[9];        /* 8 hex chars + NUL */
    int32_t  t_x100;        /* temperature * 100, signed */
    uint32_t rh_x100;       /* humidity * 100 */
    uint32_t vcap_mV;       /* capacitor voltage in mV */
    uint32_t co2_ppm;       /* CO2 ppm */
    uint32_t tvoc_ppb;      /* TVOC ppb */
    uint32_t lux;           /* illuminance lux */
    int32_t  rssi;          /* dBm */
    int32_t  snr;           /* dB */
    float    temperature_c; /* derived: t_x100 / 100.0 */
    float    humidity_rh;   /* derived: rh_x100 / 100.0 */
    float    vcap_v;        /* derived: vcap_mV / 1000.0 */
    bool  valid;
    uint64_t last_seen_ms;  /* Monotonic time since boot, not wall-clock time. */
    uint32_t age_ms;        /* Computed when read; saturates at UINT32_MAX. */
    bool     online;        /* age_ms < LORA_SENSOR_TIMEOUT_MS; valid retains history. */
} lora_sensor_data_t;

/* Task context only. Custom RA08 firmware configures RF/continuous RX itself.
 * ESP_OK means the host UART/task is ready, not that radio reception is proven.
 * Repeated calls preserve the cache; initialization failures can be retried. */
esp_err_t lora_start(void);

/* True means the full command was written to UART. The custom firmware sends
 * no OK and does not forward FIND acknowledgements to the controller. */
bool lora_send_find_command(int addr, int duration);

/* Unsupported by ra08-solar-polling: returns false without writing to UART,
 * and records ESP_ERR_NOT_SUPPORTED in status.last_error. */
bool lora_send_payload_reliable(const char *payload, size_t len, int max_retries, uint32_t ack_timeout_ms);

/* Get latest AT+SENSOR data for a given UID. Returns false if not found. */
bool lora_get_sensor_data(const char *uid, lora_sensor_data_t *out);
/* Stable slot enumeration; false for an unused/out-of-range slot. */
bool lora_get_sensor_by_index(size_t index, lora_sensor_data_t *out);
void lora_get_status(lora_status_t *out);

#ifdef __cplusplus
}
#endif
