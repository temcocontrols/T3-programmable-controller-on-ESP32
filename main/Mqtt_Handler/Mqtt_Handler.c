/**
 * @file Mqtt_Handler.c
 * @brief MQTT client — configurable broker/topics; WAN = WiFi or GSM AUTO.
 */

#include "Mqtt_Handler.h"
#include "mqtt_client.h"
#include "user_data.h"
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "cJSON.h"
#include "trendlog.h"
#include "cov.h"
#include "bactext.h"
#include "define.h"
#include "handlers.h"
#include "modbus.h"
#include "hub_network_manager.h"

extern EventGroupHandle_t s_wifi_event_group;
extern TaskHandle_t main_task_handle[20];
static volatile bool mqtt_task_exit = false;

#define WIFI_CONNECTED_BIT BIT0

extern TL_DATA_REC Logs[MAX_TREND_LOGS][TL_MAX_ENTRIES];
extern uint8_t TRENDLOGS;

static const char *TAG = "MQTT_HANDLER";
static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_connected = false;
static char s_broker_uri[128];

extern int Send_UCOV_Notify(uint8_t * buffer, BACNET_COV_DATA * cov_data, uint8_t protocal);
extern void udp_client_send(uint16_t time);
#define BAC_IP_CLIENT 2

#define MQTT_DEBUG_EN 1
#if MQTT_DEBUG_EN
#define MQTT_DBG(fmt, ...) ESP_LOGI(TAG, "[DEBUG] " fmt, ##__VA_ARGS__)
#else
#define MQTT_DBG(fmt, ...)
#endif

static bool mqtt_wifi_ready(void)
{
	return (s_wifi_event_group != NULL) &&
	       ((xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT) != 0);
}

static bool mqtt_gsm_ready(void)
{
	hub_network_manager_status_t status;

	if (hub_network_manager_get_status(&status) != ESP_OK) {
		return false;
	}
	return status.lte_connected && (status.lte_ip_addr[0] != '\0');
}

static bool mqtt_auto_underlay_ready(const char **underlay_name)
{
	if (mqtt_wifi_ready()) {
		if (underlay_name) {
			*underlay_name = "wifi";
		}
		return true;
	}
	if (mqtt_gsm_ready()) {
		if (underlay_name) {
			*underlay_name = "gsm";
		}
		return true;
	}
	return false;
}

static const char *mqtt_pub_topic(void)
{
	if (mqtt_point.reg.pub_topic[0] != '\0') {
		return mqtt_point.reg.pub_topic;
	}
	return "temco/test/hub/pub";
}

static const char *mqtt_sub_topic(void)
{
	if (mqtt_point.reg.sub_topic[0] != '\0') {
		return mqtt_point.reg.sub_topic;
	}
	return "temco/test/hub/sub";
}

static void mqtt_build_broker_uri(void)
{
	uint16_t port = mqtt_point.reg.port ? mqtt_point.reg.port : 1883;
	const char *host = mqtt_point.reg.broker[0] ? mqtt_point.reg.broker : "broker.hivemq.com";

	snprintf(s_broker_uri, sizeof(s_broker_uri), "mqtt://%s:%u", host, (unsigned)port);
}

static void mqtt_forward_subscription_to_bacnet(uint8_t object_type, uint32_t instance, uint32_t lifetime, bool is_unsubscribe)
{
	BACNET_SUBSCRIBE_COV_DATA cov_data = {0};
	cov_data.subscriberProcessIdentifier = 1;
	cov_data.monitoredObjectIdentifier.type = object_type;
	cov_data.monitoredObjectIdentifier.instance = instance;
	cov_data.issueConfirmedNotifications = false;
	cov_data.lifetime = lifetime;
	cov_data.cancellationRequest = is_unsubscribe;

	uint8_t apdu[64];
	int apdu_len = cov_subscribe_encode_apdu(apdu, 1, &cov_data);

	if (apdu_len > 4) {
		BACNET_ADDRESS src = {0};
		BACNET_CONFIRMED_SERVICE_DATA service_data = {0};
		service_data.invoke_id = 1;
		service_data.segmented_message = false;
		handler_cov_subscribe(apdu + 4, apdu_len - 4, &src, &service_data, BAC_IP_CLIENT);
	} else {
		ESP_LOGE(TAG, "Failed to encode BACnet subscribe APDU");
	}
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	esp_mqtt_event_handle_t event = event_data;
	esp_mqtt_client_handle_t client = event->client;
	int msg_id;
	(void)handler_args;
	(void)base;

	switch ((esp_mqtt_event_id_t)event_id) {
	case MQTT_EVENT_CONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED to %s", s_broker_uri);
		s_connected = true;
		mqtt_point.reg.connected = 1;

		msg_id = esp_mqtt_client_subscribe(client, mqtt_sub_topic(), 1);
		ESP_LOGI(TAG, "Subscribe %s msg_id=%d", mqtt_sub_topic(), msg_id);

		{
			char conn_payload[160];
			extern uint32_t Instance;
			snprintf(conn_payload, sizeof(conn_payload),
			         "{\"status\":\"online\",\"device\":\"hub\",\"device_id\":%lu}",
			         (unsigned long)Instance);
			msg_id = esp_mqtt_client_publish(client, mqtt_pub_topic(), conn_payload, 0, 1, 0);
			ESP_LOGI(TAG, "Online publish msg_id=%d", msg_id);
		}
		break;

	case MQTT_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
		s_connected = false;
		mqtt_point.reg.connected = 0;
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

			if (strcmp(temp_topic, mqtt_sub_topic()) == 0) {
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
							mqtt_forward_subscription_to_bacnet(obj_type, instance, lifetime, is_unsubscribe);
						}
					}
					cJSON_Delete(root);
				}
			}
		}
		if (temp_topic) free(temp_topic);
		if (temp_data) free(temp_data);
		break;
	}

	case MQTT_EVENT_ERROR:
		ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
		break;

	default:
		break;
	}
}

int Mqtt_Handler_Publish(const char *topic, const char *data, int qos, bool retain)
{
	if (s_mqtt_client == NULL) {
		return -1;
	}
	return esp_mqtt_client_publish(s_mqtt_client, topic, data, 0, qos, retain ? 1 : 0);
}

bool Mqtt_Handler_Send_COV(const struct BACnet_COV_Data *cov_data)
{
	if (s_mqtt_client == NULL || !s_connected || cov_data == NULL) {
		return false;
	}

	uint32_t display_instance = cov_data->monitoredObjectIdentifier.instance;
	cJSON *root = cJSON_CreateObject();
	if (root == NULL) {
		return false;
	}

	cJSON_AddNumberToObject(root, "subscriber_process_id", cov_data->subscriberProcessIdentifier);
	cJSON_AddNumberToObject(root, "initiating_device_id", cov_data->initiatingDeviceIdentifier);
	cJSON_AddNumberToObject(root, "time_remaining", cov_data->timeRemaining);

	cJSON *obj_id = cJSON_CreateObject();
	if (obj_id) {
		cJSON_AddNumberToObject(obj_id, "type", cov_data->monitoredObjectIdentifier.type);
		cJSON_AddStringToObject(obj_id, "type_name", bactext_object_type_name(cov_data->monitoredObjectIdentifier.type));
		cJSON_AddNumberToObject(obj_id, "instance", display_instance);
		cJSON_AddItemToObject(root, "monitored_object", obj_id);
	}

	cJSON *values_array = cJSON_CreateArray();
	if (values_array) {
		const BACNET_PROPERTY_VALUE *prop_val = cov_data->listOfValues;
		while (prop_val != NULL) {
			cJSON *val_obj = cJSON_CreateObject();
			if (val_obj) {
				const BACNET_APPLICATION_DATA_VALUE *val = &prop_val->value;
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
		return false;
	}

	char topic[160];
	snprintf(topic, sizeof(topic), "%s/cov/device_%lu/%s_%lu",
	         mqtt_pub_topic(),
	         (unsigned long)cov_data->initiatingDeviceIdentifier,
	         bactext_object_type_name(cov_data->monitoredObjectIdentifier.type),
	         (unsigned long)display_instance);

	int msg_id = Mqtt_Handler_Publish(topic, json_str, 1, false);
	free(json_str);
	return msg_id >= 0;
}

void Mqtt_HandlerTask(void *pvParameters)
{
	const char *underlay = NULL;
	(void)pvParameters;

	ESP_LOGI(TAG, "Waiting for WAN (WiFi or GSM)...");
	while (!mqtt_task_exit) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		if (mqtt_auto_underlay_ready(&underlay)) {
			ESP_LOGI(TAG, "AUTO underlay ready (%s), starting MQTT", underlay ? underlay : "?");
			break;
		}
	}
	if (mqtt_task_exit) {
		goto exit_task;
	}

	mqtt_build_broker_uri();
	ESP_LOGI(TAG, "Initializing MQTT client uri=%s", s_broker_uri);

	esp_mqtt_client_config_t mqtt_cfg = {0};
	mqtt_cfg.broker.address.uri = s_broker_uri;
	if (mqtt_point.reg.client_id[0] != '\0') {
		mqtt_cfg.credentials.client_id = mqtt_point.reg.client_id;
	}
	if (mqtt_point.reg.username[0] != '\0') {
		mqtt_cfg.credentials.username = mqtt_point.reg.username;
		mqtt_cfg.credentials.authentication.password = mqtt_point.reg.password;
	}

	s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
	if (s_mqtt_client == NULL) {
		ESP_LOGE(TAG, "Failed to initialize MQTT client");
		goto exit_task;
	}

	esp_err_t err = esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to register MQTT event handler: %s", esp_err_to_name(err));
		goto exit_task;
	}

	err = esp_mqtt_client_start(s_mqtt_client);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(err));
	}

	while (mqtt_task_exit == false) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	if (s_mqtt_client) {
		esp_mqtt_client_unregister_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler);
		esp_mqtt_client_stop(s_mqtt_client);
		esp_mqtt_client_destroy(s_mqtt_client);
		s_mqtt_client = NULL;
	}

exit_task:
	s_connected = false;
	mqtt_point.reg.connected = 0;
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
	if (mqtt_point.reg.enable == 0 && Modbus.enable_mqtt == 0) {
		return;
	}
	/* Prefer mqtt_point; keep Modbus.enable_mqtt in sync for legacy reg 24. */
	if (mqtt_point.reg.enable) {
		Modbus.enable_mqtt = 1;
	} else if (Modbus.enable_mqtt) {
		mqtt_point.reg.enable = 1;
	}

	if (main_task_handle[19] != NULL) {
		ESP_LOGW(TAG, "MQTT task already running");
		return;
	}

	mqtt_task_exit = false;
	xTaskCreate(Mqtt_HandlerTask, "mqtt_handler", 4096, NULL, tskIDLE_PRIORITY + 2, &main_task_handle[19]);
}

void Mqtt_Handler_Apply_Config(void)
{
	Modbus.enable_mqtt = mqtt_point.reg.enable ? 1 : 0;
	if (mqtt_point.reg.enable == 0) {
		Mqtt_Deinit();
		return;
	}
	if (main_task_handle[19] != NULL) {
		Mqtt_Deinit();
		/* Wait briefly for task exit then restart */
		for (int i = 0; i < 30 && main_task_handle[19] != NULL; i++) {
			vTaskDelay(100 / portTICK_PERIOD_MS);
		}
	}
	Mqtt_Handler_Init();
}

static uint16_t mqtt_pack_chars(const char *buf, size_t buf_size, uint8_t item)
{
	if ((item * 2u + 1u) < buf_size) {
		return ((uint16_t)(uint8_t)buf[item * 2u] << 8) | (uint8_t)buf[item * 2u + 1u];
	}
	return 0;
}

static void mqtt_unpack_chars(char *buf, size_t buf_size, uint8_t item, uint16_t value)
{
	if ((item * 2u + 1u) < buf_size) {
		buf[item * 2u] = (char)((value >> 8) & 0xff);
		buf[item * 2u + 1u] = (char)(value & 0xff);
	}
}

uint16_t mqtt_config_read_by_block(uint16_t addr)
{
	uint8_t item;

	mqtt_point.reg.connected = s_connected ? 1 : 0;

	if (addr == MODBUS_MQTT_ENABLE) {
		return mqtt_point.reg.enable;
	}
	if (addr == MODBUS_MQTT_CONNECTED) {
		return mqtt_point.reg.connected;
	}
	if (addr == MODBUS_MQTT_PORT) {
		return mqtt_point.reg.port;
	}
	if ((addr >= MODBUS_MQTT_BROKER_START) && (addr <= MODBUS_MQTT_BROKER_END)) {
		item = (uint8_t)(addr - MODBUS_MQTT_BROKER_START);
		return mqtt_pack_chars(mqtt_point.reg.broker, sizeof(mqtt_point.reg.broker), item);
	}
	if ((addr >= MODBUS_MQTT_USERNAME_START) && (addr <= MODBUS_MQTT_USERNAME_END)) {
		item = (uint8_t)(addr - MODBUS_MQTT_USERNAME_START);
		return mqtt_pack_chars(mqtt_point.reg.username, sizeof(mqtt_point.reg.username), item);
	}
	if ((addr >= MODBUS_MQTT_PASSWORD_START) && (addr <= MODBUS_MQTT_PASSWORD_END)) {
		item = (uint8_t)(addr - MODBUS_MQTT_PASSWORD_START);
		return mqtt_pack_chars(mqtt_point.reg.password, sizeof(mqtt_point.reg.password), item);
	}
	if ((addr >= MODBUS_MQTT_CLIENT_ID_START) && (addr <= MODBUS_MQTT_CLIENT_ID_END)) {
		item = (uint8_t)(addr - MODBUS_MQTT_CLIENT_ID_START);
		return mqtt_pack_chars(mqtt_point.reg.client_id, sizeof(mqtt_point.reg.client_id), item);
	}
	if ((addr >= MODBUS_MQTT_PUB_TOPIC_START) && (addr <= MODBUS_MQTT_PUB_TOPIC_END)) {
		item = (uint8_t)(addr - MODBUS_MQTT_PUB_TOPIC_START);
		return mqtt_pack_chars(mqtt_point.reg.pub_topic, sizeof(mqtt_point.reg.pub_topic), item);
	}
	if ((addr >= MODBUS_MQTT_SUB_TOPIC_START) && (addr <= MODBUS_MQTT_SUB_TOPIC_END)) {
		item = (uint8_t)(addr - MODBUS_MQTT_SUB_TOPIC_START);
		return mqtt_pack_chars(mqtt_point.reg.sub_topic, sizeof(mqtt_point.reg.sub_topic), item);
	}
	return 0;
}

static bool mqtt_write_register(uint16_t addr, uint16_t value)
{
	uint8_t item;

	if (addr == MODBUS_MQTT_ENABLE) {
		mqtt_point.reg.enable = (U8_T)(value ? 1 : 0);
		Modbus.enable_mqtt = mqtt_point.reg.enable;
		return true;
	}
	if (addr == MODBUS_MQTT_CONNECTED) {
		return false;
	}
	if (addr == MODBUS_MQTT_PORT) {
		mqtt_point.reg.port = value ? value : 1883;
		return true;
	}
	if ((addr >= MODBUS_MQTT_BROKER_START) && (addr <= MODBUS_MQTT_BROKER_END)) {
		item = (uint8_t)(addr - MODBUS_MQTT_BROKER_START);
		mqtt_unpack_chars(mqtt_point.reg.broker, sizeof(mqtt_point.reg.broker), item, value);
		return true;
	}
	if ((addr >= MODBUS_MQTT_USERNAME_START) && (addr <= MODBUS_MQTT_USERNAME_END)) {
		item = (uint8_t)(addr - MODBUS_MQTT_USERNAME_START);
		mqtt_unpack_chars(mqtt_point.reg.username, sizeof(mqtt_point.reg.username), item, value);
		return true;
	}
	if ((addr >= MODBUS_MQTT_PASSWORD_START) && (addr <= MODBUS_MQTT_PASSWORD_END)) {
		item = (uint8_t)(addr - MODBUS_MQTT_PASSWORD_START);
		mqtt_unpack_chars(mqtt_point.reg.password, sizeof(mqtt_point.reg.password), item, value);
		return true;
	}
	if ((addr >= MODBUS_MQTT_CLIENT_ID_START) && (addr <= MODBUS_MQTT_CLIENT_ID_END)) {
		item = (uint8_t)(addr - MODBUS_MQTT_CLIENT_ID_START);
		mqtt_unpack_chars(mqtt_point.reg.client_id, sizeof(mqtt_point.reg.client_id), item, value);
		return true;
	}
	if ((addr >= MODBUS_MQTT_PUB_TOPIC_START) && (addr <= MODBUS_MQTT_PUB_TOPIC_END)) {
		item = (uint8_t)(addr - MODBUS_MQTT_PUB_TOPIC_START);
		mqtt_unpack_chars(mqtt_point.reg.pub_topic, sizeof(mqtt_point.reg.pub_topic), item, value);
		return true;
	}
	if ((addr >= MODBUS_MQTT_SUB_TOPIC_START) && (addr <= MODBUS_MQTT_SUB_TOPIC_END)) {
		item = (uint8_t)(addr - MODBUS_MQTT_SUB_TOPIC_START);
		mqtt_unpack_chars(mqtt_point.reg.sub_topic, sizeof(mqtt_point.reg.sub_topic), item, value);
		return true;
	}
	return false;
}

void mqtt_config_write_by_block(uint16_t addr, uint8_t HeadLen, uint8_t *pData)
{
	bool changed = false;
	extern esp_err_t save_mqtt_config_to_flash(void);

	if (pData == NULL) {
		return;
	}

	if (pData[HeadLen + 1] == MULTIPLE_WRITE_VARIABLES) {
		uint16_t quantity = ((uint16_t)pData[HeadLen + 4] << 8) | pData[HeadLen + 5];
		uint8_t byte_count = pData[HeadLen + 6];
		for (uint16_t index = 0; index < quantity; index++) {
			uint16_t data_offset = HeadLen + 7 + (index * 2);
			if ((index * 2 + 1) >= byte_count) {
				break;
			}
			uint16_t value_word = ((uint16_t)pData[data_offset] << 8) | pData[data_offset + 1];
			changed |= mqtt_write_register((uint16_t)(addr + index), value_word);
		}
	} else {
		uint16_t value_word = ((uint16_t)pData[HeadLen + 4] << 8) | pData[HeadLen + 5];
		changed = mqtt_write_register(addr, value_word);
	}

	if (changed) {
		save_mqtt_config_to_flash();
		Mqtt_Handler_Apply_Config();
	}
}
