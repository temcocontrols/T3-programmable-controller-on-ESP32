#ifndef MOCK_ESP_WIFI_H
#define MOCK_ESP_WIFI_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Events
#define WIFI_EVENT "wifi_event"
#define IP_EVENT   "ip_event"

typedef enum {
    WIFI_EVENT_STA_START,
    WIFI_EVENT_SCAN_DONE,
    WIFI_EVENT_STA_DISCONNECTED,
} wifi_event_t;

typedef enum {
    IP_EVENT_STA_GOT_IP,
    IP_EVENT_ETH_GOT_IP,
} ip_event_t;

#define ESP_ERR_WIFI_NOT_STARTED (ESP_FAIL)

typedef enum {
    WIFI_MODE_NULL = 0,
    WIFI_MODE_STA,
    WIFI_MODE_AP,
    WIFI_MODE_APSTA,
} wifi_mode_t;

typedef enum {
    WIFI_IF_STA,
    WIFI_IF_AP,
} wifi_interface_t;

typedef enum { WIFI_AUTH_OPEN = 0, WIFI_AUTH_WPA_PSK = 1, WIFI_AUTH_WPA2_PSK = 2 } wifi_auth_mode_t;
/* Backwards-compatible alias used in production code */
#define WIFI_AUTH_WPA_WPA2_PSK WIFI_AUTH_WPA2_PSK

typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
    /* Added fields to match newer esp-idf wifi_sta_config_t */
    int8_t bssid_set; /* non-zero if bssid is set */
    uint8_t bssid[6];
    struct {
        bool capable;
        bool required;
    } pmf_cfg;
} wifi_sta_config_t;

typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
    uint8_t ssid_len;
    uint8_t channel;
    wifi_auth_mode_t authmode;
    uint8_t max_connection;
} wifi_ap_config_t;

typedef union {
    wifi_sta_config_t sta;
    wifi_ap_config_t ap;
} wifi_config_t;

typedef struct {
    const uint8_t *ssid;
    const uint8_t *bssid;
    uint8_t channel;
    bool show_hidden;
} wifi_scan_config_t;

typedef struct {
    uint8_t ssid[33];
    uint8_t bssid[6];
    int8_t primary; /* channel */
    int8_t rssi;
    wifi_auth_mode_t authmode;
} wifi_ap_record_t;

typedef struct {
    int dummy;
} wifi_init_config_t;

#define WIFI_INIT_CONFIG_DEFAULT() {0}

int esp_wifi_init(const wifi_init_config_t *config);
int esp_wifi_stop(void);
int esp_wifi_start(void);
int esp_wifi_connect(void);
int esp_wifi_set_mode(wifi_mode_t mode);
int esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf);
int esp_wifi_sta_get_ap_info(wifi_ap_record_t *ap_info);
int esp_wifi_scan_start(const void *config, bool block);
int esp_wifi_scan_get_ap_num(uint16_t *number);
int esp_wifi_scan_get_ap_records(uint16_t *number, wifi_ap_record_t *ap_records);
int esp_wifi_get_mode(wifi_mode_t *mode);
int esp_wifi_disconnect(void);

#ifdef __cplusplus
}
#endif

#endif // MOCK_ESP_WIFI_H
