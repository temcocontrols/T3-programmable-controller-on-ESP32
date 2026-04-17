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
#include <nvs_flash.h>
#include <nvs.h>

#include <app/AttributeAccessInterface.h>
#include <app/ConcreteAttributePath.h>
#include <app/AttributeValueEncoder.h>
#include <app/AttributeAccessInterfaceRegistry.h>

#include "wifi.h"
#include "matter_tstat.h"
#include "user_data.h"

#ifdef u8
#undef u8
#endif

#define MATTER_TSTAT_NVS_NAMESPACE   "tstat_map"
#define MATTER_TSTAT_NVS_KEY         "map_data"

using namespace esp_matter;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters::Thermostat;

/* ------------------------------------------------------------------ */
/* static functions                                                   */
/* ------------------------------------------------------------------ */
static void matter_report_data_on_change( void );
static bool tstat_read_map_value(const matter_tstat_map_t &map, int32_t &value);

/* ------------------------------------------------------------------ */
/* Internal state                                                     */
/* ------------------------------------------------------------------ */
static uint16_t      s_ep_id = 0;
static bool          s_device_ready = false;
static EXT_RAM_BSS_ATTR tstat_data_t  s_data  = {
    .local_temperature    = 2200,
    .outdoor_temperature  = 0,
    .heat_setpoint        = 2100,
    .cool_setpoint        = 2600,
    .measured_humidity    = 5000,   /* 50.00% */
    .min_humidity         = 0,
    .max_humidity         = 10000,
    .system_mode          = TSTAT_MODE_COOL,
    .running_state        = TSTAT_RUNNING_IDLE,
    .occupancy            = 1,
    .setpoint_hold        = 0,
    .min_heat_setpoint    = 0,
    .max_heat_setpoint    = 3200,  /* 32.00 °C */
    .min_cool_setpoint    = 0,
    .max_cool_setpoint    = 3200,  /* 32.00 °C */
};

typedef struct {
    uint8_t point_type;
    uint8_t point_number;
    int32_t last_value;
    bool valid;
} matter_tstat_map_state_t;

typedef struct {
    uint16_t ep_id;
    uint32_t cluster_id;
    uint32_t attr_id;
    esp_matter_attr_val_t val;
} attr_update_ctx_t;

static constexpr uint8_t kMatterMapUnused = 0xFF;

static EXT_RAM_BSS_ATTR matter_tstat_map_t s_map[MATTER_TSTAT_MAP_COUNT] = {
    { kMatterMapUnused, 0 }, // MATTER_TSTAT_MAP_LOCAL_TEMP
    { kMatterMapUnused, 0 }, // MATTER_TSTAT_MAP_OUTDOOR_TEMP
    { kMatterMapUnused, 0 }, // MATTER_TSTAT_MAP_HUMIDITY
    { kMatterMapUnused, 0 }, // MATTER_TSTAT_MAP_HEAT_SETPOINT
    { kMatterMapUnused, 0 }, // MATTER_TSTAT_MAP_COOL_SETPOINT
    { kMatterMapUnused, 0 }, // MATTER_TSTAT_MAP_MODE
    { kMatterMapUnused, 0 }, // MATTER_TSTAT_MAP_RUNNING_STATE
    { kMatterMapUnused, 0 }, // MATTER_TSTAT_MAP_OCCUPANCY
};

static const matter_tstat_map_t s_map_defaults[MATTER_TSTAT_MAP_COUNT] = {
    { IN,  8  }, // MATTER_TSTAT_MAP_LOCAL_TEMP
    { IN,  6  }, // MATTER_TSTAT_MAP_OUTDOOR_TEMP
    { IN,  10 }, // MATTER_TSTAT_MAP_HUMIDITY
    { VAR, 0  }, // MATTER_TSTAT_MAP_HEAT_SETPOINT
    { VAR, 0  }, // MATTER_TSTAT_MAP_COOL_SETPOINT
    { VAR, 1  }, // MATTER_TSTAT_MAP_MODE
    { VAR, 2  }, // MATTER_TSTAT_MAP_RUNNING_STATE
    { IN,  12 }, // MATTER_TSTAT_MAP_OCCUPANCY
};

static EXT_RAM_BSS_ATTR matter_tstat_map_state_t s_map_state[MATTER_TSTAT_MAP_COUNT];

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */
static void tstat_log(const char *fmt, ...)
{
    char buf[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    debug_info(buf);
}

using namespace chip;
using namespace chip::app;

class ThermostatAttrOverride : public AttributeAccessInterface
{
public:
    ThermostatAttrOverride() :
        AttributeAccessInterface(Optional<EndpointId>::Missing(), Clusters::Thermostat::Id) {}

    CHIP_ERROR Read(const ConcreteReadAttributePath & path,
                    AttributeValueEncoder & encoder) override
    {
        if (path.mClusterId == Clusters::Thermostat::Id)
        {
            if(path.mAttributeId == Clusters::Thermostat::Attributes::LocalTemperature::Id)
            {
                int32_t temp = 0;
                if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_LOCAL_TEMP], temp))
                {
                    ESP_LOGI("TSTAT", "Providing LocalTemperature: %d", temp);
                    int16_t temp_16 = static_cast<int16_t>(temp/10);
                    return encoder.Encode(temp);
                }
            }
            else if(path.mAttributeId == Clusters::Thermostat::Attributes::OutdoorTemperature::Id)
            {
                int32_t temp = 0;
                if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_OUTDOOR_TEMP], temp))
                {
                    ESP_LOGI("TSTAT", "Providing OutdorrTemperature: %d", temp);
                    int16_t temp_16 = static_cast<int16_t>(temp/10);
                    return encoder.Encode(temp);
                }
            }
        }
        else if(path.mClusterId == Clusters::RelativeHumidityMeasurement::Id)
        {
            if(path.mAttributeId == Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id)
            {
                int32_t temp = 0;
                if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_HUMIDITY], temp))
                {
                    ESP_LOGI("TSTAT", "Providing Humidity: %d", temp);
                    int16_t temp_16 = static_cast<int16_t>(temp/10);
                    return encoder.Encode(temp);
                }
            }
        }

        ESP_LOGW("TSTAT", "Attribute read for cluster %d attr %d not overridden, using default handler",
                         path.mClusterId, path.mAttributeId);

        return CHIP_NO_ERROR; // fallback to default handler
    }
};

static ThermostatAttrOverride g_tstat_override;

static void tstat_attr_update_cb(intptr_t arg)
{
    attr_update_ctx_t *ctx = (attr_update_ctx_t *)arg;
    attribute::update(ctx->ep_id, ctx->cluster_id, ctx->attr_id, &ctx->val);
    delete ctx;
}

/* Push one attribute into the Matter data model */
static esp_err_t tstat_update_attr(uint32_t attr_id, esp_matter_attr_val_t val)
{
    attr_update_ctx_t *ctx = new attr_update_ctx_t();
    if (!ctx) return ESP_ERR_NO_MEM;

    ctx->ep_id      = s_ep_id;
    ctx->cluster_id = chip::app::Clusters::Thermostat::Id;
    ctx->attr_id    = attr_id;
    ctx->val        = val;

    CHIP_ERROR err = chip::DeviceLayer::PlatformMgr().ScheduleWork(
                         tstat_attr_update_cb, (intptr_t)ctx);
    if (err != CHIP_NO_ERROR) {
        delete ctx;
        return ESP_FAIL;
    }
    return ESP_OK;
}

static bool tstat_map_entry_valid(const matter_tstat_map_t &m)
{
    /* point_type must be IN, OUT, or VAR (0,1,2 typically) */
    return (m.point_type <= VAR);
}

static bool tstat_map_all_valid(const matter_tstat_map_t *map)
{
    for (int i = 0; i < MATTER_TSTAT_MAP_COUNT; i++) {
        if (!tstat_map_entry_valid(map[i])) return false;
    }
    return true;
}

esp_err_t matter_tstat_map_save(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(MATTER_TSTAT_NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) { tstat_log("NVS open failed: %d", err); return err; }

    err = nvs_set_blob(h, MATTER_TSTAT_NVS_KEY,
                       s_map, sizeof(s_map));
    if (err == ESP_OK) err = nvs_commit(h);

    nvs_close(h);
    tstat_log("Map saved to NVS: %s", esp_err_to_name(err));
    return err;
}

esp_err_t matter_tstat_map_load(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(MATTER_TSTAT_NVS_NAMESPACE, NVS_READONLY, &h);

    if (err == ESP_OK) {
        matter_tstat_map_t tmp[MATTER_TSTAT_MAP_COUNT];
        size_t len = sizeof(tmp);

        err = nvs_get_blob(h, MATTER_TSTAT_NVS_KEY, tmp, &len);
        nvs_close(h);

        if (err == ESP_OK &&
            len == sizeof(s_map) &&
            tstat_map_all_valid(tmp))
        {
            memcpy(s_map, tmp, sizeof(s_map));
            debug_info("Map loaded from NVS");
            return ESP_OK;
        }

        debug_info("NVS map invalid or wrong size — loading defaults");
    } else {
        tstat_log("NVS open failed (%s) — loading defaults", esp_err_to_name(err));
    }

    return matter_tstat_map_reset_defaults();
}

esp_err_t matter_tstat_map_reset_defaults(void)
{
    memcpy(s_map, s_map_defaults, sizeof(s_map));
    debug_info("Map reset to defaults");
    return matter_tstat_map_save();   /* persist immediately */
}

/* Called when Modbus master WRITES a register in range 600-615 */
void matter_tstat_map_from_modbus(uint16_t reg, uint16_t value)
{
    /* Check within range */
    if (reg < MODBUS_MATTER_MAP_REG_BASE || reg >= MODBUS_MATTER_MAP_REG_END)
    {
        return;
    }

    uint8_t offset = reg - MODBUS_MATTER_MAP_REG_BASE;
    uint8_t idx    = offset / 2;       /* which map entry */
    bool    is_type = (offset % 2 == 0); /* even = type, odd = number */

    if (is_type) {
        if (value > VAR) {
            tstat_log("MB reg %d: invalid point_type=%d", reg, value);
            return;
        }
        s_map[idx].point_type = (uint8_t)value;
        tstat_log("MB reg %d -> map[%d].point_type = %d", reg, idx, value);
    } else {
        s_map[idx].point_number = (uint8_t)value;
        tstat_log("MB reg %d -> map[%d].point_number = %d", reg, idx, value);
    }

    matter_tstat_map_save();
}

/* Called when Modbus master READS a register in range 600-615 */
uint16_t matter_tstat_map_to_modbus(uint16_t reg)
{
    if (reg < MODBUS_MATTER_MAP_REG_BASE || reg >= MODBUS_MATTER_MAP_REG_END)
    {
        return 0xFFFF;  /* out of range */
    }

    uint8_t offset  = reg - MODBUS_MATTER_MAP_REG_BASE;
    uint8_t idx     = offset / 2;
    bool    is_type = (offset % 2 == 0);

    return is_type ? s_map[idx].point_type
                   : s_map[idx].point_number;
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
            s_device_ready = true;
            break;
        case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
            debug_info("Commissioning failed - failsafe expired");
            break;
        case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
            debug_info("Fabric removed");
            s_device_ready = false;
            break;
        case chip::DeviceLayer::DeviceEventType::kServerReady:
            debug_info("Matter server ready");
            break;
        default:
            break;
    }
}

static void tstat_write_map_value(matter_tstat_map_id_t id, int32_t value)
{
    const matter_tstat_map_t &map = s_map[id];
    if (map.point_type > VAR) return;

    Str_points_ptr ptr = put_io_buf((Point_type_equate)map.point_type, map.point_number);

    switch (map.point_type) {
        case IN:  ptr.pin->value  = value; break;
        case OUT: ptr.pout->value = value; break;
        case VAR: ptr.pvar->value = value; break;
        default: break;
    }
}

static esp_err_t app_attribute_update_cb(attribute::callback_type_t type,
                                         uint16_t endpoint_id,
                                         uint32_t cluster_id,
                                         uint32_t attribute_id,
                                         esp_matter_attr_val_t *val,
                                         void *priv_data)
{
    if (endpoint_id != s_ep_id || cluster_id != chip::app::Clusters::Thermostat::Id)
        return ESP_OK;

    if (type == attribute::PRE_UPDATE)
    {
        if (attribute_id == chip::app::Clusters::Thermostat::Attributes::LocalTemperature::Id)
        {
            debug_info("Intercept LocalTemperature update\n");
            // Inject your real sensor value
            val->val.i16 = 2200;
            return ESP_OK;
        }
        return ESP_OK;
    }
    if (type == attribute::POST_UPDATE)
    {
        switch (attribute_id)
        {

            case Attributes::OccupiedHeatingSetpoint::Id:
                s_data.heat_setpoint = val->val.i16;
                tstat_log("Heat setpoint <- %.2f C", val->val.i16 / 100.0f);
                /* write back to mapped point (Matter is *100, point is *1000) */
                tstat_write_map_value(MATTER_TSTAT_MAP_HEAT_SETPOINT, (int32_t)val->val.i16);
                break;

            case Attributes::OccupiedCoolingSetpoint::Id:
                s_data.cool_setpoint = val->val.i16;
                tstat_log("Cool setpoint <- %.2f C", val->val.i16 / 100.0f);
                tstat_write_map_value(MATTER_TSTAT_MAP_COOL_SETPOINT, (int32_t)val->val.i16);
                break;

            case Attributes::SystemMode::Id:
                s_data.system_mode = val->val.u8;
                tstat_log("System mode <- %d", val->val.u8);
                tstat_write_map_value(MATTER_TSTAT_MAP_MODE, (int32_t)val->val.u8);
                break;

            case Attributes::TemperatureSetpointHold::Id:
                s_data.setpoint_hold = val->val.u8;
                tstat_log("Setpoint hold <- %d", val->val.u8);
                break;

            default:
                break;
        }
    }
    else if (type == attribute::READ)
    {
       debug_info("attribute_update_cb type : READ \n");
    }
    else if (type == attribute::WRITE)
    {
       debug_info("attribute_update_cb type : WRITE \n");
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
        s_device_ready = true;
    }
}

static void matter_clear_commissioning_cb(intptr_t arg)
{
    esp_matter::factory_reset();
}

void matter_clear_commissioning(void)
{
    chip::DeviceLayer::PlatformMgr().ScheduleWork(matter_clear_commissioning_cb, 0);
    debug_info("Factory reset scheduled");
}

static void matter_user_task(void *pvParameters)
{
    while (!s_device_ready) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    matter_tstat_map_load();
    while (1)
    {
        matter_report_data_on_change();
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

    AttributeAccessInterfaceRegistry::Instance().Register(&g_tstat_override);

    esp_matter::endpoint::thermostat::config_t cfg;
    cfg.thermostat.system_mode       = s_data.system_mode;
    cfg.thermostat.local_temperature = s_data.local_temperature;
    cfg.thermostat.feature_flags = cluster::thermostat::feature::heating::get_id() |
                                  cluster::thermostat::feature::cooling::get_id() ;
    cfg.thermostat.control_sequence_of_operation = 4; // heat-cool auto

    endpoint_t *ep = esp_matter::endpoint::thermostat::create(
                        node, &cfg, ENDPOINT_FLAG_NONE, nullptr);
    if (!ep) { debug_info("Failed: thermostat EP"); return ESP_FAIL; }

    cluster_t *tstat_cluster = cluster::get(ep, chip::app::Clusters::Thermostat::Id);
    if (!tstat_cluster) {
        debug_info("Failed to get thermostat cluster");
        return ESP_FAIL;
    }

    cluster::relative_humidity_measurement::config_t hum_cfg;
    hum_cfg.measured_value    = s_data.measured_humidity;
    hum_cfg.min_measured_value = s_data.min_humidity;
    hum_cfg.max_measured_value = s_data.max_humidity;

    cluster_t *hum_cluster = cluster::relative_humidity_measurement::create(
        ep, &hum_cfg, CLUSTER_FLAG_SERVER);
    if (!hum_cluster) {
        debug_info("Failed to create humidity cluster");
        return ESP_FAIL;
    }

    s_ep_id = endpoint::get_id(ep);
    tstat_log("Thermostat EP: %d", s_ep_id);

    esp_err_t err = esp_matter::start(app_event_cb);
    if (err != ESP_OK) { tstat_log("Matter start failed: %d", err); return err; }
    debug_info("Matter started");

    chip::DeviceLayer::PlatformMgr().ScheduleWork(open_commissioning_cb, 0);

   xTaskCreate(matter_user_task, "matter_user_task", 4096, NULL, 3, NULL);

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

esp_err_t matter_tstat_report_humidity(uint16_t humidity_001pct)
{
    s_data.measured_humidity = humidity_001pct;
    tstat_log("Humidity -> %.2f %%", humidity_001pct / 100.0f);

    esp_matter_attr_val_t val = esp_matter_uint16(humidity_001pct);  // store first

    attr_update_ctx_t *ctx = new attr_update_ctx_t();
    if (!ctx) return ESP_ERR_NO_MEM;

    ctx->ep_id      = s_ep_id;
    ctx->cluster_id = chip::app::Clusters::RelativeHumidityMeasurement::Id;
    ctx->attr_id    = chip::app::Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id;
    ctx->val        = val;

    CHIP_ERROR err = chip::DeviceLayer::PlatformMgr().ScheduleWork(
                         tstat_attr_update_cb, (intptr_t)ctx);
    if (err != CHIP_NO_ERROR) {
        delete ctx;
        return ESP_FAIL;
    }
    return ESP_OK;
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

static void matter_report_data_on_change(void)
{
    int32_t value = 0;

    // /* Temperature: raw *1000, Matter needs *100, valid -20C to 80C */
    // if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_LOCAL_TEMP], value) &&
    //     tstat_map_value_changed(MATTER_TSTAT_MAP_LOCAL_TEMP, value))
    // {
    //     tstat_log("Raw local temp: %d", (int)value);
    //     if (value >= -20000 && value <= 80000) {
    //         matter_tstat_report_temperature((int16_t)(value / 10));
    //     } else {
    //         tstat_log("LocalTemp invalid: %d", (int)value);
    //     }
    // }

    // /* Outdoor temp: same scale as local */
    // else if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_OUTDOOR_TEMP], value) &&
    //     tstat_map_value_changed(MATTER_TSTAT_MAP_OUTDOOR_TEMP, value))
    // {
    //     tstat_log("Raw outdoor temp: %d", (int)value);
    //     if (value >= -20000 && value <= 80000) {
    //         matter_tstat_report_outdoor_temperature((int16_t)(value / 10));
    //     } else {
    //         tstat_log("OutdoorTemp invalid: %d", (int)value);
    //     }
    // }

    /* Heat setpoint: VAR point is already *100 (no divide needed) */
    if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_HEAT_SETPOINT], value) &&
        tstat_map_value_changed(MATTER_TSTAT_MAP_HEAT_SETPOINT, value))
    {
        tstat_log("Raw heat setpoint: %d", (int)value);
        value = value / 100; /* map value to 0.1 units for Matter (point is *1000, Matter is *100) */
        if (value >= s_data.min_heat_setpoint && value <= s_data.max_heat_setpoint) {
            matter_tstat_report_heat_setpoint((int16_t)value);
        } else {
            // tstat_write_map_value(MATTER_TSTAT_MAP_HEAT_SETPOINT, (int32_t)s_data.max_heat_setpoint);
            tstat_log("HeatSP invalid: %d", (int)value);
        }
    }

    /* Cool setpoint: same as heat */
    else if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_COOL_SETPOINT], value) &&
        tstat_map_value_changed(MATTER_TSTAT_MAP_COOL_SETPOINT, value))
    {
        tstat_log("Raw cool setpoint: %d", (int)value);
        value = value / 100; /* map value to 0.1 units for Matter (point is *1000, Matter is *100) */
        if (value >= s_data.min_cool_setpoint && value <= s_data.max_cool_setpoint) {
            matter_tstat_report_cool_setpoint((int16_t)value);
        } else {
            // tstat_write_map_value(MATTER_TSTAT_MAP_COOL_SETPOINT, (int32_t)s_data.min_cool_setpoint);
            tstat_log("CoolSP invalid: %d", (int)value);
        }
    }

    // /* Humidity: raw *1000, Matter needs *100, valid 0-100% */
    // else if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_HUMIDITY], value) &&
    //     tstat_map_value_changed(MATTER_TSTAT_MAP_HUMIDITY, value))
    // {
    //     tstat_log("Raw humidity: %d", (int)value);
    //     if (value >= 0 && value <= 100000) {
    //         matter_tstat_report_humidity((uint16_t)(value));
    //     } else {
    //         tstat_log("Humidity invalid: %d", (int)value);
    //     }
    // }

    /* Mode: valid 0-9 (Matter SystemMode enum) */
    else if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_MODE], value) &&
        tstat_map_value_changed(MATTER_TSTAT_MAP_MODE, value))
    {
        tstat_log("Raw mode: %d", (int)value);
        if (value >= 0 && value <= 9) {
            matter_tstat_report_mode((tstat_mode_t)value);
        } else {
            tstat_log("Mode invalid: %d", (int)value);
        }
    }

    /* Running state: bitmap16, valid 0-0x7F */
    else if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_RUNNING_STATE], value) &&
        tstat_map_value_changed(MATTER_TSTAT_MAP_RUNNING_STATE, value))
    {
        tstat_log("Raw running state: 0x%02X", (int)value);
        if (value >= 0 && value <= 0x7F) {
            matter_tstat_report_running_state((tstat_running_state_t)value);
        } else {
            tstat_log("RunningState invalid: %d", (int)value);
        }
    }

    /* Occupancy: bitmap8, valid 0 or 1 */
    else if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_OCCUPANCY], value) &&
        tstat_map_value_changed(MATTER_TSTAT_MAP_OCCUPANCY, value))
    {
        tstat_log("Raw occupancy: %d", (int)value);
        if (value == 0 || value == 1) {
            matter_tstat_report_occupancy((uint8_t)value);
        } else {
            tstat_log("Occupancy invalid: %d", (int)value);
        }
    }
}


/* -------------- End of file -------------------------------------------------- */
