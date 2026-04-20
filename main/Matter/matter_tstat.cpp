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
#include <algorithm>

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

#define TAG "TSTAT"

using namespace esp_matter;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters::Thermostat;

/* ------------------------------------------------------------------ */
/* static functions                                                   */
/* ------------------------------------------------------------------ */
static bool tstat_read_map_value(const matter_tstat_map_t &map, int32_t &value);

/* ------------------------------------------------------------------ */
/* Internal state                                                     */
/* ------------------------------------------------------------------ */
static uint16_t s_ep_id     = 0;   // thermostat
static uint16_t s_hum_ep_id = 0;   // humidity

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
            if (path.mAttributeId == Clusters::Thermostat::Attributes::LocalTemperature::Id)
            {
                int32_t temp = 0;
                if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_LOCAL_TEMP], temp))
                {
                    int16_t temp_16 = temp / 10;

                    temp_16 = std::clamp(temp_16,
                        (int16_t)TSTAT_MIN_LOCAL_TEMP,
                        (int16_t)TSTAT_MAX_LOCAL_TEMP);

                    ESP_LOGI(TAG, "Providing LocalTemperature: %d ,tmp %d", temp_16, temp);
                    return encoder.Encode(temp_16);
                }
            }
            else if (path.mAttributeId == Clusters::Thermostat::Attributes::OutdoorTemperature::Id)
            {
                int32_t temp = 0;
                if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_OUTDOOR_TEMP], temp))
                {
                    int16_t temp_16 = temp / 10;

                    temp_16 = std::clamp(temp_16,
                        (int16_t)TSTAT_MIN_OUTDOOR_TEMP,
                        (int16_t)TSTAT_MAX_OUTDOOR_TEMP);

                    ESP_LOGI(TAG, "Providing OutdoorTemperature: %d ,tmp %d", temp_16, temp);
                    return encoder.Encode(temp_16);
                }
            }
            else if (path.mAttributeId == Clusters::Thermostat::Attributes::OccupiedHeatingSetpoint::Id)
            {
                int32_t temp = 0;
                if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_HEAT_SETPOINT], temp))
                {
                    int16_t temp_16 = temp / 10;

                    temp_16 = std::clamp(temp_16,
                        (int16_t)TSTAT_MIN_HEAT_SETPOINT,
                        (int16_t)TSTAT_MAX_HEAT_SETPOINT);

                    ESP_LOGI(TAG, "Providing HeatSetpoint: %d ,tmp %d", temp_16, temp);
                    return encoder.Encode(temp_16);
                }
            }
            else if (path.mAttributeId == Clusters::Thermostat::Attributes::OccupiedCoolingSetpoint::Id)
            {
                int32_t temp = 0;
                if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_COOL_SETPOINT], temp))
                {
                    int16_t temp_16 = temp / 10;

                    temp_16 = std::clamp(temp_16,
                        (int16_t)TSTAT_MIN_COOL_SETPOINT,
                        (int16_t)TSTAT_MAX_COOL_SETPOINT);

                    ESP_LOGI(TAG, "Providing CoolSetpoint: %d ,tmp %d", temp_16, temp);
                    return encoder.Encode(temp_16);
                }
            }
            else if (path.mAttributeId == Clusters::Thermostat::Attributes::SystemMode::Id)
            {
                int32_t mode = 0;
                if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_MODE], mode))
                {
                    uint8_t matter_mode;

                    switch (mode)
                    {
                        case 0: matter_mode = to_underlying(SystemModeEnum::kOff); break;
                        case 1: matter_mode = to_underlying(SystemModeEnum::kHeat); break;
                        case 2: matter_mode = to_underlying(SystemModeEnum::kCool); break;
                        case 3: matter_mode = to_underlying(SystemModeEnum::kAuto); break;
                        default:
                            matter_mode = to_underlying(SystemModeEnum::kOff);
                            ESP_LOGW(TAG, "Invalid mode %ld, defaulting to OFF", mode);
                            break;
                    }

                    return encoder.Encode(matter_mode);
                }
            }
        }
        ESP_LOGW(TAG, "Attribute read for cluster %d attr %d not overridden, using default handler",
                 path.mClusterId, path.mAttributeId);

        return CHIP_NO_ERROR;
    }
};

class HumidityAttrOverride : public AttributeAccessInterface
{
public:
    HumidityAttrOverride() :
        AttributeAccessInterface(Optional<EndpointId>::Missing(),
                                 Clusters::RelativeHumidityMeasurement::Id) {}

    CHIP_ERROR Read(const ConcreteReadAttributePath & path,
                    AttributeValueEncoder & encoder) override
    {
        if (path.mAttributeId ==
            Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Id)
        {
            int32_t temp = 0;
            if (tstat_read_map_value(s_map[MATTER_TSTAT_MAP_HUMIDITY], temp))
            {
                int16_t humidity = std::clamp(
                    static_cast<int16_t>(temp),
                    (int16_t)HUMIDITY_MIN_MEASURED_VALUE,
                    (int16_t)HUMIDITY_MAX_MEASURED_VALUE);

                ESP_LOGI(TAG, "Providing Humidity: %d", humidity);
                return encoder.Encode(humidity);
            }
        }
        return CHIP_NO_ERROR;
    }
};

static ThermostatAttrOverride g_tstat_override;
static HumidityAttrOverride g_humidity_override;

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
            // Print all map entries for debug
            int count = sizeof(s_map) / sizeof(s_map[0]);
            for (int i = 0; i < count; i++) {
                ESP_LOGI(TAG,"Map[%d]: type=%d, number=%d", i, s_map[i].point_type, s_map[i].point_number);
            }
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
    if (!ptr.pin) return false;  // adjust based on your union/struct type

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
    if (!ptr.pin) return;

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
                uint8_t sys_mode;
                switch(val->val.u8)
                {
                    case to_underlying(SystemModeEnum::kOff):  sys_mode = 0; break;
                    case to_underlying(SystemModeEnum::kHeat): sys_mode = 1; break;
                    case to_underlying(SystemModeEnum::kCool): sys_mode = 2; break;
                    case to_underlying(SystemModeEnum::kAuto): sys_mode = 3; break;
                    default:
                        sys_mode = 0; // fallback to OFF
                        ESP_LOGW("TSTAT", "Unknown SystemMode received: %d, defaulting to OFF", val->val.u8);
                        break;
                }
                tstat_write_map_value(MATTER_TSTAT_MAP_MODE, (int32_t)sys_mode);
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
        // TODO: for further optimization
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

        // Create a SEPARATE endpoint for humidity sensor
    esp_matter::endpoint::humidity_sensor::config_t hum_ep_cfg;
    hum_ep_cfg.relative_humidity_measurement.measured_value    = s_data.measured_humidity;
    hum_ep_cfg.relative_humidity_measurement.min_measured_value = s_data.min_humidity;
    hum_ep_cfg.relative_humidity_measurement.max_measured_value = s_data.max_humidity;

    endpoint_t *hum_ep = esp_matter::endpoint::humidity_sensor::create(
        node, &hum_ep_cfg, ENDPOINT_FLAG_NONE, nullptr);
    if (!hum_ep) { debug_info("Failed: humidity EP"); return ESP_FAIL; }

    s_hum_ep_id = endpoint::get_id(hum_ep);
    tstat_log("Humidity EP: %d", s_hum_ep_id);

    s_ep_id = endpoint::get_id(ep);
    tstat_log("Thermostat EP: %d", s_ep_id);

    AttributeAccessInterfaceRegistry::Instance().Register(&g_tstat_override);
    AttributeAccessInterfaceRegistry::Instance().Register(&g_humidity_override);

    esp_err_t err = esp_matter::start(app_event_cb);
    if (err != ESP_OK) { tstat_log("Matter start failed: %d", err); return err; }
    debug_info("Matter started");

    chip::DeviceLayer::PlatformMgr().ScheduleWork(open_commissioning_cb, 0);

   xTaskCreate(matter_user_task, "matter_user_task", 4096, NULL, 3, NULL);

    return ESP_OK;
}

/* -------------- End of file -------------------------------------------------- */
