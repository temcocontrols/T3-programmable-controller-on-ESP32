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
            ESP_LOGI(TAG, "Sent subscribe successful, msg_id=%d", msg_id);

            // Publish a test confirmation message
            const char *conn_payload = "{\"status\":\"online\",\"device\":\"tstat11\",\"broker\":\"HiveMQ\"}";
            msg_id = esp_mqtt_client_publish(client, "temco/test/tstat11/pub", conn_payload, 0, 1, 0);
            ESP_LOGI(TAG, "Sent publish successful, msg_id=%d", msg_id);
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
            ESP_LOGI(TAG, "MQTT_EVENT_DATA received");
            ESP_LOGI(TAG, "Topic: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Payload: %.*s", event->data_len, event->data);
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
        cJSON_AddNumberToObject(obj_id, "instance", cov_data->monitoredObjectIdentifier.instance);
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
             cov_data->monitoredObjectIdentifier.instance);

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

#if ALL_COV
static int32_t backup_mqtt_inputs[MAX_INS];
static int32_t backup_mqtt_outputs[MAX_OUTS];
static int32_t backup_mqtt_vars[MAX_VARS + 12];

static void mqtt_check_all_cov(void)
{
    static bool initialized = false;

    // Check Inputs
    for (int i = 0; i < MAX_INS; i++) {
        if (inputs[i].range == 0) continue;

        bool changed = false;
        static BACNET_COV_DATA mock_cov;
        static BACNET_PROPERTY_VALUE mock_value;
        static char text[16];

        if (inputs[i].digital_analog == 1) { // Analog Input
            int32_t diff = abs(inputs[i].value - backup_mqtt_inputs[i]);
            int32_t err = 5000;
            if (inputs[i].range <= 49 || inputs[i].range == 57) {
                err = 500;
            } else if (inputs[i].range == 58) {
                err = 10000;
            }
            if (diff > err || !initialized) {
                backup_mqtt_inputs[i] = inputs[i].value;
                changed = true;
                mock_value.propertyIdentifier = PROP_PRESENT_VALUE;
                mock_value.propertyArrayIndex = BACNET_ARRAY_ALL;
                mock_value.priority = BACNET_NO_PRIORITY;
                mock_value.next = NULL;
                snprintf(text, sizeof(text), "%f", (float)inputs[i].value / 1000);
                bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL, text, &mock_value.value);
            }
        } else { // Binary Input
            if (inputs[i].control != backup_mqtt_inputs[i] || !initialized) {
                backup_mqtt_inputs[i] = inputs[i].control;
                changed = true;
                mock_value.propertyIdentifier = PROP_PRESENT_VALUE;
                mock_value.propertyArrayIndex = BACNET_ARRAY_ALL;
                mock_value.priority = BACNET_NO_PRIORITY;
                mock_value.next = NULL;
                if (inputs[i].control == 1) {
                    bacapp_parse_application_data(BACNET_APPLICATION_TAG_BOOLEAN, "1", &mock_value.value);
                } else {
                    bacapp_parse_application_data(BACNET_APPLICATION_TAG_BOOLEAN, "0", &mock_value.value);
                }
            }
        }

        if (changed) {
            extern uint32_t Instance;
            mock_cov.subscriberProcessIdentifier = 1;
            mock_cov.initiatingDeviceIdentifier = Instance;
            mock_cov.monitoredObjectIdentifier.type = (inputs[i].digital_analog == 1) ? OBJECT_ANALOG_INPUT : OBJECT_BINARY_INPUT;
            mock_cov.monitoredObjectIdentifier.instance = i + 1;
            mock_cov.timeRemaining = 60;
            mock_cov.listOfValues = &mock_value;
            Mqtt_Handler_Send_COV(&mock_cov);
        }
    }

    // Check Outputs
    for (int i = 0; i < MAX_OUTS; i++) {
        if (outputs[i].range == 0) continue;

        bool changed = false;
        static BACNET_COV_DATA mock_cov;
        static BACNET_PROPERTY_VALUE mock_value;
        static char text[16];

        if (outputs[i].digital_analog == 1) { // Analog Output
            int32_t diff = abs(outputs[i].value - backup_mqtt_outputs[i]);
            int32_t err = 5000;
            if (diff > err || !initialized) {
                backup_mqtt_outputs[i] = outputs[i].value;
                changed = true;
                mock_value.propertyIdentifier = PROP_PRESENT_VALUE;
                mock_value.propertyArrayIndex = BACNET_ARRAY_ALL;
                mock_value.priority = BACNET_NO_PRIORITY;
                mock_value.next = NULL;
                snprintf(text, sizeof(text), "%f", (float)outputs[i].value / 1000);
                bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL, text, &mock_value.value);
            }
        } else { // Binary Output
            if (outputs[i].control != backup_mqtt_outputs[i] || !initialized) {
                backup_mqtt_outputs[i] = outputs[i].control;
                changed = true;
                mock_value.propertyIdentifier = PROP_PRESENT_VALUE;
                mock_value.propertyArrayIndex = BACNET_ARRAY_ALL;
                mock_value.priority = BACNET_NO_PRIORITY;
                mock_value.next = NULL;
                if (outputs[i].control == 1) {
                    bacapp_parse_application_data(BACNET_APPLICATION_TAG_BOOLEAN, "1", &mock_value.value);
                } else {
                    bacapp_parse_application_data(BACNET_APPLICATION_TAG_BOOLEAN, "0", &mock_value.value);
                }
            }
        }

        if (changed) {
            extern uint32_t Instance;
            mock_cov.subscriberProcessIdentifier = 1;
            mock_cov.initiatingDeviceIdentifier = Instance;
            mock_cov.monitoredObjectIdentifier.type = (outputs[i].digital_analog == 1) ? OBJECT_ANALOG_OUTPUT : OBJECT_BINARY_OUTPUT;
            mock_cov.monitoredObjectIdentifier.instance = i + 1;
            mock_cov.timeRemaining = 60;
            mock_cov.listOfValues = &mock_value;
            Mqtt_Handler_Send_COV(&mock_cov);
        }
    }

    // Check Variables
    for (int i = 0; i < MAX_VARS; i++) {
        if (vars[i].range == 0) continue;

        bool changed = false;
        static BACNET_COV_DATA mock_cov;
        static BACNET_PROPERTY_VALUE mock_value;
        static char text[16];

        if (vars[i].digital_analog == 1) { // Analog Value
            int32_t diff = abs(vars[i].value - backup_mqtt_vars[i]);
            int32_t err = 5000;
            if (diff > err || !initialized) {
                backup_mqtt_vars[i] = vars[i].value;
                changed = true;
                mock_value.propertyIdentifier = PROP_PRESENT_VALUE;
                mock_value.propertyArrayIndex = BACNET_ARRAY_ALL;
                mock_value.priority = BACNET_NO_PRIORITY;
                mock_value.next = NULL;
                snprintf(text, sizeof(text), "%f", (float)vars[i].value / 1000);
                bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL, text, &mock_value.value);
            }
        } else { // Binary Value
            if (vars[i].control != backup_mqtt_vars[i] || !initialized) {
                backup_mqtt_vars[i] = vars[i].control;
                changed = true;
                mock_value.propertyIdentifier = PROP_PRESENT_VALUE;
                mock_value.propertyArrayIndex = BACNET_ARRAY_ALL;
                mock_value.priority = BACNET_NO_PRIORITY;
                mock_value.next = NULL;
                if (vars[i].control == 1) {
                    bacapp_parse_application_data(BACNET_APPLICATION_TAG_BOOLEAN, "1", &mock_value.value);
                } else {
                    bacapp_parse_application_data(BACNET_APPLICATION_TAG_BOOLEAN, "0", &mock_value.value);
                }
            }
        }

        if (changed) {
            extern uint32_t Instance;
            mock_cov.subscriberProcessIdentifier = 1;
            mock_cov.initiatingDeviceIdentifier = Instance;
            mock_cov.monitoredObjectIdentifier.type = (vars[i].digital_analog == 1) ? OBJECT_ANALOG_VALUE : OBJECT_BINARY_VALUE;
            mock_cov.monitoredObjectIdentifier.instance = i + 1;
            mock_cov.timeRemaining = 60;
            mock_cov.listOfValues = &mock_value;
            Mqtt_Handler_Send_COV(&mock_cov);
        }
    }

    initialized = true;
}
#endif

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

    while(mqtt_task_exit == false)
    {
#if ALL_COV
        mqtt_check_all_cov();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
#else
        vTaskDelay(10000 / portTICK_PERIOD_MS);
#endif
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
