#ifndef MOCK_ESP_ETH_H
#define MOCK_ESP_ETH_H

#include "esp_err.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* esp_eth_handle_t;

#define ETH_EVENT "eth_event"

typedef enum {
    ETHERNET_EVENT_START,
    ETHERNET_EVENT_STOP,
    ETHERNET_EVENT_CONNECTED,
    ETHERNET_EVENT_DISCONNECTED,
} eth_event_t;

typedef enum {
    ETH_CMD_G_MAC_ADDR,
} eth_ioctl_cmd_t;

typedef struct {
    int dummy;
} eth_mac_config_t;

typedef struct {
    int phy_addr;
    int reset_gpio_num;
} eth_phy_config_t;

typedef struct {
    struct {
        int mdc_num;
        int mdio_num;
    } smi_gpio;
} eth_esp32_emac_config_t;

typedef struct esp_eth_mac_s esp_eth_mac_t;
typedef struct esp_eth_phy_s esp_eth_phy_t;

typedef struct {
    esp_eth_mac_t *mac;
    esp_eth_phy_t *phy;
} esp_eth_config_t;

#define ETH_MAC_DEFAULT_CONFIG() {0}
#define ETH_PHY_DEFAULT_CONFIG() {0}
#define ETH_ESP32_EMAC_DEFAULT_CONFIG() {0}
#define ETH_DEFAULT_CONFIG(mac, phy) { mac, phy }

esp_eth_mac_t *esp_eth_mac_new_esp32(const eth_esp32_emac_config_t *emac_config, const eth_mac_config_t *mac_config);
esp_eth_phy_t *esp_eth_phy_new_ip101(const eth_phy_config_t *phy_config);

esp_err_t esp_eth_driver_install(const esp_eth_config_t *config, esp_eth_handle_t *out_hdl);
esp_err_t esp_eth_ioctl(esp_eth_handle_t hdl, uint32_t cmd, void *data);
esp_err_t esp_eth_start(esp_eth_handle_t hdl);
void *esp_eth_new_netif_glue(esp_eth_handle_t hdl);

#ifdef __cplusplus
}
#endif

#endif // MOCK_ESP_ETH_H
