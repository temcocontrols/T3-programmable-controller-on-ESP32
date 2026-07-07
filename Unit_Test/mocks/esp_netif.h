#ifndef ESP_NETIF_H
#define ESP_NETIF_H

#include <stdint.h>
#include "esp_err.h"
#include "esp_netif_ip_addr.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* esp_netif_t;

typedef enum {
    ESP_IPADDR_TYPE_V4 = 0,
    ESP_IPADDR_TYPE_V6 = 1,
} esp_ip_addr_type_t;

#define IPADDR_TYPE_V4 ESP_IPADDR_TYPE_V4

typedef struct {
    union {
        esp_ip4_addr_t ip4;
    } u_addr;
    esp_ip_addr_type_t type;
} esp_ip_addr_t;

typedef struct {
    esp_netif_ip_info_t ip_info;
} ip_event_got_ip_t;

typedef struct {
    esp_ip_addr_t ip;
} esp_netif_dns_info_t;

typedef enum {
    ESP_NETIF_DNS_MAIN,
    ESP_NETIF_DNS_BACKUP,
    ESP_NETIF_DNS_FALLBACK,
} esp_netif_dns_type_t;

#define esp_ip4_addr1(ip) (((ip)->addr >> 0) & 0xff)
#define esp_ip4_addr2(ip) (((ip)->addr >> 8) & 0xff)
#define esp_ip4_addr3(ip) (((ip)->addr >> 16) & 0xff)
#define esp_ip4_addr4(ip) (((ip)->addr >> 24) & 0xff)

#define ESP_IP4TOADDR(a, b, c, d) (((uint32_t)(a) << 0) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#define IP4_ADDR(ip, a, b, c, d) do { \
    (ip)->addr = ESP_IP4TOADDR(a, b, c, d); \
} while(0)

#define IP_ADDR4(ipaddr, a, b, c, d) do { \
    (ipaddr)->type = ESP_IPADDR_TYPE_V4; \
    (ipaddr)->u_addr.ip4.addr = ((uint32_t)(a) << 0) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24); \
} while(0)

int esp_netif_init(void);
esp_netif_t esp_netif_create_default_wifi_sta(void);
esp_netif_t esp_netif_get_handle_from_ifkey(const char* if_key);
int esp_netif_dhcpc_stop(esp_netif_t esp_netif);
int esp_netif_dhcpc_start(esp_netif_t esp_netif);
int esp_netif_set_ip_info(esp_netif_t esp_netif, const esp_netif_ip_info_t* ip_info);
int esp_netif_set_dns_info(esp_netif_t esp_netif, esp_netif_dns_type_t type, esp_netif_dns_info_t* dns);

typedef struct {
    int dummy;
} esp_netif_config_t;

#define ESP_NETIF_DEFAULT_ETH() {0}

esp_netif_t esp_netif_new(const esp_netif_config_t *config);
esp_err_t esp_netif_attach(esp_netif_t esp_netif, void *glue);

int esp_event_handler_register(const char* event_base, int32_t event_id, void* event_handler, void* event_handler_arg);

#ifdef __cplusplus
}
#endif

#endif // ESP_NETIF_H
