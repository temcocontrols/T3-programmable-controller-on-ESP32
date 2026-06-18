/**
 * @file Mqtt_Handler.c
 * @brief Implementation of the MQTT Client handler for HiveMQ broker integration.
 *
 * Configures the esp-mqtt client to connect to HiveMQ, handles connection
 * events, subscribes to/publishes on test topics, and implements future-proof
 * trend log sending via JSON serialization.
 *
 * @author Antigravity AI Coding Assistant
 * @date June 2026
 */

#include "Mqtt_Handler.h"
#include "mqtt_client.h"
#include "user_data.h"
#include "esp_log.h"
#include "esp_event.h"
#include "cJSON.h"
#include "trendlog.h"
#include "cov.h"
#include "bactext.h"
#include "define.h"
#include "handlers.h"

extern  EventGroupHandle_t s_wifi_event_group;
extern TaskHandle_t main_task_handle[20];
static volatile bool mqtt_task_exit = false;

/** Event bit for WiFi connected with IP. */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/**
 * @brief Global BACnet trend logs data array.
 * Defined in trendlog.c.
 */
extern TL_DATA_REC Logs[MAX_TREND_LOGS][TL_MAX_ENTRIES];

/**
 * @brief Global count of active trend logs in the system.
 * Defined in trendlog.c.
 */
extern uint8_t TRENDLOGS;

static const char *TAG = "MQTT_HANDLER";

/**
 * @brief The internal MQTT client handle.
 */
static esp_mqtt_client_handle_t s_mqtt_client = NULL;

/**
 * @brief Flag indicating if the MQTT client is currently connected.
 */
static bool s_connected = false;

// External BACnet functions and structures needed for broadcasting MQTT-subscribed changes back to BACnet
extern int Send_UCOV_Notify(uint8_t * buffer, BACNET_COV_DATA * cov_data, uint8_t protocal);
extern void udp_client_send(uint16_t time);
#define BAC_IP_CLIENT 2

#define MQTT_DEBUG_EN 1

#if MQTT_DEBUG_EN
#define MQTT_DBG(fmt, ...) ESP_LOGI(TAG, "[DEBUG] " fmt, ##__VA_ARGS__)
#else
#define MQTT_DBG(fmt, ...)
#endif

// NVS storage includes for persisting subscriptions
#include "nvs_flash.h"
#include "nvs.h"

#define MQTT_FLASH_PERSIST_MIN_LIFETIME (30 * 60) // 30 minutes in seconds

typedef struct {
    uint8_t object_type;     // 0 to 5 (matches BACnet types)
    uint16_t instance;       // 1-based instance
    uint32_t lifetime;       // Remaining lifetime in seconds
    int32_t last_value;      // Last value for change detection
    bool is_active;          // Is slot active
    bool persist;            // Should persist in NVS
} MQTT_Subscription;

#define MAX_MQTT_SUBSCRIPTIONS 20
static MQTT_Subscription s_mqtt_subscriptions[MAX_MQTT_SUBSCRIPTIONS];

static void mqtt_save_subscriptions_to_flash(void)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace 'storage' for writing: %s", esp_err_to_name(err));
        return;
    }

    // Create a temporary copy to filter out transient subscriptions
    MQTT_Subscription temp_subs[MAX_MQTT_SUBSCRIPTIONS];
    memcpy(temp_subs, s_mqtt_subscriptions, sizeof(s_mqtt_subscriptions));

    for (int i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++) {
        if (temp_subs[i].is_active && !temp_subs[i].persist) {
            temp_subs[i].is_active = false; // Do not persist transient subscriptions
        }
    }

    err = nvs_set_blob(my_handle, "mqtt_subs", temp_subs, sizeof(temp_subs));
    if (err == ESP_OK) {
        nvs_commit(my_handle);
        MQTT_DBG("Saved long-lived MQTT subscriptions to NVS");
    } else {
        ESP_LOGE(TAG, "Failed to save MQTT subscriptions to NVS: %s", esp_err_to_name(err));
    }
    nvs_close(my_handle);
}

static void mqtt_load_subscriptions_from_flash(void)
{
    nvs_handle_t my_handle;
    esp_err_t err;
    size_t required_size = sizeof(s_mqtt_subscriptions);

    memset(s_mqtt_subscriptions, 0, sizeof(s_mqtt_subscriptions));

    err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS namespace 'storage' not found or failed to open (clean boot?): %s", esp_err_to_name(err));
        return;
    }

    err = nvs_get_blob(my_handle, "mqtt_subs", s_mqtt_subscriptions, &required_size);
    if (err == ESP_OK) {
        MQTT_DBG("Loaded MQTT subscriptions from flash");
    } else {
        ESP_LOGI(TAG, "No MQTT subscriptions found in NVS (or size mismatch): %s", esp_err_to_name(err));
    }
    nvs_close(my_handle);
}

static bool mqtt_get_point_value(uint8_t object_type, uint16_t instance, int32_t *out_value)
{
    int index = instance - 1;
    if (index < 0) return false;

    switch (object_type) {
        case 0: // Analog Input
            if (index < MAX_INS && inputs[index].range > 0 && inputs[index].digital_analog == 1) {
                *out_value = inputs[index].value;
                return true;
            }
            break;
        case 1: // Analog Output
            if (index < MAX_OUTS && outputs[index].range > 0 && outputs[index].digital_analog == 1) {
                *out_value = outputs[index].value;
                return true;
            }
            break;
        case 2: // Analog Value
            if (index < MAX_VARS && vars[index].range > 0 && vars[index].digital_analog == 1) {
                *out_value = vars[index].value;
                return true;
            }
            break;
        case 3: // Binary Input
            if (index < MAX_INS && inputs[index].range > 0 && inputs[index].digital_analog == 0) {
                *out_value = inputs[index].control;
                return true;
            }
            break;
        case 4: // Binary Output
            if (index < MAX_OUTS && outputs[index].range > 0 && outputs[index].digital_analog == 0) {
                *out_value = outputs[index].control;
                return true;
            }
            break;
        case 5: // Binary Value
            if (index < MAX_VARS && vars[index].range > 0 && vars[index].digital_analog == 0) {
                *out_value = vars[index].control;
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

static bool mqtt_check_point_change(uint8_t object_type, uint16_t instance, int32_t current_value, int32_t last_value)
{
    if (object_type == 0 || object_type == 1 || object_type == 2) {
        // Analog point
        int32_t diff = abs(current_value - last_value);
        int32_t err = 5000; // default 5.0

        if (object_type == 0) { // Analog Input special range checks
            int index = instance - 1;
            if (index >= 0 && index < MAX_INS) {
                if (inputs[index].range <= 49 || inputs[index].range == 57) {
                    err = 500; // 0.5
                } else if (inputs[index].range == 58) {
                    err = 10000; // 10.0
                }
            }
        }
        return (diff > err);
    } else {
        // Binary point
        return (current_value != last_value);
    }
}

static void mqtt_send_standalone_cov(uint8_t object_type, uint16_t instance, int32_t raw_value, uint32_t lifetime)
{
    BACNET_COV_DATA mock_cov;
    BACNET_PROPERTY_VALUE mock_value;

    extern uint32_t Instance;
    mock_cov.subscriberProcessIdentifier = 1;
    mock_cov.initiatingDeviceIdentifier = Instance;
    mock_cov.monitoredObjectIdentifier.type = object_type;
    mock_cov.monitoredObjectIdentifier.instance = instance - 1; // 0-based index
    mock_cov.timeRemaining = lifetime;
    mock_cov.listOfValues = &mock_value;

    mock_value.propertyIdentifier = PROP_PRESENT_VALUE;
    mock_value.propertyArrayIndex = BACNET_ARRAY_ALL;
    mock_value.priority = BACNET_NO_PRIORITY;
    mock_value.next = NULL;

    if (object_type == 0 || object_type == 1 || object_type == 2) {
        // Analog: Real value
        mock_value.value.tag = BACNET_APPLICATION_TAG_REAL;
        mock_value.value.type.Real = (float)raw_value / 1000.0f;
    } else {
        // Binary: Boolean value
        mock_value.value.tag = BACNET_APPLICATION_TAG_BOOLEAN;
        mock_value.value.type.Boolean = (raw_value == 1);
    }

    Mqtt_Handler_Send_COV(&mock_cov);
}

static void mqtt_register_standalone_subscription(uint8_t object_type, uint16_t instance, uint32_t lifetime, bool is_unsubscribe)
{
    if (is_unsubscribe || lifetime == 0) {
        // Find active subscription and deactivate it
        for (int i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++) {
            if (s_mqtt_subscriptions[i].is_active &&
                s_mqtt_subscriptions[i].object_type == object_type &&
                s_mqtt_subscriptions[i].instance == instance) {
                s_mqtt_subscriptions[i].is_active = false;
                MQTT_DBG("Unsubscribed object_type=%d, instance=%d standalone", object_type, instance);
                mqtt_save_subscriptions_to_flash();
                return;
            }
        }
        MQTT_DBG("Unsubscribe failed: Subscription not found for object_type=%d, instance=%d", object_type, instance);
        return;
    }

    // Check if it already exists
    for (int i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++) {
        if (s_mqtt_subscriptions[i].is_active &&
            s_mqtt_subscriptions[i].object_type == object_type &&
            s_mqtt_subscriptions[i].instance == instance) {
            s_mqtt_subscriptions[i].lifetime = lifetime;
            s_mqtt_subscriptions[i].persist = (lifetime > MQTT_FLASH_PERSIST_MIN_LIFETIME);
            MQTT_DBG("Updated subscription: object_type=%d, instance=%d standalone, lifetime=%lu, persist=%d", 
                     object_type, instance, lifetime, s_mqtt_subscriptions[i].persist);
            
            // Send initial value immediately
            int32_t current_val = 0;
            if (mqtt_get_point_value(object_type, instance, &current_val)) {
                s_mqtt_subscriptions[i].last_value = current_val;
                mqtt_send_standalone_cov(object_type, instance, current_val, lifetime);
            }
            mqtt_save_subscriptions_to_flash();
            return;
        }
    }

    // Find a free slot
    for (int i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++) {
        if (!s_mqtt_subscriptions[i].is_active) {
            s_mqtt_subscriptions[i].object_type = object_type;
            s_mqtt_subscriptions[i].instance = instance;
            s_mqtt_subscriptions[i].lifetime = lifetime;
            s_mqtt_subscriptions[i].persist = (lifetime > MQTT_FLASH_PERSIST_MIN_LIFETIME);
            s_mqtt_subscriptions[i].is_active = true;

            int32_t current_val = 0;
            if (mqtt_get_point_value(object_type, instance, &current_val)) {
                s_mqtt_subscriptions[i].last_value = current_val;
                MQTT_DBG("Subscribed: object_type=%d, instance=%d standalone, lifetime=%lu, persist=%d, initial_val=%ld",
                         object_type, instance, lifetime, s_mqtt_subscriptions[i].persist, current_val);
                mqtt_send_standalone_cov(object_type, instance, current_val, lifetime);
            } else {
                s_mqtt_subscriptions[i].last_value = 0;
                MQTT_DBG("Subscribed (inactive/invalid point): object_type=%d, instance=%d standalone, lifetime=%lu, persist=%d",
                         object_type, instance, lifetime, s_mqtt_subscriptions[i].persist);
            }
            mqtt_save_subscriptions_to_flash();
            return;
        }
    }

    ESP_LOGW(TAG, "Failed to subscribe object_type=%d, instance=%d: subscription table full", object_type, instance);
}

static void mqtt_update_standalone_subscriptions(uint32_t seconds)
{
    bool needs_save = false;
    for (int i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++) {
        if (!s_mqtt_subscriptions[i].is_active) {
            continue;
        }

        // Decrement lifetime
        if (s_mqtt_subscriptions[i].lifetime > seconds) {
            s_mqtt_subscriptions[i].lifetime -= seconds;
        } else {
            s_mqtt_subscriptions[i].is_active = false;
            MQTT_DBG("Subscription expired: object_type=%d, instance=%d",
                     s_mqtt_subscriptions[i].object_type, s_mqtt_subscriptions[i].instance);
            needs_save = true;
            continue;
        }

        // Check for value change
        int32_t current_val = 0;
        if (mqtt_get_point_value(s_mqtt_subscriptions[i].object_type, s_mqtt_subscriptions[i].instance, &current_val)) {
            if (mqtt_check_point_change(s_mqtt_subscriptions[i].object_type, s_mqtt_subscriptions[i].instance,
                                        current_val, s_mqtt_subscriptions[i].last_value)) {
                MQTT_DBG("COV detected: object_type=%d, instance=%d, old=%ld, new=%ld",
                         s_mqtt_subscriptions[i].object_type, s_mqtt_subscriptions[i].instance,
                         s_mqtt_subscriptions[i].last_value, current_val);
                s_mqtt_subscriptions[i].last_value = current_val;
                mqtt_send_standalone_cov(s_mqtt_subscriptions[i].object_type, s_mqtt_subscriptions[i].instance,
                                         current_val, s_mqtt_subscriptions[i].lifetime);
            }
        }
    }

    if (needs_save) {
        mqtt_save_subscriptions_to_flash();
    }
}

static void mqtt_publish_active_subscriptions(void)
{
    if (!s_connected) return;

    for (int i = 0; i < MAX_MQTT_SUBSCRIPTIONS; i++) {
        if (s_mqtt_subscriptions[i].is_active) {
            int32_t current_val = 0;
            if (mqtt_get_point_value(s_mqtt_subscriptions[i].object_type, s_mqtt_subscriptions[i].instance, &current_val)) {
                s_mqtt_subscriptions[i].last_value = current_val;
                MQTT_DBG("Publishing active subscription after reconnect: type=%d, instance=%d, val=%ld",
                         s_mqtt_subscriptions[i].object_type, s_mqtt_subscriptions[i].instance, current_val);
                mqtt_send_standalone_cov(s_mqtt_subscriptions[i].object_type, s_mqtt_subscriptions[i].instance,
                                         current_val, s_mqtt_subscriptions[i].lifetime);
            }
        }
    }
}

/**
 * @brief Callback handler for MQTT events dispatched by esp-mqtt.
 *
 * Handles connection, disconnection, message receipt, and logging of errors.
 *
 * @param[in] handler_args  User arguments registered with the handler (NULL).
 * @param[in] base          Event base (always MQTT_EVENTS).
 * @param[in] event_id      The specific MQTT event ID.
 * @param[in] event_data    Data payload associated with the event.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED to HiveMQ broker");
            s_connected = true;

            // Subscribe to the test subscription topic
            msg_id = esp_mqtt_client_subscribe(client, "temco/test/tstat11/sub", 1);
            ESP_LOGI(TAG, "Sent subscribe to test topic successful, msg_id=%d", msg_id);

            // Subscribe to the COV subscription topic
            msg_id = esp_mqtt_client_subscribe(client, "temco/cov/tstat11/sub", 1);
            ESP_LOGI(TAG, "Sent subscribe to COV topic successful, msg_id=%d", msg_id);

            // Publish a test confirmation message
            const char *conn_payload = "{\"status\":\"online\",\"device\":\"tstat11\",\"broker\":\"HiveMQ\"}";
            msg_id = esp_mqtt_client_publish(client, "temco/test/tstat11/pub", conn_payload, 0, 1, 0);
            ESP_LOGI(TAG, "Sent publish successful, msg_id=%d", msg_id);
            mqtt_publish_active_subscriptions();
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            s_connected = false;
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            {
                char *temp_topic = malloc(event->topic_len + 1);
                char *temp_data = malloc(event->data_len + 1);
                if (temp_topic && temp_data) {
                    memcpy(temp_topic, event->topic, event->topic_len);
                    temp_topic[event->topic_len] = '\0';
                    memcpy(temp_data, event->data, event->data_len);
                    temp_data[event->data_len] = '\0';

                    MQTT_DBG("Received topic: %s, payload: %s", temp_topic, temp_data);

                    if (strcmp(temp_topic, "temco/test/tstat11/sub") == 0 ||
                        strcmp(temp_topic, "temco/cov/tstat11/sub") == 0) {
                        
                        cJSON *root = cJSON_Parse(temp_data);
                        if (root) {
                            cJSON *action_item = cJSON_GetObjectItem(root, "action");
                            cJSON *type_item = cJSON_GetObjectItem(root, "object_type");
                            cJSON *instance_item = cJSON_GetObjectItem(root, "instance");
                            cJSON *lifetime_item = cJSON_GetObjectItem(root, "lifetime");

                            if (action_item && action_item->valuestring && type_item && instance_item) {
                                int obj_type = -1;
                                if (cJSON_IsNumber(type_item)) {
                                    obj_type = type_item->valueint;
                                } else if (cJSON_IsString(type_item)) {
                                    if (strcasecmp(type_item->valuestring, "ANALOG_INPUT") == 0) obj_type = 0;
                                    else if (strcasecmp(type_item->valuestring, "ANALOG_OUTPUT") == 0) obj_type = 1;
                                    else if (strcasecmp(type_item->valuestring, "ANALOG_VALUE") == 0) obj_type = 2;
                                    else if (strcasecmp(type_item->valuestring, "BINARY_INPUT") == 0) obj_type = 3;
                                    else if (strcasecmp(type_item->valuestring, "BINARY_OUTPUT") == 0) obj_type = 4;
                                    else if (strcasecmp(type_item->valuestring, "BINARY_VALUE") == 0) obj_type = 5;
                                }

                                int instance = instance_item->valueint;
                                int lifetime = lifetime_item ? lifetime_item->valueint : 300;
                                bool is_unsubscribe = (strcmp(action_item->valuestring, "unsubscribe") == 0);

                                if (obj_type >= 0 && instance > 0) {
                                     mqtt_register_standalone_subscription(obj_type, instance, lifetime, is_unsubscribe);
                                 } else {
                                    MQTT_DBG("Subscription failed: Invalid object type %d or instance %d", obj_type, instance);
                                }
                            } else {
                                MQTT_DBG("Payload validation failed (missing action, object_type, or instance)");
                            }
                            cJSON_Delete(root);
                        } else {
                            MQTT_DBG("JSON parsing failed");
                        }
                    }
                } else {
                    MQTT_DBG("Failed to allocate memory for topic or data copy");
                }
                if (temp_topic) free(temp_topic);
                if (temp_data) free(temp_data);
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "TCP/Transport Error");
                ESP_LOGE(TAG, "Last error code from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGE(TAG, "Last TLS stack error: 0x%x", event->error_handle->esp_tls_stack_err);
                ESP_LOGE(TAG, "Last socket errno: %d", event->error_handle->esp_transport_sock_errno);
            }
            break;

        default:
            ESP_LOGD(TAG, "Other MQTT event ID: %d", event->event_id);
            break;
    }
}

int Mqtt_Handler_Publish(const char *topic, const char *data, int qos, bool retain)
{
    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return -1;
    }
    if (!s_connected) {
        ESP_LOGW(TAG, "MQTT client not connected, publish may fail");
    }
    return esp_mqtt_client_publish(s_mqtt_client, topic, data, 0, qos, retain ? 1 : 0);
}

bool Mqtt_Handler_Send_COV(const struct BACnet_COV_Data *cov_data)
{
    if (s_mqtt_client == NULL || !s_connected) {
        ESP_LOGE(TAG, "MQTT not connected, cannot send COV data");
        return false;
    }

    if (cov_data == NULL) {
        ESP_LOGE(TAG, "COV data is NULL");
        return false;
    }

    extern uint32_t Instance;
    uint32_t display_instance = cov_data->monitoredObjectIdentifier.instance;
    if (cov_data->initiatingDeviceIdentifier == Instance) {
        display_instance = cov_data->monitoredObjectIdentifier.instance + 1; // Map back to 1-based instance for local COVs
    }

    // Start JSON serialization
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to allocate cJSON root object");
        return false;
    }

    cJSON_AddNumberToObject(root, "subscriber_process_id", cov_data->subscriberProcessIdentifier);
    cJSON_AddNumberToObject(root, "initiating_device_id", cov_data->initiatingDeviceIdentifier);
    cJSON_AddNumberToObject(root, "time_remaining", cov_data->timeRemaining);

    // Monitored Object ID
    cJSON *obj_id = cJSON_CreateObject();
    if (obj_id) {
        cJSON_AddNumberToObject(obj_id, "type", cov_data->monitoredObjectIdentifier.type);
        cJSON_AddStringToObject(obj_id, "type_name", bactext_object_type_name(cov_data->monitoredObjectIdentifier.type));
        cJSON_AddNumberToObject(obj_id, "instance", display_instance);
        cJSON_AddItemToObject(root, "monitored_object", obj_id);
    }

    // List of Values
    cJSON *values_array = cJSON_CreateArray();
    if (values_array) {
        const BACNET_PROPERTY_VALUE *prop_val = cov_data->listOfValues;
        while (prop_val != NULL) {
            cJSON *val_obj = cJSON_CreateObject();
            if (val_obj) {
                cJSON_AddNumberToObject(val_obj, "property_id", prop_val->propertyIdentifier);
                cJSON_AddStringToObject(val_obj, "property_name", bactext_property_name(prop_val->propertyIdentifier));
                cJSON_AddNumberToObject(val_obj, "property_array_index", prop_val->propertyArrayIndex);
                cJSON_AddNumberToObject(val_obj, "priority", prop_val->priority);

                // Add value representation
                const BACNET_APPLICATION_DATA_VALUE *val = &prop_val->value;
                cJSON_AddNumberToObject(val_obj, "tag", val->tag);
                switch (val->tag) {
                    case BACNET_APPLICATION_TAG_NULL:
                        cJSON_AddNullToObject(val_obj, "value");
                        break;
                    case BACNET_APPLICATION_TAG_BOOLEAN:
                        cJSON_AddBoolToObject(val_obj, "value", val->type.Boolean);
                        break;
                    case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                        cJSON_AddNumberToObject(val_obj, "value", val->type.Unsigned_Int);
                        break;
                    case BACNET_APPLICATION_TAG_SIGNED_INT:
                        cJSON_AddNumberToObject(val_obj, "value", val->type.Signed_Int);
                        break;
                    case BACNET_APPLICATION_TAG_REAL:
                        cJSON_AddNumberToObject(val_obj, "value", val->type.Real);
                        break;
                    case BACNET_APPLICATION_TAG_DOUBLE:
                        cJSON_AddNumberToObject(val_obj, "value", val->type.Double);
                        break;
                    case BACNET_APPLICATION_TAG_ENUMERATED:
                        cJSON_AddNumberToObject(val_obj, "value", val->type.Enumerated);
                        break;
                    case BACNET_APPLICATION_TAG_CHARACTER_STRING: {
                        char temp_str[128];
                        int copy_len = val->type.Character_String.length < (sizeof(temp_str) - 1) ?
                                       val->type.Character_String.length : (sizeof(temp_str) - 1);
                        memcpy(temp_str, val->type.Character_String.value, copy_len);
                        temp_str[copy_len] = '\0';
                        cJSON_AddStringToObject(val_obj, "value", temp_str);
                        break;
                    }
                    case BACNET_APPLICATION_TAG_OBJECT_ID: {
                        cJSON *sub_obj_id = cJSON_CreateObject();
                        if (sub_obj_id) {
                            cJSON_AddNumberToObject(sub_obj_id, "type", val->type.Object_Id.type);
                            cJSON_AddNumberToObject(sub_obj_id, "instance", val->type.Object_Id.instance);
                            cJSON_AddItemToObject(val_obj, "value", sub_obj_id);
                        }
                        break;
                    }
                    default:
                        cJSON_AddStringToObject(val_obj, "value", "(unsupported_tag)");
                        break;
                }

                cJSON_AddItemToArray(values_array, val_obj);
            }
            prop_val = prop_val->next;
        }
        cJSON_AddItemToObject(root, "values", values_array);
    }

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_str == NULL) {
        ESP_LOGE(TAG, "Failed to format COV JSON string");
        return false;
    }

    // Publish to the COV topic
    char topic[128];
    snprintf(topic, sizeof(topic), "temco/cov/tstat11/device_%lu/%s_%lu",
             cov_data->initiatingDeviceIdentifier,
             bactext_object_type_name(cov_data->monitoredObjectIdentifier.type),
             display_instance);

    ESP_LOGI(TAG, "Publishing COV to topic: %s", topic);
    int msg_id = Mqtt_Handler_Publish(topic, json_str, 1, false);
    free(json_str);

    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish COV MQTT message");
        return false;
    }

    ESP_LOGI(TAG, "COV MQTT message queued successfully, msg_id=%d", msg_id);
    return true;
}

// mqtt_check_all_cov is removed to use minimum variables/RAM.

void Mqtt_HandlerTask(void *pvParameters)
{
    /* Wait for wifi to be connected before initializing WireGuard */
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if (xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT)
        {
            ESP_LOGI(TAG, "WiFi is connected, proceeding with Mqtt setup.");
            break;
        }
    }

    ESP_LOGI(TAG, "Initializing MQTT Client connection...");

    // ESP-IDF v5 style configuration
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = "mqtt://broker.hivemq.com:1883",
        },
    };

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return;
    }

    // Register event handler with default loop
    esp_err_t err = esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register MQTT event handler: %s", esp_err_to_name(err));
        return;
    }

    // Start the MQTT client
    err = esp_mqtt_client_start(s_mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "MQTT client task started. Connecting to HiveMQ...");
    }

    mqtt_load_subscriptions_from_flash();

    while(mqtt_task_exit == false)
    {
        mqtt_update_standalone_subscriptions(1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    if (s_mqtt_client)
    {
        esp_mqtt_client_unregister_event(
            s_mqtt_client,
            ESP_EVENT_ANY_ID,
            mqtt_event_handler);

        esp_mqtt_client_stop(s_mqtt_client);
        esp_mqtt_client_destroy(s_mqtt_client);

        s_mqtt_client = NULL;
    }

    s_connected = false;
    main_task_handle[19] = NULL;

    ESP_LOGI(TAG, "MQTT task exiting");

    vTaskDelete(NULL);
}

void Mqtt_Deinit(void)
{
    mqtt_task_exit = true;
}

void Mqtt_Handler_Init(void)
{
    if(Modbus.enable_mqtt)
    {
        if (main_task_handle[19] != NULL)
        {
            ESP_LOGW(TAG, "MQTT task already running");
            return;
        }

        mqtt_task_exit = false;

        xTaskCreate(Mqtt_HandlerTask, "mqtt_handler", 4096, NULL, tskIDLE_PRIORITY + 2, &main_task_handle[19]);
    }
}
