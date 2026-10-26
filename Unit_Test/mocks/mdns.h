#ifndef MDNS_H
#define MDNS_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t mdns_init(void);
esp_err_t mdns_hostname_set(const char *hostname);
esp_err_t mdns_instance_name_set(const char *instance);
esp_err_t mdns_service_add(const char *instance, const char *service, const char *proto, uint16_t port, const void *txt, uint8_t num_items);

#ifdef __cplusplus
}
#endif

#endif // MDNS_H
