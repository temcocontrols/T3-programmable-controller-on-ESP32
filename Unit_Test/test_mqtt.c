#include "unity.h"
#include "cJSON.h"
#include "bacnet.h"
#include "cov.h"
#include "bactext.h"
#include <string.h>
#include <stdbool.h>
#include "mqtt_client.h"

// Mock MQTT connection variables defined in main/Mqtt_Handler/Mqtt_Handler.c
static char last_published_topic[128] = {0};
static char last_published_data[1024] = {0};
static int publish_called_count = 0;

// Mock implementation of esp_mqtt_client_publish to intercept MQTT messages
int esp_mqtt_client_publish(esp_mqtt_client_handle_t client, const char* topic, const char* data, int len, int qos, int retain)
{
    (void)client; (void)len; (void)qos; (void)retain;
    publish_called_count++;
    snprintf(last_published_topic, sizeof(last_published_topic), "%s", topic);
    snprintf(last_published_data, sizeof(last_published_data), "%s", data);
    return 1; // Return dummy message ID
}

// Include Mqtt_Handler.c source to test its real logic and access static connection variables
#include "../main/Mqtt_Handler/Mqtt_Handler.c"

extern bool Mqtt_Handler_Send_COV(const struct BACnet_COV_Data *cov_data);

static void reset_mqtt_test_state(void) {
    s_mqtt_client = (void*)1;
    s_connected = true;
    publish_called_count = 0;
    memset(last_published_topic, 0, sizeof(last_published_topic));
    memset(last_published_data, 0, sizeof(last_published_data));
}

void test_mqtt_send_cov_real_value(void) {
    reset_mqtt_test_state();
    BACNET_COV_DATA cov = {0};
    cov.subscriberProcessIdentifier = 1;
    cov.initiatingDeviceIdentifier = 1028;
    cov.monitoredObjectIdentifier.type = 0; // ANALOG_INPUT
    cov.monitoredObjectIdentifier.instance = 2;
    cov.timeRemaining = 120;

    BACNET_PROPERTY_VALUE prop_val = {0};
    prop_val.value.tag = 4; // BACNET_APPLICATION_TAG_REAL
    prop_val.value.type.Real = 23.5f;
    cov.listOfValues = &prop_val;

    bool success = Mqtt_Handler_Send_COV(&cov);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_INT(1, publish_called_count);
    TEST_ASSERT_EQUAL_STRING("temco/cov/tstat11/device_1028/Analog Input_2", last_published_topic);

    // Verify JSON payload
    cJSON *root = cJSON_Parse(last_published_data);
    TEST_ASSERT_NOT_NULL(root);
    
    cJSON *sub_id = cJSON_GetObjectItem(root, "subscriber_process_id");
    TEST_ASSERT_NOT_NULL(sub_id);
    TEST_ASSERT_EQUAL_INT(1, sub_id->valueint);

    cJSON *device_id = cJSON_GetObjectItem(root, "initiating_device_id");
    TEST_ASSERT_NOT_NULL(device_id);
    TEST_ASSERT_EQUAL_INT(1028, device_id->valueint);

    cJSON *monitored_obj = cJSON_GetObjectItem(root, "monitored_object");
    TEST_ASSERT_NOT_NULL(monitored_obj);
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetObjectItem(monitored_obj, "type")->valueint);
    TEST_ASSERT_EQUAL_STRING("Analog Input", cJSON_GetObjectItem(monitored_obj, "type_name")->valuestring);
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetObjectItem(monitored_obj, "instance")->valueint);

    cJSON *values = cJSON_GetObjectItem(root, "values");
    TEST_ASSERT_NOT_NULL(values);
    TEST_ASSERT_TRUE(cJSON_IsArray(values));
    cJSON *val_item = cJSON_GetArrayItem(values, 0);
    TEST_ASSERT_NOT_NULL(val_item);
    TEST_ASSERT_EQUAL_FLOAT(23.5f, (float)cJSON_GetObjectItem(val_item, "value")->valuedouble);

    cJSON_Delete(root);
}

void test_mqtt_send_cov_boolean_value(void) {
    reset_mqtt_test_state();
    BACNET_COV_DATA cov = {0};
    cov.subscriberProcessIdentifier = 1;
    cov.initiatingDeviceIdentifier = 1028;
    cov.monitoredObjectIdentifier.type = 3; // BINARY_INPUT
    cov.monitoredObjectIdentifier.instance = 1;
    cov.timeRemaining = 300;

    BACNET_PROPERTY_VALUE prop_val = {0};
    prop_val.value.tag = 1; // BACNET_APPLICATION_TAG_BOOLEAN
    prop_val.value.type.Boolean = true;
    cov.listOfValues = &prop_val;

    bool success = Mqtt_Handler_Send_COV(&cov);
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_INT(1, publish_called_count);
    TEST_ASSERT_EQUAL_STRING("temco/cov/tstat11/device_1028/Binary Input_1", last_published_topic);

    // Verify JSON payload
    cJSON *root = cJSON_Parse(last_published_data);
    TEST_ASSERT_NOT_NULL(root);

    cJSON *values = cJSON_GetObjectItem(root, "values");
    TEST_ASSERT_NOT_NULL(values);
    cJSON *val_item = cJSON_GetArrayItem(values, 0);
    TEST_ASSERT_NOT_NULL(val_item);
    TEST_ASSERT_TRUE(cJSON_IsTrue(cJSON_GetObjectItem(val_item, "value")));

    cJSON_Delete(root);
}
