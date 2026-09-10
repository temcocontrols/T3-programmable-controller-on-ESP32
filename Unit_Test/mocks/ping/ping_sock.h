#ifndef PING_SOCK_H
#define PING_SOCK_H

#include <stdint.h>
#include "esp_err.h"
#include "lwip/inet.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* esp_ping_handle_t;

typedef struct {
    ip_addr_t target_addr;
    uint32_t count;
    uint32_t interval_ms;
    uint32_t timeout_ms;
    uint32_t data_size;
    uint32_t tos;
    uint32_t ttl;
} esp_ping_config_t;

#define ESP_PING_DEFAULT_CONFIG() { .count = 0 }
#define ESP_PING_COUNT_INFINITE 0

typedef struct {
    void (*on_ping_success)(esp_ping_handle_t hdl, void *args);
    void (*on_ping_timeout)(esp_ping_handle_t hdl, void *args);
    void (*on_ping_end)(esp_ping_handle_t hdl, void *args);
    void *cb_args;
} esp_ping_callbacks_t;

typedef enum {
    ESP_PING_PROF_SEQNO,
    ESP_PING_PROF_TTL,
    ESP_PING_PROF_IPADDR,
    ESP_PING_PROF_SIZE,
    ESP_PING_PROF_TIMEGAP,
    ESP_PING_PROF_REQUEST,
    ESP_PING_PROF_REPLY,
    ESP_PING_PROF_DURATION,
} esp_ping_profile_t;

esp_err_t esp_ping_get_profile(esp_ping_handle_t hdl, esp_ping_profile_t profile_type, void *out_val, uint32_t val_len);
esp_err_t esp_ping_new_session(const esp_ping_config_t *config, const esp_ping_callbacks_t *cbs, esp_ping_handle_t *ping_hdl);
esp_err_t esp_ping_start(esp_ping_handle_t ping_hdl);

#ifdef __cplusplus
}
#endif

#endif // PING_SOCK_H
