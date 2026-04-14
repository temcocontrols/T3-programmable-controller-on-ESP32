/**
 * @file matter_tstat.cpp
 * @brief Matter thermostat endpoint implementation.
 *
 * @author Bhavik Panchal (bhavikp@electrobittech.com)
 * @date 14-04-2026
 * @version 1.0
 */

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdarg.h>

#include <esp_matter.h>
#include <esp_matter_endpoint.h>
#include <app/server/Server.h>
#include <app/server/CommissioningWindowManager.h>
#include <platform/CHIPDeviceLayer.h>

#include "wifi.h"
#include "matter_tstat.h"
#include "user_data.h"

#ifdef u8
#undef u8
#endif

using namespace esp_matter;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters::Thermostat;

/* ------------------------------------------------------------------ */
/* static functions                                                   */
/* ------------------------------------------------------------------ */
static void MatterReportDataOnChange( void );

/* ------------------------------------------------------------------ */
/* Internal state                                                     */
/* ------------------------------------------------------------------ */
static uint16_t      s_ep_id = 0;
EXT_RAM_BSS_ATTR tstat_data_t  s_data  = {
    .local_temperature    = 2200,
    .outdoor_temperature  = 0,
    .heat_setpoint        = 2100,
    .cool_setpoint        = 2600,
    .system_mode          = TSTAT_MODE_OFF,
    .running_state        = TSTAT_RUNNING_IDLE,
    .occupancy            = 1,
    .setpoint_hold        = 0,
    .min_heat_setpoint    = 1600,  /* 16.00 °C */
    .max_heat_setpoint    = 3000,  /* 30.00 °C */
    .min_cool_setpoint    = 1600,
    .max_cool_setpoint    = 3200,  /* 32.00 °C */
};

typedef struct {
    uint8_t point_type;
    uint8_t point_number;
    int32_t last_value;
    bool valid;
} matter_tstat_map_state_t;

static constexpr uint8_t kMatterMapUnused = 0xFF;

static matter_tstat_map_t s_map[MATTER_TSTAT_MAP_COUNT] = {
    { kMatterMapUnused, 0 }, // MATTER_TSTAT_MAP_LOCAL_TEMP
    { kMatterMapUnused, 0 }, // MATTER_TSTAT_MAP_OUTDOOR_TEMP
    { kMatterMapUnused, 0 }, // MATTER_TSTAT_MAP_HEAT_SETPOINT
    { kMatterMapUnused, 0 }, // MATTER_TSTAT_MAP_COOL_SETPOINT
    { kMatterMapUnused, 0 }, // MATTER_TSTAT_MAP_MODE
    { kMatterMapUnused, 0 }, // MATTER_TSTAT_MAP_RUNNING_STATE
    { kMatterMapUnused, 0 }, // MATTER_TSTAT_MAP_OCCUPANCY
};

static matter_tstat_map_state_t s_map_state[MATTER_TSTAT_MAP_COUNT];

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */
static void tstat_log(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    debug_info(buf);
}

/* Push one attribute into the Matter data model */
static esp_err_t tstat_update_attr(uint32_t attr_id, esp_matter_attr_val_t val)
{
    return attribute::update(s_ep_id,
                             chip::app::Clusters::Thermostat::Id,
                             attr_id,
                             &val);
}

static bool tstat_read_map_value(const matter_tstat_map_t &map, int32_t &value)
{
    if (map.point_type > VAR) {
        return false;
    }

    Str_points_ptr ptr = put_io_buf((Point_type_equate)map.point_type, map.point_number);

    switch (map.point_type)
    {
        case IN:
            value = ptr.pin->value;
            return true;
        case OUT:
            value = ptr.pout->value;
            return true;
        case VAR:
            value = ptr.pvar->value;
            return true;
        default:
            return false;
    }
}

static bool tstat_map_value_changed(matter_tstat_map_id_t id, int32_t value)
{
    matter_tstat_map_state_t &state = s_map_state[id];
    const matter_tstat_map_t &map = s_map[id];

    if (!state.valid ||
        state.point_type != map.point_type ||
        state.point_number != map.point_number ||
        state.last_value != value)
    {
        state.point_type = map.point_type;
        state.point_number = map.point_number;
        state.last_value = value;
        state.valid = true;
        return true;
    }

    return false;
}

/* ------------------------------------------------------------------ */
/* Event / attribute callbacks                                         */
/* ------------------------------------------------------------------ */
static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type)
    {
        case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted:
            debug_info("Commissioning started");
            break;
        case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
            debug_info("Commissioning complete");
            break;
        case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
            debug_info("Commissioning failed - failsafe expired");
            break;
        case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
            debug_info("Fabric removed");
            break;
        default:
            break;
    }
}

static esp_err_t app_attribute_update_cb(attribute::callback_type_t type,
                                         uint16_t endpoint_id,
                                         uint32_t cluster_id,
                                         uint32_t attribute_id,
                                         esp_matter_attr_val_t *val,
                                         void *priv_data)
{
    /* Only care about our thermostat endpoint */
    if (endpoint_id != s_ep_id || cluster_id != chip::app::Clusters::Thermostat::Id)
        return ESP_OK;

    /* PRE_UPDATE: validate / clamp incoming controller writes */
    if (type == attribute::PRE_UPDATE) {
        if (attribute_id == Attributes::OccupiedHeatingSetpoint::Id) {
            if (val->val.i16 < s_data.min_heat_setpoint) val->val.i16 = s_data.min_heat_setpoint;
            if (val->val.i16 > s_data.max_heat_setpoint) val->val.i16 = s_data.max_heat_setpoint;
        }
        if (attribute_id == Attributes::OccupiedCoolingSetpoint::Id) {
            if (val->val.i16 < s_data.min_cool_setpoint) val->val.i16 = s_data.min_cool_setpoint;
            if (val->val.i16 > s_data.max_cool_setpoint) val->val.i16 = s_data.max_cool_setpoint;
        }
        return ESP_OK;
    }

    /* POST_UPDATE: controller changed something — update local state and notify app */
    if (type == attribute::POST_UPDATE) {
        switch (attribute_id) {

        case Attributes::OccupiedHeatingSetpoint::Id:
            s_data.heat_setpoint = val->val.i16;
            tstat_log("Heat setpoint <- %.2f °C", val->val.i16 / 100.0f);
            app_tstat_on_heat_setpoint_change(val->val.i16);
            break;

        case Attributes::OccupiedCoolingSetpoint::Id:
            s_data.cool_setpoint = val->val.i16;
            tstat_log("Cool setpoint <- %.2f °C", val->val.i16 / 100.0f);
            app_tstat_on_cool_setpoint_change(val->val.i16);
            break;

        case Attributes::SystemMode::Id:
            s_data.system_mode = val->val.u8;
            tstat_log("System mode <- %d", val->val.u8);
            app_tstat_on_mode_change((tstat_mode_t)val->val.u8);
            break;

        case Attributes::TemperatureSetpointHold::Id:
            s_data.setpoint_hold = val->val.u8;
            tstat_log("Setpoint hold <- %d", val->val.u8);
            app_tstat_on_setpoint_hold_change(val->val.u8);
            break;

        default:
            break;
        }
    }

    return ESP_OK;
}

static esp_err_t app_identification_cb(identification::callback_type_t type,
                                       uint16_t endpoint_id,
                                       uint8_t effect_id,
                                       uint8_t effect_variant,
                                       void *priv_data)
{
    return ESP_OK;
}

/* ------------------------------------------------------------------ */
/* Commissioning                                                        */
/* ------------------------------------------------------------------ */
static void open_commissioning_cb(intptr_t arg)
{
    auto &mgr = chip::Server::GetInstance().GetCommissioningWindowManager();
    if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0)
    {
        mgr.OpenBasicCommissioningWindow(
            chip::System::Clock::Seconds16(300),
            chip::CommissioningWindowAdvertisement::kAllSupported);
        debug_info("Commissioning window opened");
    }
    else
    {
        debug_info("Already commissioned - skipping");
    }
}

static void matter_clear_commissioning_cb(intptr_t arg)
{
    chip::Server::GetInstance().ScheduleFactoryReset();
}

void matter_clear_commissioning(void)
{
    chip::DeviceLayer::PlatformMgr().ScheduleWork(matter_clear_commissioning_cb, 0);
    debug_info("Factory reset scheduled");
}

static void matter_user_task(void *pvParameters)
{
    while (1)
    {
        MatterReportDataOnChange();
        vTaskDelay(pdMS_TO_TICKS(MATTER_SYNC_INTERVAL_MS));
    }
}

/* ------------------------------------------------------------------ */
/* Init                                                                */
/* ------------------------------------------------------------------ */
esp_err_t matter_tstat_init(void)
{
    node::config_t node_config;
    node_t *node = node::create(&node_config,
                                app_attribute_update_cb,
                                app_identification_cb);
    if (!node) { debug_info("Failed to create node"); return ESP_FAIL; }

    esp_matter::endpoint::thermostat::config_t cfg;
    cfg.thermostat.system_mode       = s_data.system_mode;
    cfg.thermostat.local_temperature = s_data.local_temperature;
    cfg.thermostat.feature_flags     = 0x03; /* Heat + Cool */
    cfg.thermostat.features.heating.occupied_heating_setpoint = s_data.heat_setpoint;
    cfg.thermostat.features.cooling.occupied_cooling_setpoint = s_data.cool_setpoint;

    endpoint_t *ep = esp_matter::endpoint::thermostat::create(
                        node, &cfg, ENDPOINT_FLAG_NONE, nullptr);
    if (!ep) { debug_info("Failed: thermostat EP"); return ESP_FAIL; }

    s_ep_id = endpoint::get_id(ep);
    tstat_log("Thermostat EP: %d", s_ep_id);

    esp_err_t err = esp_matter::start(app_event_cb);
    if (err != ESP_OK) { tstat_log("Matter start failed: %d", err); return err; }
    debug_info("Matter started");

    /* Push limits into data model after start */
    tstat_update_attr(Attributes::MinHeatSetpointLimit::Id,
                      esp_matter_int16(s_data.min_heat_setpoint));
    tstat_update_attr(Attributes::MaxHeatSetpointLimit::Id,
                      esp_matter_int16(s_data.max_heat_setpoint));
    tstat_update_attr(Attributes::MinCoolSetpointLimit::Id,
                      esp_matter_int16(s_data.min_cool_setpoint));
    tstat_update_attr(Attributes::MaxCoolSetpointLimit::Id,
                      esp_matter_int16(s_data.max_cool_setpoint));

    chip::DeviceLayer::PlatformMgr().ScheduleWork(open_commissioning_cb, 0);

    xTaskCreate(matter_user_task, "matter_user_task", 2048, NULL, 5, NULL);
    return ESP_OK;
}

/* ------------------------------------------------------------------ */
/* Report APIs — call from your sensor / control tasks                 */
/* ------------------------------------------------------------------ */
esp_err_t matter_tstat_report_temperature(int16_t temp_001c)
{
    s_data.local_temperature = temp_001c;
    tstat_log("LocalTemp -> %.2f °C", temp_001c / 100.0f);
    return tstat_update_attr(Attributes::LocalTemperature::Id,
                             esp_matter_int16(temp_001c));
}

esp_err_t matter_tstat_report_outdoor_temperature(int16_t temp_001c)
{
    s_data.outdoor_temperature = temp_001c;
    return tstat_update_attr(Attributes::OutdoorTemperature::Id,
                             esp_matter_int16(temp_001c));
}

esp_err_t matter_tstat_report_running_state(tstat_running_state_t state)
{
    s_data.running_state = state;
    tstat_log("RunningState -> 0x%02X", state);
    return tstat_update_attr(Attributes::ThermostatRunningState::Id,
                             esp_matter_bitmap16((uint16_t)state));
}

esp_err_t matter_tstat_report_occupancy(uint8_t occupied)
{
    s_data.occupancy = occupied;
    return tstat_update_attr(Attributes::Occupancy::Id,
                             esp_matter_bitmap8(occupied));
}

esp_err_t matter_tstat_report_heat_setpoint(int16_t sp_001c)
{
    s_data.heat_setpoint = sp_001c;
    tstat_log("HeatSP local -> %.2f °C", sp_001c / 100.0f);
    return tstat_update_attr(Attributes::OccupiedHeatingSetpoint::Id,
                             esp_matter_int16(sp_001c));
}

esp_err_t matter_tstat_report_cool_setpoint(int16_t sp_001c)
{
    s_data.cool_setpoint = sp_001c;
    tstat_log("CoolSP local -> %.2f °C", sp_001c / 100.0f);
    return tstat_update_attr(Attributes::OccupiedCoolingSetpoint::Id,
                             esp_matter_int16(sp_001c));
}

esp_err_t matter_tstat_report_mode(tstat_mode_t mode)
{
    s_data.system_mode = mode;
    tstat_log("Mode local -> %d", mode);
    return tstat_update_attr(Attributes::SystemMode::Id,
                             esp_matter_uint8((uint8_t)mode));
}

static void MatterReportDataOnChange( void )
{
    int32_t value = 0;

    if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_LOCAL_TEMP], value) &&
        tstat_map_value_changed(MATTER_TSTAT_MAP_LOCAL_TEMP, value))
    {
        matter_tstat_report_temperature((int16_t)value);
    }

    if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_OUTDOOR_TEMP], value) &&
        tstat_map_value_changed(MATTER_TSTAT_MAP_OUTDOOR_TEMP, value))
    {
        matter_tstat_report_outdoor_temperature((int16_t)value);
    }

    if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_HEAT_SETPOINT], value) &&
        tstat_map_value_changed(MATTER_TSTAT_MAP_HEAT_SETPOINT, value))
    {
        matter_tstat_report_heat_setpoint((int16_t)value);
    }

    if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_COOL_SETPOINT], value) &&
        tstat_map_value_changed(MATTER_TSTAT_MAP_COOL_SETPOINT, value))
    {
        matter_tstat_report_cool_setpoint((int16_t)value);
    }

    if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_MODE], value) &&
        tstat_map_value_changed(MATTER_TSTAT_MAP_MODE, value))
    {
        matter_tstat_report_mode((tstat_mode_t)value);
    }

    if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_RUNNING_STATE], value) &&
        tstat_map_value_changed(MATTER_TSTAT_MAP_RUNNING_STATE, value))
    {
        matter_tstat_report_running_state((tstat_running_state_t)value);
    }

    if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_OCCUPANCY], value) &&
        tstat_map_value_changed(MATTER_TSTAT_MAP_OCCUPANCY, value))
    {
        matter_tstat_report_occupancy((uint8_t)value);
    }
}
/* ------------------------------------------------------------------ */
/* State snapshot                                                      */
/* ------------------------------------------------------------------ */
tstat_data_t *matter_tstat_get_data(void) { return &s_data; }
matter_tstat_map_t *matter_tstat_get_map(void) { return s_map; }

/* ------------------------------------------------------------------ */
/* Weak default callbacks — override in your app                       */
/* ------------------------------------------------------------------ */
__attribute__((weak)) void app_tstat_on_mode_change(tstat_mode_t mode)             { (void)mode; }
__attribute__((weak)) void app_tstat_on_heat_setpoint_change(int16_t sp)           { (void)sp;   }
__attribute__((weak)) void app_tstat_on_cool_setpoint_change(int16_t sp)           { (void)sp;   }
__attribute__((weak)) void app_tstat_on_setpoint_hold_change(uint8_t hold)         { (void)hold; }
