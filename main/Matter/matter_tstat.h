#pragma once
#include <stdint.h>
#include <esp_err.h>
#include "modbus.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MATTER_SYNC_INTERVAL_MS 500

#define TSTAT_FEATURE_HEATING        0x01
#define TSTAT_FEATURE_COOLING        0x02
#define TSTAT_FEATURE_OCCUPANCY      0x04
#define TSTAT_FEATURE_AUTO_MODE      0x08
#define TSTAT_FEATURE_LOCAL_TEMP_NOT_EXPOSED 0x10
#define TSTAT_FEATURE_SETBACK        0x20
#define TSTAT_FEATURE_SCHEDULE       0x40

/* System modes (matches Matter spec) */
typedef enum {
    TSTAT_MODE_OFF      = 0,
    TSTAT_MODE_AUTO     = 1,
    TSTAT_MODE_COOL     = 3,
    TSTAT_MODE_HEAT     = 4,
    TSTAT_MODE_FAN_ONLY = 7,
    TSTAT_MODE_DRY      = 8,
} tstat_mode_t;

/* Running state bits (what hardware is actually doing) */
typedef enum {
    TSTAT_RUNNING_IDLE    = 0x00,
    TSTAT_RUNNING_HEAT    = 0x01,
    TSTAT_RUNNING_COOL    = 0x02,
    TSTAT_RUNNING_FAN     = 0x04,
} tstat_running_state_t;

/* Full thermostat state snapshot */
typedef struct {
    int16_t  local_temperature;      /* 0.01 °C  — from your sensor      */
    int16_t  outdoor_temperature;    /* 0.01 °C  — optional              */
    int16_t  heat_setpoint;          /* 0.01 °C                          */
    int16_t  cool_setpoint;          /* 0.01 °C                          */
    uint16_t measured_humidity;      /* e.g. 5000 = 50.00 %              */
    uint16_t min_humidity;           /* e.g. 0                           */
    uint16_t max_humidity;           /* e.g. 10000 = 100.00 %            */
    uint8_t  system_mode;            /* tstat_mode_t                     */
    uint8_t  running_state;          /* tstat_running_state_t bitmask    */
    uint8_t  occupancy;              /* 1 = occupied, 0 = unoccupied     */
    uint8_t  setpoint_hold;          /* 0 = off, 1 = hold indefinitely   */
    int16_t  min_heat_setpoint;      /* 0.01 °C                          */
    int16_t  max_heat_setpoint;      /* 0.01 °C                          */
    int16_t  min_cool_setpoint;      /* 0.01 °C                          */
    int16_t  max_cool_setpoint;      /* 0.01 °C                          */
} tstat_data_t;

typedef enum {
    MATTER_TSTAT_MAP_LOCAL_TEMP = 0,
    MATTER_TSTAT_MAP_OUTDOOR_TEMP,
    MATTER_TSTAT_MAP_HUMIDITY,
    MATTER_TSTAT_MAP_HEAT_SETPOINT,
    MATTER_TSTAT_MAP_COOL_SETPOINT,
    MATTER_TSTAT_MAP_MODE,
    MATTER_TSTAT_MAP_RUNNING_STATE,
    MATTER_TSTAT_MAP_OCCUPANCY,
    MATTER_TSTAT_MAP_COUNT
} matter_tstat_map_id_t;

typedef struct {
    uint8_t point_type;
    uint8_t point_number;
} matter_tstat_map_t;

/* ------------------------------------------------------------------ */
/* Modbus details                                                     */
/* ------------------------------------------------------------------ */

#define MATTER_TSTAT_MB_REG_COUNT    (MATTER_TSTAT_MAP_COUNT * 2)  /* 16 registers */

#define	MODBUS_MATTER_MAP_REG_END    (MODBUS_MATTER_MAP_REG_BASE + MATTER_TSTAT_MB_REG_COUNT - 1)

/* Helper macros */
#define MB_REG_TYPE(idx)    (MODBUS_MATTER_MAP_REG_BASE + ((idx) * 2))
#define MB_REG_NUM(idx)     (MODBUS_MATTER_MAP_REG_BASE + ((idx) * 2) + 1)

/* Named register addresses for convenience */
#define MB_REG_LOCAL_TEMP_TYPE       MB_REG_TYPE(MATTER_TSTAT_MAP_LOCAL_TEMP)      /* 600 */
#define MB_REG_LOCAL_TEMP_NUM        MB_REG_NUM(MATTER_TSTAT_MAP_LOCAL_TEMP)       /* 601 */
#define MB_REG_OUTDOOR_TEMP_TYPE     MB_REG_TYPE(MATTER_TSTAT_MAP_OUTDOOR_TEMP)    /* 602 */
#define MB_REG_OUTDOOR_TEMP_NUM      MB_REG_NUM(MATTER_TSTAT_MAP_OUTDOOR_TEMP)     /* 603 */
#define MB_REG_HUMIDITY_TYPE         MB_REG_TYPE(MATTER_TSTAT_MAP_HUMIDITY)        /* 604 */
#define MB_REG_HUMIDITY_NUM          MB_REG_NUM(MATTER_TSTAT_MAP_HUMIDITY)         /* 605 */
#define MB_REG_HEAT_SETPOINT_TYPE    MB_REG_TYPE(MATTER_TSTAT_MAP_HEAT_SETPOINT)   /* 606 */
#define MB_REG_HEAT_SETPOINT_NUM     MB_REG_NUM(MATTER_TSTAT_MAP_HEAT_SETPOINT)    /* 607 */
#define MB_REG_COOL_SETPOINT_TYPE    MB_REG_TYPE(MATTER_TSTAT_MAP_COOL_SETPOINT)   /* 608 */
#define MB_REG_COOL_SETPOINT_NUM     MB_REG_NUM(MATTER_TSTAT_MAP_COOL_SETPOINT)    /* 609 */
#define MB_REG_MODE_TYPE             MB_REG_TYPE(MATTER_TSTAT_MAP_MODE)            /* 610 */
#define MB_REG_MODE_NUM              MB_REG_NUM(MATTER_TSTAT_MAP_MODE)             /* 611 */
#define MB_REG_RUNNING_STATE_TYPE    MB_REG_TYPE(MATTER_TSTAT_MAP_RUNNING_STATE)   /* 612 */
#define MB_REG_RUNNING_STATE_NUM     MB_REG_NUM(MATTER_TSTAT_MAP_RUNNING_STATE)    /* 613 */
#define MB_REG_OCCUPANCY_TYPE        MB_REG_TYPE(MATTER_TSTAT_MAP_OCCUPANCY)       /* 614 */
#define MB_REG_OCCUPANCY_NUM         MB_REG_NUM(MATTER_TSTAT_MAP_OCCUPANCY)        /* 615 */

/* ------------------------------------------------------------------ */
/* Public APIs                                                        */
/* ------------------------------------------------------------------ */
esp_err_t matter_tstat_map_load(void);
esp_err_t matter_tstat_map_save(void);
esp_err_t matter_tstat_map_reset_defaults(void);
void      matter_tstat_map_from_modbus(uint16_t reg, uint16_t value);
uint16_t  matter_tstat_map_to_modbus(uint16_t reg);

/* ------------------------------------------------------------------ */
/* Init                                                               */
/* ------------------------------------------------------------------ */

esp_err_t matter_tstat_init(void);

/* ------------------------------------------------------------------ */
/* Report FROM hardware → Matter (call these from your sensor/control */
/* tasks whenever a value changes locally)                            */
/* ------------------------------------------------------------------ */
esp_err_t matter_tstat_report_temperature(int16_t temp_001c);
esp_err_t matter_tstat_report_outdoor_temperature(int16_t temp_001c);
esp_err_t matter_tstat_report_running_state(tstat_running_state_t state);
esp_err_t matter_tstat_report_occupancy(uint8_t occupied);

/* Local setpoint change (e.g. physical buttons on device) */
esp_err_t matter_tstat_report_heat_setpoint(int16_t sp_001c);
esp_err_t matter_tstat_report_cool_setpoint(int16_t sp_001c);
esp_err_t matter_tstat_report_mode(tstat_mode_t mode);

/* ------------------------------------------------------------------ */
/* Commissioning                                                      */
/* ------------------------------------------------------------------ */
void matter_clear_commissioning(void);

/* ------------------------------------------------------------------  */
/* Callback — implement this in your app to react to controller writes */
/* ------------------------------------------------------------------  */
void app_tstat_on_mode_change(tstat_mode_t mode);
void app_tstat_on_heat_setpoint_change(int16_t sp_001c);
void app_tstat_on_cool_setpoint_change(int16_t sp_001c);
void app_tstat_on_setpoint_hold_change(uint8_t hold);

#ifdef __cplusplus
}
#endif
