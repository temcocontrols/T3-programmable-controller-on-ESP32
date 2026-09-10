#ifndef ESP_HTTP_CLIENT_H
#define ESP_HTTP_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void* esp_http_client_handle_t;

typedef enum {
    HTTP_EVENT_ERROR,
    HTTP_EVENT_ON_CONNECTED,
    HTTP_EVENT_HEADER_SENT,
    HTTP_EVENT_ON_HEADER,
    HTTP_EVENT_ON_DATA,
    HTTP_EVENT_ON_FINISH,
    HTTP_EVENT_DISCONNECTED,
    HTTP_EVENT_REDIRECT,
} esp_http_client_event_id_t;

typedef struct {
    esp_http_client_event_id_t event_id;
    void *user_data;
    void *data;
    int data_len;
    const char *header_key;
    const char *header_value;
} esp_http_client_event_t;

typedef int (*http_event_handle_cb)(esp_http_client_event_t *evt);

typedef struct {
    const char *url;
    http_event_handle_cb event_handler;
    void *user_data;
    const char *cert_pem;
} esp_http_client_config_t;

#ifdef __cplusplus
}
#endif

#endif // ESP_HTTP_CLIENT_H
