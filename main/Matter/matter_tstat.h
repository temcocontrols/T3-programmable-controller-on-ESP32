#pragma once
#include <stdint.h>
#include <esp_err.h>
#include "modbus.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MATTER_SYNC_INTERVAL_MS      1000

#define TSTAT_FEATURE_HEATING        0x01
#define TSTAT_FEATURE_COOLING        0x02
#define TSTAT_FEATURE_OCCUPANCY      0x04
#define TSTAT_FEATURE_AUTO_MODE      0x08
#define TSTAT_FEATURE_LOCAL_TEMP_NOT_EXPOSED 0x10
#define TSTAT_FEATURE_SETBACK        0x20
#define TSTAT_FEATURE_SCHEDULE       0x40

/* Temperature values are in 0.01°C (centi-degrees) */

/* Local Temperature */
#define TSTAT_MIN_LOCAL_TEMP            (-4000)   // -40.00°C
#define TSTAT_MAX_LOCAL_TEMP            (12500)   // 125.00°C

/* Occupied Heating Setpoint */
#define TSTAT_MIN_HEAT_SETPOINT         (700)     // 7.00°C
#define TSTAT_MAX_HEAT_SETPOINT         (3000)    // 30.00°C

/* Occupied Cooling Setpoint */
#define TSTAT_MIN_COOL_SETPOINT         (1600)    // 16.00°C
#define TSTAT_MAX_COOL_SETPOINT         (3200)    // 32.00°C

/* Absolute limits (hard safety range) */
#define TSTAT_ABS_MIN_HEAT_SETPOINT     (700)
#define TSTAT_ABS_MAX_HEAT_SETPOINT     (3000)

#define TSTAT_ABS_MIN_COOL_SETPOINT     (1600)
#define TSTAT_ABS_MAX_COOL_SETPOINT     (3200)

/* Deadband (difference between heat & cool) */
#define TSTAT_MIN_SETPOINT_DEADBAND     (25)      // 0.25°C
#define TSTAT_MAX_SETPOINT_DEADBAND     (1000)    // 10.00°C

/* System Mode */
#define TSTAT_MIN_SYSTEM_MODE           (0)       // OFF
#define TSTAT_MAX_SYSTEM_MODE           (4)       // AUTO

/* Control Sequence */
#define TSTAT_MIN_CONTROL_SEQUENCE      (0)
#define TSTAT_MAX_CONTROL_SEQUENCE      (5)

/* Humidity is in 0.01% (centi-percent) */

#define HUMIDITY_MIN_MEASURED_VALUE     (0)       // 0%
#define HUMIDITY_MAX_MEASURED_VALUE     (10000)   // 100%

#define HUMIDITY_MIN_LIMIT              (0)
#define HUMIDITY_MAX_LIMIT              (10000)

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
    int16_t  heat_setpoint;          /* 0.01 °C                          */
    int16_t  cool_setpoint;          /* 0.01 °C                          */
    uint16_t measured_humidity;      /* e.g. 5000 = 50.00 %              */
    uint16_t min_humidity;           /* e.g. 0                           */
    uint16_t max_humidity;           /* e.g. 10000 = 100.00 %            */
    uint8_t  system_mode;            /* tstat_mode_t                     */
    int16_t  min_heat_setpoint;      /* 0.01 °C                          */
    int16_t  max_heat_setpoint;      /* 0.01 °C                          */
    int16_t  min_cool_setpoint;      /* 0.01 °C                          */
    int16_t  max_cool_setpoint;      /* 0.01 °C                          */
} tstat_data_t;

typedef enum {
    MATTER_TSTAT_MAP_LOCAL_TEMP = 0,
    MATTER_TSTAT_MAP_HUMIDITY,
    MATTER_TSTAT_MAP_HEAT_SETPOINT,
    MATTER_TSTAT_MAP_COOL_SETPOINT,
    MATTER_TSTAT_MAP_MODE,
    MATTER_TSTAT_MAP_COUNT
} matter_tstat_map_id_t;

typedef struct {
    uint8_t point_type;
    uint8_t point_number;
} matter_tstat_map_t;

/* ------------------------------------------------------------------ */
/* Modbus details                                                     */
/* ------------------------------------------------------------------ */

#define MATTER_TSTAT_MB_REG_COUNT    (MATTER_TSTAT_MAP_COUNT * 2)  /* 12 registers */

#define	MODBUS_MATTER_MAP_REG_END    (MODBUS_MATTER_MAP_REG_BASE + MATTER_TSTAT_MB_REG_COUNT - 1)

/* Helper macros */
#define MB_REG_TYPE(idx)    (MODBUS_MATTER_MAP_REG_BASE + ((idx) * 2))
#define MB_REG_NUM(idx)     (MODBUS_MATTER_MAP_REG_BASE + ((idx) * 2) + 1)

/* Named register addresses for convenience */
#define MB_REG_LOCAL_TEMP_TYPE       MB_REG_TYPE(MATTER_TSTAT_MAP_LOCAL_TEMP)      /* 600 */
#define MB_REG_LOCAL_TEMP_NUM        MB_REG_NUM(MATTER_TSTAT_MAP_LOCAL_TEMP)       /* 601 */
#define MB_REG_HUMIDITY_TYPE         MB_REG_TYPE(MATTER_TSTAT_MAP_HUMIDITY)        /* 602 */
#define MB_REG_HUMIDITY_NUM          MB_REG_NUM(MATTER_TSTAT_MAP_HUMIDITY)         /* 603 */
#define MB_REG_HEAT_SETPOINT_TYPE    MB_REG_TYPE(MATTER_TSTAT_MAP_HEAT_SETPOINT)   /* 604 */
#define MB_REG_HEAT_SETPOINT_NUM     MB_REG_NUM(MATTER_TSTAT_MAP_HEAT_SETPOINT)    /* 605 */
#define MB_REG_COOL_SETPOINT_TYPE    MB_REG_TYPE(MATTER_TSTAT_MAP_COOL_SETPOINT)   /* 606 */
#define MB_REG_COOL_SETPOINT_NUM     MB_REG_NUM(MATTER_TSTAT_MAP_COOL_SETPOINT)    /* 607 */
#define MB_REG_MODE_TYPE             MB_REG_TYPE(MATTER_TSTAT_MAP_MODE)            /* 608 */
#define MB_REG_MODE_NUM              MB_REG_NUM(MATTER_TSTAT_MAP_MODE)             /* 609 */

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
