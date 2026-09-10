#ifndef ESP_NETIF_IP_ADDR_H
#define ESP_NETIF_IP_ADDR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t addr;
} esp_ip4_addr_t;

typedef struct {
    esp_ip4_addr_t ip;
    esp_ip4_addr_t netmask;
    esp_ip4_addr_t gw;
} esp_netif_ip_info_t;
#define IPSTR "%d.%d.%d.%d"
#define IP2STR(ipaddr) \
    (int)((ipaddr)->addr & 0xFF), \
    (int)(((ipaddr)->addr >> 8) & 0xFF), \
    (int)(((ipaddr)->addr >> 16) & 0xFF), \
    (int)(((ipaddr)->addr >> 24) & 0xFF)
#ifdef __cplusplus
}
#endif

#endif // ESP_NETIF_IP_ADDR_H
