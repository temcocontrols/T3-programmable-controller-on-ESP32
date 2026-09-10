#ifndef ESP_HTTP_SERVER_H
#define ESP_HTTP_SERVER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* httpd_handle_t;

typedef struct httpd_req {
    int dummy; /* opaque in tests */
} httpd_req_t;

#define HTTPD_RESP_USE_STRLEN (-1)

typedef struct {
    const char *uri;
    int method;
    esp_err_t (*handler)(httpd_req_t* req);
    void *user_ctx;
} httpd_uri_t;

typedef struct {
    int server_port;
    int max_uri_handlers;
    bool lru_purge_enable;
} httpd_config_t;

#define HTTPD_DEFAULT_CONFIG() (httpd_config_t){.server_port=80, .max_uri_handlers=8, .lru_purge_enable=false}

/* Methods used by wifi_web_server.c */
esp_err_t httpd_start(httpd_handle_t *handle, const httpd_config_t *config);
esp_err_t httpd_stop(httpd_handle_t handle);
esp_err_t httpd_register_uri_handler(httpd_handle_t handle, const httpd_uri_t *uri);
esp_err_t httpd_resp_send(httpd_req_t *req, const char *buf, int len);
esp_err_t httpd_resp_send_500(httpd_req_t *req);
esp_err_t httpd_resp_sendstr(httpd_req_t *req, const char *str);
void httpd_resp_set_type(httpd_req_t *req, const char *type);
int httpd_req_recv(httpd_req_t *req, char *buf, size_t buf_len);

/* HTTP methods */
#define HTTP_GET 0
#define HTTP_POST 1

#ifdef __cplusplus
}
#endif

#endif // ESP_HTTP_SERVER_H
