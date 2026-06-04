/**
 * @file WireGuard_App.c
 * @brief WireGuard gateway module implementation for Temco application.
 *
 * This module provides WiFi initialization, time synchronization (SNTP),
 * WireGuard setup, and connectivity testing for the WireGuard Gateway device.
 */

#include "WireGuard_App.h"

#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <lwip/netdb.h>
#include <lwip/inet.h>
#include <ping/ping_sock.h>
#include "define.h"
#include "sntp_app.h"

extern  EventGroupHandle_t s_wifi_event_group;

static const char *TAG = "wireguard_app";
static wireguard_config_t wg_config = ESP_WIREGUARD_CONFIG_DEFAULT();

/** Event bit for WiFi connected with IP. */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static void wireguard_app_on_ping_success(esp_ping_handle_t hdl, void *args)
{
    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_time;
    uint32_t recv_len;
    ip_addr_t target_addr;

    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));

    ESP_LOGI(TAG, "%" PRIu32 " bytes from %s icmp_seq=%" PRIu16 " ttl=%" PRIi8 " time=%" PRIu32 " ms",
             recv_len, ipaddr_ntoa(&target_addr), seqno, ttl, elapsed_time);
}

static void wireguard_app_on_ping_timeout(esp_ping_handle_t hdl, void *args)
{
    uint16_t seqno;
    ip_addr_t target_addr;

    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));

    ESP_LOGI(TAG, "From %s icmp_seq=%" PRIu16 " timeout", ipaddr_ntoa(&target_addr), seqno);
}

static void wireguard_app_on_ping_end(esp_ping_handle_t hdl, void *args)
{
    uint32_t transmitted;
    uint32_t received;
    uint32_t total_time_ms;

    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));

    ESP_LOGI(TAG, "%" PRIu32 " packets transmitted, %" PRIu32 " received, time %" PRIu32 "ms",
             transmitted, received, total_time_ms);
}

/**
 * @brief Load WireGuard configuration from Modbus data.
 * Uses defaults from #defines if Modbus values are not set.
 */
static void wireguard_app_load_config_from_modbus(void)
{
    /* Load private key from Modbus, use default if not set */
    if (Modbus.wireguard_private_key[0] != 0) {
        memcpy((void *)wg_config.private_key, Modbus.wireguard_private_key,
               sizeof(Modbus.wireguard_private_key));
    }

    /* Load peer public key from Modbus, use default if not set */
    if (Modbus.wireguard_peer_public_key[0] != 0) {
        memcpy((void *)wg_config.public_key, Modbus.wireguard_peer_public_key,
               sizeof(Modbus.wireguard_peer_public_key));
    }

    /* Load preshared key from Modbus if set */
    if (Modbus.wireguard_preshared_key[0] != 0) {
        wg_config.preshared_key = (const char *)Modbus.wireguard_preshared_key;
    } else {
        wg_config.preshared_key = NULL;
    }

    /* Load local IP from Modbus, use default if not set */
    if (Modbus.wireguard_local_ip[0] != 0 || Modbus.wireguard_local_ip[1] != 0 ||
        Modbus.wireguard_local_ip[2] != 0 || Modbus.wireguard_local_ip[3] != 0) {
        static char local_ip_str[16];
        snprintf(local_ip_str, sizeof(local_ip_str), "%d.%d.%d.%d",
                Modbus.wireguard_local_ip[0], Modbus.wireguard_local_ip[1],
                Modbus.wireguard_local_ip[2], Modbus.wireguard_local_ip[3]);
        wg_config.allowed_ip = local_ip_str;
    }

    /* Load local netmask from Modbus, use default if not set */
    if (Modbus.wireguard_local_netmask[0] != 0 || Modbus.wireguard_local_netmask[1] != 0 ||
        Modbus.wireguard_local_netmask[2] != 0 || Modbus.wireguard_local_netmask[3] != 0) {
        static char netmask_str[16];
        snprintf(netmask_str, sizeof(netmask_str), "%d.%d.%d.%d",
                Modbus.wireguard_local_netmask[0], Modbus.wireguard_local_netmask[1],
                Modbus.wireguard_local_netmask[2], Modbus.wireguard_local_netmask[3]);
        wg_config.allowed_ip_mask = netmask_str;
    }

    /* Load local port from Modbus, use default if not set */
    if (Modbus.wireguard_local_port != 0) {
        wg_config.listen_port = Modbus.wireguard_local_port;
    }

    /* Load peer IP from Modbus, use default if not set */
    if (Modbus.wireguard_peer_ip[0] != 0 || Modbus.wireguard_peer_ip[1] != 0 ||
        Modbus.wireguard_peer_ip[2] != 0 || Modbus.wireguard_peer_ip[3] != 0) {
        static char peer_ip_str[16];
        snprintf(peer_ip_str, sizeof(peer_ip_str), "%d.%d.%d.%d",
                Modbus.wireguard_peer_ip[0], Modbus.wireguard_peer_ip[1],
                Modbus.wireguard_peer_ip[2], Modbus.wireguard_peer_ip[3]);
        wg_config.endpoint = peer_ip_str;
    }

    /* Load peer port from Modbus, use default if not set */
    if (Modbus.wireguard_peer_port != 0) {
        wg_config.port = Modbus.wireguard_peer_port;
    }

    /* Load keepalive from Modbus, use default if not set */
    if (Modbus.wireguard_keepalive != 0) {
        wg_config.persistent_keepalive = Modbus.wireguard_keepalive;
    }

    /* Load timezone from Modbus, use default if not set */
    if (Modbus.wireguard_timezone[0] != 0) {
        setenv("TZ", (const char *)Modbus.wireguard_timezone, 1);
    }

    ESP_LOGI(TAG, "WireGuard config loaded from Modbus");
}

esp_err_t wireguard_app_setup(wireguard_ctx_t *ctx)
{
    esp_err_t err;

    ESP_LOGI(TAG, "Initializing WireGuard.");

    /* Initialize with default values */
    wg_config.private_key = WG_PRIVATE_KEY;
    wg_config.listen_port = WG_LOCAL_PORT;
    wg_config.public_key = WG_PEER_PUBLIC_KEY;
    wg_config.preshared_key = (strcmp(WG_PRESHARED_KEY, "") != 0) ? WG_PRESHARED_KEY : NULL;
    wg_config.allowed_ip = WG_LOCAL_IP_ADDRESS;
    wg_config.allowed_ip_mask = WG_LOCAL_IP_NETMASK;
    wg_config.endpoint = WG_PEER_ADDRESS;
    wg_config.port = WG_PEER_PORT;
    wg_config.persistent_keepalive = WG_PERSISTENT_KEEP_ALIVE;

    /* Load configuration from Modbus (overrides defaults) */
    // wireguard_app_load_config_from_modbus();

    err = esp_wireguard_init(&wg_config, ctx);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wireguard_init: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Connecting to WireGuard peer.");
    err = esp_wireguard_connect(ctx);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wireguard_connect: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t wireguard_app_peer_is_up(wireguard_ctx_t *ctx)
{
    return esp_wireguardif_peer_is_up(ctx);
}

esp_err_t wireguard_app_set_default(wireguard_ctx_t *ctx)
{
    return esp_wireguard_set_default(ctx);
}

esp_err_t wireguard_app_start_ping(void)
{
    ip_addr_t target_addr;
    struct addrinfo *res = NULL;
    struct addrinfo hint;
    esp_ping_handle_t ping;
    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    esp_ping_callbacks_t cbs = {
        .on_ping_success = wireguard_app_on_ping_success,
        .on_ping_timeout = wireguard_app_on_ping_timeout,
        .on_ping_end = wireguard_app_on_ping_end,
        .cb_args = NULL,
    };
    esp_err_t err;

    memset(&hint, 0, sizeof(hint));
    memset(&target_addr, 0, sizeof(target_addr));

    if (lwip_getaddrinfo(WG_PING_ADDRESS, NULL, &hint, &res) != 0) {
        ESP_LOGE(TAG, "lwip_getaddrinfo failed for %s", WG_PING_ADDRESS);
        return ESP_FAIL;
    }

    struct in_addr addr4 = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
    inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
    lwip_freeaddrinfo(res);

    ESP_LOGI(TAG, "Ping target: %s", WG_PING_ADDRESS);

    ping_config.target_addr = target_addr;
    ping_config.count = ESP_PING_COUNT_INFINITE;

    err = esp_ping_new_session(&ping_config, &cbs, &ping);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ping_new_session failed: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_ping_start(ping);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ping_start failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

/**
 * @brief WireGuard Gateway task - initializes WireGuard and handles connectivity.
 */
void wireguard_gateway_task(void *pvParameters)
{
    esp_err_t err;
    wireguard_ctx_t wg_ctx = {0};

    ESP_LOGI("wireguard_gateway_task", "Starting WireGuard Gateway initialization...");


    /* Initialize synchronize system time */

	// obtain_time();
    // time(&now);

    // setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    // tzset();
    // localtime_r(&now, &timeinfo);
    // strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    // ESP_LOGI(TAG, "The current date/time in New York is: %s", strftime_buf);

    /* Wait for wifi to be connected before initializing WireGuard */
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if (xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT)
        {
            ESP_LOGI(TAG, "WiFi is connected, proceeding with WireGuard setup.");
            break;
        }
    }

    /* wait for time to be synchronized before initializing WireGuard */
    ESP_LOGI(TAG, "Waiting for time synchronization...");
    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if (SNTPC_GetState() == SNTP_STATE_GET_DONE)
        {
            ESP_LOGI(TAG, "Time synchronized, proceeding with WireGuard setup.");
            break;
        }
    }

    /* Setup WireGuard interface */
    err = wireguard_app_setup(&wg_ctx);

    if (err != ESP_OK) {
    	ESP_LOGE(TAG, "WireGuard setup failed: %s", esp_err_to_name(err));
    	vTaskDelete(NULL);
    	return;
    }

    /* Wait for WireGuard peer to be up */
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if (wireguard_app_peer_is_up(&wg_ctx) == ESP_OK) {
            ESP_LOGI(TAG, "WireGuard peer is up");
            break;
        }
        ESP_LOGI(TAG, "Waiting for WireGuard peer...");
    }

    /* Start ping to verify WireGuard connectivity */
    err = wireguard_app_start_ping();
    if (err != ESP_OK) {
        ESP_LOGW("wireguard_gateway_task", "Ping initialization failed: %s", esp_err_to_name(err));
    }

    /* Set WireGuard as default route */
    err = wireguard_app_set_default(&wg_ctx);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set WireGuard as default: %s", esp_err_to_name(err));
    }

    ESP_LOGI("wireguard_gateway_task", "WireGuard Gateway ready");

    /* Keep the task alive */
    while (1) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

