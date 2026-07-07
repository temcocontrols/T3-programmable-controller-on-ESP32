#ifndef MOCK_MQTT_CLIENT_H
#define MOCK_MQTT_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

typedef void* esp_mqtt_client_handle_t;

typedef enum {
    MQTT_EVENT_ERROR = 0,
    MQTT_EVENT_CONNECTED,
    MQTT_EVENT_DISCONNECTED,
    MQTT_EVENT_SUBSCRIBED,
    MQTT_EVENT_UNSUBSCRIBED,
    MQTT_EVENT_PUBLISHED,
    MQTT_EVENT_DATA,
    MQTT_EVENT_BEFORE_CONNECT,
} esp_mqtt_event_id_t;

typedef enum {
    MQTT_ERROR_TYPE_NONE = 0,
    MQTT_ERROR_TYPE_TCP_TRANSPORT,
    MQTT_ERROR_TYPE_CONNECTION_REFUSED,
} esp_mqtt_error_type_t;

typedef struct {
    esp_mqtt_error_type_t error_type;
    int esp_tls_last_esp_err;
    int esp_tls_stack_err;
    int esp_transport_sock_errno;
} esp_mqtt_error_codes_t;

typedef struct {
    esp_mqtt_event_id_t event_id;
    esp_mqtt_client_handle_t client;
    void* user_context;
    char* topic;
    int topic_len;
    char* data;
    int data_len;
    int msg_id;
    esp_mqtt_error_codes_t* error_handle;
} esp_mqtt_event_t;

typedef esp_mqtt_event_t* esp_mqtt_event_handle_t;

typedef struct {
    struct {
        struct {
            const char* uri;
        } address;
    } broker;
} esp_mqtt_client_config_t;

static inline esp_mqtt_client_handle_t esp_mqtt_client_init(const esp_mqtt_client_config_t* config) { (void)config; return (esp_mqtt_client_handle_t)1; }
static inline esp_err_t esp_mqtt_client_register_event(esp_mqtt_client_handle_t client, esp_event_id_t event, esp_event_handler_t event_handler, void* event_handler_arg) { (void)client; (void)event; (void)event_handler; (void)event_handler_arg; return ESP_OK; }
static inline esp_err_t esp_mqtt_client_unregister_event(esp_mqtt_client_handle_t client, esp_event_id_t event, esp_event_handler_t event_handler) { (void)client; (void)event; (void)event_handler; return ESP_OK; }
static inline esp_err_t esp_mqtt_client_start(esp_mqtt_client_handle_t client) { (void)client; return ESP_OK; }
static inline esp_err_t esp_mqtt_client_stop(esp_mqtt_client_handle_t client) { (void)client; return ESP_OK; }
static inline esp_err_t esp_mqtt_client_destroy(esp_mqtt_client_handle_t client) { (void)client; return ESP_OK; }
int esp_mqtt_client_publish(esp_mqtt_client_handle_t client, const char* topic, const char* data, int len, int qos, int retain);
static inline int esp_mqtt_client_subscribe(esp_mqtt_client_handle_t client, const char* topic, int qos) { (void)client; (void)topic; (void)qos; return 1; }

#endif // MOCK_MQTT_CLIENT_H
