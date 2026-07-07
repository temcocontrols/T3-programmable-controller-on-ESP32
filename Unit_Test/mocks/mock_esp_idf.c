// Mocks for ESP-IDF APIs and drivers
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_partition.h"
#include "driver/pcnt.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "driver/ledc.h"
#include "esp_rom_sys.h"
#include "led_strip.h"
#include "driver/rmt.h"
#include "driver/spi_master.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "ping/ping_sock.h"
#include "esp_wireguard.h"

// mbedtls
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/base64.h"
#include "mbedtls/esp_debug.h"

// System
void esp_restart(void) {}
uint32_t esp_get_free_heap_size(void) { return 100000; }

// WiFi
int esp_wifi_init(const wifi_init_config_t *config) { (void)config; return ESP_OK; }
int esp_wifi_stop(void) { return ESP_OK; }
int esp_wifi_start(void) { return ESP_OK; }
int esp_wifi_connect(void) { return ESP_OK; }
int esp_wifi_set_mode(wifi_mode_t mode) { (void)mode; return ESP_OK; }
int esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf) { (void)interface; (void)conf; return ESP_OK; }
int esp_wifi_sta_get_ap_info(wifi_ap_record_t *ap_info) { (void)ap_info; return ESP_OK; }
int esp_wifi_get_mode(wifi_mode_t *mode) { if (mode) *mode = WIFI_MODE_STA; return ESP_OK; }
int esp_wifi_disconnect(void) { return ESP_OK; }
int esp_wifi_scan_start(const void *config, bool block) { (void)config; (void)block; return ESP_OK; }
int esp_wifi_scan_get_ap_num(uint16_t *number) { if (number) *number = 0; return ESP_OK; }
int esp_wifi_scan_get_ap_records(uint16_t *number, wifi_ap_record_t *ap_records) { (void)ap_records; if (number) *number = 0; return ESP_OK; }

// Netif
int esp_netif_init(void) { return ESP_OK; }
esp_netif_t esp_netif_create_default_wifi_sta(void) { return (esp_netif_t)1; }
esp_netif_t esp_netif_get_handle_from_ifkey(const char* if_key) { (void)if_key; return (esp_netif_t)1; }
int esp_netif_dhcpc_stop(esp_netif_t esp_netif) { (void)esp_netif; return ESP_OK; }
int esp_netif_dhcpc_start(esp_netif_t esp_netif) { (void)esp_netif; return ESP_OK; }
int esp_netif_set_ip_info(esp_netif_t esp_netif, const esp_netif_ip_info_t* ip_info) { (void)esp_netif; (void)ip_info; return ESP_OK; }
int esp_netif_set_dns_info(esp_netif_t esp_netif, esp_netif_dns_type_t type, esp_netif_dns_info_t* dns) { (void)esp_netif; (void)type; (void)dns; return ESP_OK; }
esp_netif_t esp_netif_new(const esp_netif_config_t *config) { (void)config; return (esp_netif_t)1; }
esp_err_t esp_netif_attach(esp_netif_t esp_netif, void *glue) { (void)esp_netif; (void)glue; return ESP_OK; }
int esp_event_handler_register(const char* event_base, int32_t event_id, void* event_handler, void* event_handler_arg) {
    (void)event_base; (void)event_id; (void)event_handler; (void)event_handler_arg;
    return ESP_OK;
}

// Ethernet
esp_eth_mac_t *esp_eth_mac_new_esp32(const eth_esp32_emac_config_t *emac_config, const eth_mac_config_t *mac_config) {
    (void)emac_config; (void)mac_config;
    return (esp_eth_mac_t*)1;
}
esp_eth_phy_t *esp_eth_phy_new_ip101(const eth_phy_config_t *phy_config) {
    (void)phy_config;
    return (esp_eth_phy_t*)1;
}
esp_err_t esp_eth_driver_install(const esp_eth_config_t *config, esp_eth_handle_t *out_hdl) {
    (void)config;
    if (out_hdl) *out_hdl = (esp_eth_handle_t)1;
    return ESP_OK;
}
esp_err_t esp_eth_ioctl(esp_eth_handle_t hdl, uint32_t cmd, void *data) {
    (void)hdl; (void)cmd; (void)data;
    return ESP_OK;
}
esp_err_t esp_eth_start(esp_eth_handle_t hdl) {
    (void)hdl;
    return ESP_OK;
}
void *esp_eth_new_netif_glue(esp_eth_handle_t hdl) {
    (void)hdl;
    return (void*)1;
}

// Flash
esp_err_t esp_flash_get_size(esp_flash_t *chip, uint32_t *out_size) {
    (void)chip;
    if (out_size) *out_size = 4 * 1024 * 1024; // Mock 4MB flash
    return ESP_OK;
}

// NVS Flash
int nvs_flash_init(void) { return ESP_OK; }
int nvs_flash_erase(void) { return ESP_OK; }

// NVS
int nvs_open(const char* name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle) {
    (void)name; (void)open_mode;
    if (out_handle) *out_handle = 1;
    return ESP_OK;
}
int nvs_get_u32(nvs_handle_t handle, const char* key, uint32_t* out_value) {
    (void)handle; (void)key;
    if (out_value) *out_value = 0;
    return ESP_OK;
}
int nvs_set_u32(nvs_handle_t handle, const char* key, uint32_t value) {
    (void)handle; (void)key; (void)value;
    return ESP_OK;
}
int nvs_get_u16(nvs_handle_t handle, const char* key, uint16_t* out_value) {
    (void)handle; (void)key;
    if (out_value) *out_value = 0;
    return ESP_OK;
}
int nvs_set_u16(nvs_handle_t handle, const char* key, uint16_t value) {
    (void)handle; (void)key; (void)value;
    return ESP_OK;
}
int nvs_get_i16(nvs_handle_t handle, const char* key, int16_t* out_value) {
    (void)handle; (void)key;
    if (out_value) *out_value = 0;
    return ESP_OK;
}
int nvs_set_i16(nvs_handle_t handle, const char* key, int16_t value) {
    (void)handle; (void)key; (void)value;
    return ESP_OK;
}
int nvs_get_u8(nvs_handle_t handle, const char* key, uint8_t* out_value) {
    (void)handle; (void)key;
    if (out_value) *out_value = 0;
    return ESP_OK;
}
int nvs_set_u8(nvs_handle_t handle, const char* key, uint8_t value) {
    (void)handle; (void)key; (void)value;
    return ESP_OK;
}
int nvs_get_blob(nvs_handle_t handle, const char* key, void* out_value, size_t* length) {
    (void)handle; (void)key;
    if (length) {
        if (out_value) {
            memset(out_value, 0, *length);
        } else {
            *length = 32; // dummy size
        }
    }
    return ESP_OK;
}
int nvs_set_blob(nvs_handle_t handle, const char* key, const void* value, size_t length) {
    (void)handle; (void)key; (void)value; (void)length;
    return ESP_OK;
}
int nvs_commit(nvs_handle_t handle) { (void)handle; return ESP_OK; }
void nvs_close(nvs_handle_t handle) { (void)handle; }

// Partition
static esp_partition_t dummy_partition = {
    .label = "storage",
    .address = 0x10000,
    .size = 0x20000
};

const esp_partition_t* esp_partition_find_first(esp_partition_type_t type, esp_partition_subtype_t subtype, const char* label) {
    (void)type; (void)subtype; (void)label;
    return &dummy_partition;
}

esp_err_t esp_partition_erase_range(const esp_partition_t* partition, uint32_t offset, uint32_t size) {
    (void)partition; (void)offset; (void)size;
    return ESP_OK;
}

esp_err_t esp_partition_write(const esp_partition_t* partition, uint32_t dst_offset, const void* src, uint32_t size) {
    (void)partition; (void)dst_offset; (void)src; (void)size;
    return ESP_OK;
}

esp_err_t esp_partition_read(const esp_partition_t* partition, uint32_t src_offset, void* dst, uint32_t size) {
    (void)partition; (void)src_offset; (void)dst; (void)size;
    if (dst) {
        memset(dst, 0, size);
    }
    return ESP_OK;
}

// PCNT
pcnt_dev_t PCNT = {0};

esp_err_t pcnt_unit_config(const pcnt_config_t *pcnt_config) { (void)pcnt_config; return ESP_OK; }
esp_err_t pcnt_set_filter_value(pcnt_unit_t pcnt_unit, uint16_t filter_val) { (void)pcnt_unit; (void)filter_val; return ESP_OK; }
esp_err_t pcnt_filter_enable(pcnt_unit_t pcnt_unit) { (void)pcnt_unit; return ESP_OK; }
esp_err_t pcnt_counter_pause(pcnt_unit_t pcnt_unit) { (void)pcnt_unit; return ESP_OK; }
esp_err_t pcnt_counter_clear(pcnt_unit_t pcnt_unit) { (void)pcnt_unit; return ESP_OK; }
esp_err_t pcnt_counter_resume(pcnt_unit_t pcnt_unit) { (void)pcnt_unit; return ESP_OK; }
esp_err_t pcnt_get_counter_value(pcnt_unit_t pcnt_unit, int16_t* count) {
    (void)pcnt_unit;
    if (count) *count = 0;
    return ESP_OK;
}
esp_err_t pcnt_isr_register(pcnt_isr_t fn, void *arg, int intr_alloc_flags, pcnt_isr_handle_t *handle) {
    (void)fn; (void)arg; (void)intr_alloc_flags;
    if (handle) *handle = (pcnt_isr_handle_t)1;
    return ESP_OK;
}
esp_err_t pcnt_intr_enable(pcnt_unit_t pcnt_unit) { (void)pcnt_unit; return ESP_OK; }

// ADC
esp_err_t adc1_config_width(adc_bits_width_t width_bit) { (void)width_bit; return ESP_OK; }
esp_err_t adc1_config_channel_atten(adc1_channel_t channel, adc_atten_t atten) { (void)channel; (void)atten; return ESP_OK; }
int adc1_get_raw(adc1_channel_t channel) { (void)channel; return 2048; }

esp_adc_cal_value_t esp_adc_cal_characterize(
    adc_unit_t adc_num,
    adc_atten_t atten,
    adc_bits_width_t bit_width,
    uint32_t default_vref,
    esp_adc_cal_characteristics_t *chars
) {
    if (chars) {
        chars->adc_num = adc_num;
        chars->atten = atten;
        chars->bit_width = bit_width;
        chars->vref = default_vref;
    }
    return ESP_ADC_CAL_VAL_DEFAULT_VREF;
}

uint32_t esp_adc_cal_raw_to_voltage(uint32_t adc_reading, const esp_adc_cal_characteristics_t *chars) {
    (void)chars;
    return (adc_reading * 3300) / 4095;
}

esp_err_t esp_intr_free(void* handle) {
    (void)handle;
    return ESP_OK;
}

// LEDC
int ledc_channel_config(const ledc_channel_config_t* config) { (void)config; return ESP_OK; }
int ledc_timer_config(const ledc_timer_config_t* config) { (void)config; return ESP_OK; }
int ledc_set_duty(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t duty) { (void)speed_mode; (void)channel; (void)duty; return ESP_OK; }
int ledc_update_duty(ledc_mode_t speed_mode, ledc_channel_t channel) { (void)speed_mode; (void)channel; return ESP_OK; }

// ROM
void esp_rom_delay_us(uint32_t us) { (void)us; }

// mbedtls stubs
void mbedtls_net_init(mbedtls_net_context *ctx) { if (ctx) ctx->fd = -1; }
int mbedtls_net_connect(mbedtls_net_context *ctx, const char *host, const char *port, int proto) {
    (void)ctx; (void)host; (void)port; (void)proto;
    return 0;
}
int mbedtls_net_send(void *ctx, const unsigned char *buf, size_t len) {
    (void)ctx; (void)buf;
    return len;
}
int mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len) {
    (void)ctx; (void)buf;
    return len;
}
void mbedtls_net_free(mbedtls_net_context *ctx) { (void)ctx; }

void mbedtls_esp_enable_debug_log(mbedtls_ssl_config *conf, int threshold) { (void)conf; (void)threshold; }

void mbedtls_ssl_init(mbedtls_ssl_context *ssl) { (void)ssl; }
void mbedtls_x509_crt_init(mbedtls_x509_crt *crt) { (void)crt; }
void mbedtls_ssl_config_init(mbedtls_ssl_config *conf) { (void)conf; }

int mbedtls_x509_crt_parse(mbedtls_x509_crt *chain, const unsigned char *buf, size_t buflen) {
    (void)chain; (void)buf; (void)buflen;
    return 0;
}
int mbedtls_ssl_set_hostname(mbedtls_ssl_context *ssl, const char *hostname) {
    (void)ssl; (void)hostname;
    return 0;
}
int mbedtls_ssl_config_defaults(mbedtls_ssl_config *conf, int endpoint, int transport, int preset) {
    (void)conf; (void)endpoint; (void)transport; (void)preset;
    return 0;
}
void mbedtls_ssl_conf_authmode(mbedtls_ssl_config *conf, int authmode) { (void)conf; (void)authmode; }
void mbedtls_ssl_conf_ca_chain(mbedtls_ssl_config *conf, mbedtls_x509_crt *ca_chain, void *ca_crl) {
    (void)conf; (void)ca_chain; (void)ca_crl;
}
void mbedtls_ssl_conf_rng(mbedtls_ssl_config *conf, int (*f_rng)(void *, unsigned char *, size_t), void *p_rng) {
    (void)conf; (void)f_rng; (void)p_rng;
}
int mbedtls_ssl_setup(mbedtls_ssl_context *ssl, const mbedtls_ssl_config *conf) {
    (void)ssl; (void)conf;
    return 0;
}
void mbedtls_ssl_set_bio(mbedtls_ssl_context *ssl, void *p_bio,
                         int (*f_send)(void *, const unsigned char *, size_t),
                         int (*f_recv)(void *, unsigned char *, size_t),
                         int (*f_recv_timeout)(void *, unsigned char *, size_t, uint32_t)) {
    (void)ssl; (void)p_bio; (void)f_send; (void)f_recv; (void)f_recv_timeout;
}

int mbedtls_ssl_handshake(mbedtls_ssl_context *ssl) { (void)ssl; return 0; }
uint32_t mbedtls_ssl_get_verify_result(const mbedtls_ssl_context *ssl) { (void)ssl; return 0; }
int mbedtls_x509_crt_verify_info(char *buf, size_t size, const char *prefix, uint32_t flags) {
    (void)buf; (void)size; (void)prefix; (void)flags;
    return 0;
}
const char *mbedtls_ssl_get_ciphersuite(const mbedtls_ssl_context *ssl) { (void)ssl; return "MOCK_CIPHER"; }
int mbedtls_ssl_write(mbedtls_ssl_context *ssl, const unsigned char *buf, size_t len) {
    (void)ssl; (void)buf;
    return len;
}
int mbedtls_ssl_read(mbedtls_ssl_context *ssl, unsigned char *buf, size_t len) {
    (void)ssl; (void)buf;
    return len;
}
int mbedtls_ssl_close_notify(mbedtls_ssl_context *ssl) { (void)ssl; return 0; }

void mbedtls_x509_crt_free(mbedtls_x509_crt *crt) { (void)crt; }
void mbedtls_ssl_free(mbedtls_ssl_context *ssl) { (void)ssl; }
void mbedtls_ssl_config_free(mbedtls_ssl_config *conf) { (void)conf; }

void mbedtls_entropy_init(mbedtls_entropy_context *entropy) { (void)entropy; }
int mbedtls_entropy_func(void *data, unsigned char *output, size_t len) {
    (void)data; (void)output; (void)len;
    return 0;
}
void mbedtls_entropy_free(mbedtls_entropy_context *entropy) { (void)entropy; }

void mbedtls_ctr_drbg_init(mbedtls_ctr_drbg_context *ctx) { (void)ctx; }
int mbedtls_ctr_drbg_seed(mbedtls_ctr_drbg_context *ctx,
                          int (*f_entropy)(void *, unsigned char *, size_t),
                          void *p_entropy,
                          const unsigned char *custom,
                          size_t len) {
    (void)ctx; (void)f_entropy; (void)p_entropy; (void)custom; (void)len;
    return 0;
}
int mbedtls_ctr_drbg_random(void *p_rng, unsigned char *output, size_t output_len) {
    (void)p_rng; (void)output; (void)output_len;
    return 0;
}
void mbedtls_ctr_drbg_free(mbedtls_ctr_drbg_context *ctx) { (void)ctx; }

void mbedtls_strerror(int errnum, char *buffer, size_t buflen) {
    (void)errnum;
    if (buffer && buflen > 0) {
        snprintf(buffer, buflen, "Mock error %d", errnum);
    }
}

int mbedtls_base64_encode(unsigned char *dst, size_t dlen, size_t *olen,
                          const unsigned char *src, size_t slen) {
    (void)src; (void)slen;
    if (olen) *olen = 4;
    if (dst && dlen >= 4) {
        memcpy(dst, "dGVz", 4);
    }
    return 0;
}

// LED strip mock implementation (legacy)
static esp_err_t mock_led_strip_set_pixel(led_strip_t *strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue) {
    (void)strip; (void)index; (void)red; (void)green; (void)blue;
    return ESP_OK;
}

static esp_err_t mock_led_strip_refresh(led_strip_t *strip, uint32_t timeout_ms) {
    (void)strip; (void)timeout_ms;
    return ESP_OK;
}

static esp_err_t mock_led_strip_clear(led_strip_t *strip, uint32_t timeout_ms) {
    (void)strip; (void)timeout_ms;
    return ESP_OK;
}

static esp_err_t mock_led_strip_del(led_strip_t *strip) {
    (void)strip;
    return ESP_OK;
}

static led_strip_t mock_strip = {
    .set_pixel = mock_led_strip_set_pixel,
    .refresh = mock_led_strip_refresh,
    .clear = mock_led_strip_clear,
    .del = mock_led_strip_del
};

led_strip_t *led_strip_new_rmt_ws2812(const led_strip_config_t *config) {
    (void)config;
    return &mock_strip;
}

// LED strip mock implementation (new device API)
esp_err_t led_strip_new_rmt_device(const led_strip_config_t *config, const led_strip_rmt_config_t *rmt_config, led_strip_handle_t *ret_strip) {
    (void)config; (void)rmt_config;
    if (ret_strip) *ret_strip = (led_strip_handle_t)1;
    return ESP_OK;
}

esp_err_t led_strip_clear(led_strip_handle_t strip) { (void)strip; return ESP_OK; }

esp_err_t led_strip_set_pixel(led_strip_handle_t strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue) {
    (void)strip; (void)index; (void)red; (void)green; (void)blue;
    return ESP_OK;
}

esp_err_t led_strip_refresh(led_strip_handle_t strip) { (void)strip; return ESP_OK; }

esp_err_t led_strip_del(led_strip_handle_t strip) { (void)strip; return ESP_OK; }

// RMT
esp_err_t rmt_config(const rmt_config_t *config) { (void)config; return ESP_OK; }
esp_err_t rmt_driver_install(rmt_channel_t channel, size_t rx_buf_size, int intr_alloc_flags) {
    (void)channel; (void)rx_buf_size; (void)intr_alloc_flags;
    return ESP_OK;
}

// Sleep
esp_err_t esp_sleep_enable_timer_wakeup(uint64_t time_in_us) { (void)time_in_us; return ESP_OK; }
esp_err_t esp_sleep_enable_ext0_wakeup(int gpio_num, int level) { (void)gpio_num; (void)level; return ESP_OK; }
esp_err_t esp_sleep_enable_ext1_wakeup(uint64_t mask, int mode) { (void)mask; (void)mode; return ESP_OK; }
esp_err_t esp_sleep_enable_gpio_wakeup(void) { return ESP_OK; }
void esp_deep_sleep_start(void) { while(1); }
int esp_light_sleep_start(void) { return 0; }
int esp_sleep_get_wakeup_cause(void) { return 0; }

// SNTP
void esp_sntp_init(void) {}
void esp_sntp_stop(void) {}
bool esp_sntp_enabled(void) { return false; }
void esp_sntp_setoperatingmode(int operating_mode) { (void)operating_mode; }
void esp_sntp_setservername(uint8_t idx, const char *server_name) { (void)idx; (void)server_name; }
void esp_sntp_set_time_sync_notification_cb(void (*callback)(struct timeval *tv)) { (void)callback; }
void esp_sntp_set_sync_mode(int sync_mode) { (void)sync_mode; }

// SPI additional functions
int spi_bus_remove_device(spi_device_handle_t handle) { (void)handle; return 0; }
int spi_bus_free(spi_host_device_t host) { (void)host; return 0; }

// Heap caps malloc functions
void *heap_caps_malloc(size_t size, uint32_t caps) { (void)caps; return malloc(size); }
void heap_caps_free(void *ptr) { free(ptr); }
void *heap_caps_realloc(void *ptr, size_t size, uint32_t caps) { (void)caps; return realloc(ptr, size); }
void *heap_caps_calloc(size_t n, size_t size, uint32_t caps) { (void)caps; return calloc(n, size); }

// Timer
int64_t esp_timer_get_time(void) { return 0; }

size_t heap_caps_get_total_size(uint32_t caps) { (void)caps; return 100000; }
size_t heap_caps_get_free_size(uint32_t caps) { (void)caps; return 50000; }
uint32_t esp_get_minimum_free_heap_size(void) { return 50000; }

// LwIP / Ping / WireGuard stubs
const char *ipaddr_ntoa(const ip_addr_t *ipaddr) { (void)ipaddr; return "127.0.0.1"; }
esp_err_t esp_ping_get_profile(esp_ping_handle_t hdl, esp_ping_profile_t profile_type, void *out_val, uint32_t val_len) { (void)hdl; (void)profile_type; (void)out_val; (void)val_len; return ESP_OK; }
esp_err_t esp_ping_new_session(const esp_ping_config_t *config, const esp_ping_callbacks_t *cbs, esp_ping_handle_t *ping_hdl) { (void)config; (void)cbs; if(ping_hdl) *ping_hdl = (esp_ping_handle_t)1; return ESP_OK; }
esp_err_t esp_ping_start(esp_ping_handle_t ping_hdl) { (void)ping_hdl; return ESP_OK; }
esp_err_t esp_wireguard_init(const wireguard_config_t *config, wireguard_ctx_t *ctx) { (void)config; (void)ctx; return ESP_OK; }
esp_err_t esp_wireguard_connect(wireguard_ctx_t *ctx) { (void)ctx; return ESP_OK; }
esp_err_t esp_wireguard_set_default(wireguard_ctx_t *ctx) { (void)ctx; return ESP_OK; }
esp_err_t esp_wireguardif_peer_is_up(wireguard_ctx_t *ctx) { (void)ctx; return ESP_OK; }

int lwip_getaddrinfo(const char *nodename,
       const char *servname,
       const struct addrinfo *hints,
       struct addrinfo **res)
{
    (void)nodename; (void)servname; (void)hints;
    if (!res) return -1;

    struct addrinfo *ai = (struct addrinfo *)malloc(sizeof(struct addrinfo));
    if (!ai) return -1;
    memset(ai, 0, sizeof(struct addrinfo));

    struct sockaddr_in *sa = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    if (!sa) {
        free(ai);
        return -1;
    }
    memset(sa, 0, sizeof(struct sockaddr_in));
    sa->sin_family = AF_INET;
    sa->sin_addr.s_addr = 0x0100007f; // 127.0.0.1

    ai->ai_family = AF_INET;
    ai->ai_addr = (struct sockaddr *)sa;
    ai->ai_addrlen = sizeof(struct sockaddr_in);

    *res = ai;
    return 0;
}

void lwip_freeaddrinfo(struct addrinfo *ai)
{
    if (ai) {
        if (ai->ai_addr) {
            free(ai->ai_addr);
        }
        free(ai);
    }
}

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"

int gpio_config(const gpio_config_t* config) { (void)config; return 0; }

int spi_bus_initialize(spi_host_device_t host, const spi_bus_config_t* bus_config, int dma_chan) {
    (void)host; (void)bus_config; (void)dma_chan;
    return 0;
}

int spi_bus_add_device(spi_host_device_t host, const spi_device_interface_config_t* dev_config, spi_device_handle_t* handle) {
    (void)host; (void)dev_config;
    if (handle) *handle = (spi_device_handle_t)1;
    return 0;
}

int spi_device_transmit(spi_device_handle_t handle, spi_transaction_t* trans_desc) {
    (void)handle; (void)trans_desc;
    return 0;
}

i2c_cmd_handle_t i2c_cmd_link_create(void) { return (i2c_cmd_handle_t)1; }
void i2c_cmd_link_delete(i2c_cmd_handle_t cmd) { (void)cmd; }
esp_err_t i2c_master_start(i2c_cmd_handle_t cmd) { (void)cmd; return ESP_OK; }
esp_err_t i2c_master_stop(i2c_cmd_handle_t cmd) { (void)cmd; return ESP_OK; }
esp_err_t i2c_master_write_byte(i2c_cmd_handle_t cmd, uint8_t data, bool ack_en) { (void)cmd; (void)data; (void)ack_en; return ESP_OK; }
esp_err_t i2c_master_write(i2c_cmd_handle_t cmd, const uint8_t *data, size_t data_len, bool ack_en) { (void)cmd; (void)data; (void)data_len; (void)ack_en; return ESP_OK; }
esp_err_t i2c_master_read_byte(i2c_cmd_handle_t cmd, uint8_t *data, int ack_val) { (void)cmd; if(data) *data = 0; (void)ack_val; return ESP_OK; }
esp_err_t i2c_master_read(i2c_cmd_handle_t cmd, uint8_t *data, size_t data_len, int ack_val) { (void)cmd; (void)data; (void)data_len; (void)ack_val; return ESP_OK; }
esp_err_t i2c_master_cmd_begin(i2c_port_t i2c_num, i2c_cmd_handle_t cmd, TickType_t ticks_to_wait) { (void)i2c_num; (void)cmd; (void)ticks_to_wait; return ESP_OK; }
esp_err_t i2c_param_config(i2c_port_t i2c_num, const i2c_config_t *i2c_conf) { (void)i2c_num; (void)i2c_conf; return ESP_OK; }

#include "lwip/sockets.h"

int socket(int domain, int type, int protocol) { (void)domain; (void)type; (void)protocol; return 1; }

int close(int s) { (void)s; return 0; }
int shutdown(int s, int how) { (void)s; (void)how; return 0; }
int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen) {
    (void)s; (void)level; (void)optname; (void)optval; (void)optlen;
    return 0;
}
int listen(int s, int backlog) { (void)s; (void)backlog; return 0; }

#ifdef _WIN32

int bind(int s, const struct sockaddr *name, socklen_t namelen) { (void)s; (void)name; (void)namelen; return 0; }
int connect(int s, const struct sockaddr *name, socklen_t namelen) { (void)s; (void)name; (void)namelen; return 0; }
int sendto(int s, const void *dataptr, size_t size, int flags, const struct sockaddr *to, socklen_t tolen) {
    (void)s; (void)dataptr; (void)size; (void)flags; (void)to; (void)tolen;
    return 0;
}
int recvfrom(int s, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen) {
    (void)s; (void)mem; (void)len; (void)flags; (void)from; (void)fromlen;
    return 0;
}
int accept(int s, struct sockaddr *addr, socklen_t *addrlen) { (void)s; (void)addr; (void)addrlen; return 1; }

int select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout) {
    (void)maxfdp1; (void)readset; (void)writeset; (void)exceptset; (void)timeout;
    return 0;
}

#endif

const char *inet_ntoa_r_in(struct in_addr addr, char *buf, int buflen) {
    (void)addr;
    if (buf && buflen > 0) {
        strncpy(buf, "127.0.0.1", buflen);
        buf[buflen - 1] = '\0';
    }
    return buf;
}
const char *inet_ntoa_r_u32(uint32_t addr, char *buf, int buflen) {
    (void)addr;
    if (buf && buflen > 0) {
        strncpy(buf, "127.0.0.1", buflen);
        buf[buflen - 1] = '\0';
    }
    return buf;
}
const char *inet6_ntoa_r(struct in6_addr addr, char *buf, int buflen) {
    (void)addr;
    if (buf && buflen > 0) {
        strncpy(buf, "::1", buflen);
        buf[buflen - 1] = '\0';
    }
    return buf;
}

void dns_tmr(void) {}
esp_err_t esp_https_ota(const void *config) { (void)config; return ESP_OK; }
const char _binary_ca_cert_pem_start[] = "";
int esp_ota_set_boot_partition(const esp_partition_t* partition) { (void)partition; return ESP_OK; }






