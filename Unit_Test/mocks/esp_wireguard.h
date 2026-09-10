#ifndef MOCK_ESP_WIREGUARD_H
#define MOCK_ESP_WIREGUARD_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int listen_port;
    int port;
    const char *private_key;
    const char *public_key;
    const char *preshared_key;
    const char *allowed_ip;
    const char *endpoint;
    const char *allowed_ip_mask;
    int persistent_keepalive;
} wireguard_config_t;

#define ESP_WIREGUARD_CONFIG_DEFAULT() {0}

typedef struct {
    int dummy;
} wireguard_ctx_t;

esp_err_t esp_wireguard_init(const wireguard_config_t *config, wireguard_ctx_t *ctx);
esp_err_t esp_wireguard_connect(wireguard_ctx_t *ctx);
esp_err_t esp_wireguard_set_default(wireguard_ctx_t *ctx);
esp_err_t esp_wireguardif_peer_is_up(wireguard_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif // MOCK_ESP_WIREGUARD_H
