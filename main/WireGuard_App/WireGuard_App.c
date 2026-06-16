/**
 * @file WireGuard_App.c
 * @brief WireGuard gateway module implementation for Temco application.
 *
 * This module provides WiFi initialization, time synchronization (SNTP),
 * WireGuard setup, and connectivity testing for the WireGuard Gateway device.
 */

#include "WireGuard_App.h"

#include <string.h>
#include <stdbool.h>
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
#include "flash.h"
#include "modbus.h"
#include "sntp_app.h"
#include "user_data.h"

extern  EventGroupHandle_t s_wifi_event_group;

static const char *TAG = "wireguard_app";
static wireguard_config_t wg_config = ESP_WIREGUARD_CONFIG_DEFAULT();

/** Event bit for WiFi connected with IP. */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static bool wireguard_buffer_has_text(uint8_t *buffer, size_t size)
{
    if (buffer == NULL || size == 0)
        return false;

    buffer[size - 1] = '\0';

    return buffer[0] != '\0';
}

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
 * @brief Read WireGuard data by block for Modbus register access
 * Supports reading both single bytes (IPs) and word values (port)
 */
uint16_t wireguard_read_by_block(uint16_t addr)
{
	uint8_t item = 0;

	if (addr == MODBUS_WIREGUARD_ENABLE)
    {
		return wireguard_point.reg.wireguard_enable;
	}
	else if ((addr >= MODBUS_WIREGUARD_PRIVATE_KEY_START) &&
		(addr <= MODBUS_WIREGUARD_PRIVATE_KEY_END))
    {
		item = addr - MODBUS_WIREGUARD_PRIVATE_KEY_START;
		if (item * 2 + 1 < sizeof(wireguard_point.reg.wireguard_private_key))
        {
			return (wireguard_point.reg.wireguard_private_key[item * 2] << 8) |
				wireguard_point.reg.wireguard_private_key[item * 2 + 1];
		}
	}
	else if ((addr >= MODBUS_WIREGUARD_PEER_PUBLIC_KEY_START) &&
		(addr <= MODBUS_WIREGUARD_PEER_PUBLIC_KEY_END))
    {
		item = addr - MODBUS_WIREGUARD_PEER_PUBLIC_KEY_START;
		if (item * 2 + 1 < sizeof(wireguard_point.reg.wireguard_peer_public_key))
        {
			return (wireguard_point.reg.wireguard_peer_public_key[item * 2] << 8) |
				wireguard_point.reg.wireguard_peer_public_key[item * 2 + 1];
		}
	}
	else if ((addr >= MODBUS_WIREGUARD_PRESHARED_KEY_START) &&
		(addr <= MODBUS_WIREGUARD_PRESHARED_KEY_END))
    {
		item = addr - MODBUS_WIREGUARD_PRESHARED_KEY_START;
		if (item * 2 + 1 < sizeof(wireguard_point.reg.wireguard_preshared_key))
        {
			return (wireguard_point.reg.wireguard_preshared_key[item * 2] << 8) |
				wireguard_point.reg.wireguard_preshared_key[item * 2 + 1];
		}
	}
	else if ((addr >= MODBUS_WIREGUARD_LOCAL_IP1) &&
		(addr <= MODBUS_WIREGUARD_LOCAL_IP4))
    {
		item = addr - MODBUS_WIREGUARD_LOCAL_IP1;
		return wireguard_point.reg.wireguard_local_ip[item];
	}
	else if (addr == MODBUS_WIREGUARD_PORT)
    {
		return wireguard_point.reg.wireguard_port;
	}
	else if ((addr >= MODBUS_WIREGUARD_PEER_IP1) &&
		(addr <= MODBUS_WIREGUARD_PEER_IP4))
    {
		item = addr - MODBUS_WIREGUARD_PEER_IP1;
		return wireguard_point.reg.wireguard_peer_ip[item];
	}

	return 0;
}

static bool wireguard_write_register(uint16_t addr, uint16_t value_word)
{
	uint8_t item = 0;
	uint8_t value = value_word & 0xff;

	if (addr == MODBUS_WIREGUARD_ENABLE)
    {
		wireguard_point.reg.wireguard_enable = value ? 1 : 0;
		return true;
	}
	else if ((addr >= MODBUS_WIREGUARD_PRIVATE_KEY_START) &&
		(addr <= MODBUS_WIREGUARD_PRIVATE_KEY_END))
        {
		item = addr - MODBUS_WIREGUARD_PRIVATE_KEY_START;
		if (item * 2 + 1 < sizeof(wireguard_point.reg.wireguard_private_key))
        {
			wireguard_point.reg.wireguard_private_key[item * 2] = value_word >> 8;
			wireguard_point.reg.wireguard_private_key[item * 2 + 1] = value;
			return true;
		}
	}
	else if ((addr >= MODBUS_WIREGUARD_PEER_PUBLIC_KEY_START) &&
		(addr <= MODBUS_WIREGUARD_PEER_PUBLIC_KEY_END))
    {
		item = addr - MODBUS_WIREGUARD_PEER_PUBLIC_KEY_START;
		if (item * 2 + 1 < sizeof(wireguard_point.reg.wireguard_peer_public_key))
        {
			wireguard_point.reg.wireguard_peer_public_key[item * 2] = value_word >> 8;
			wireguard_point.reg.wireguard_peer_public_key[item * 2 + 1] = value;
			return true;
		}
	}
	else if ((addr >= MODBUS_WIREGUARD_PRESHARED_KEY_START) &&
		(addr <= MODBUS_WIREGUARD_PRESHARED_KEY_END))
    {
		item = addr - MODBUS_WIREGUARD_PRESHARED_KEY_START;
		if (item * 2 + 1 < sizeof(wireguard_point.reg.wireguard_preshared_key))
        {
			wireguard_point.reg.wireguard_preshared_key[item * 2] = value_word >> 8;
			wireguard_point.reg.wireguard_preshared_key[item * 2 + 1] = value;
			return true;
		}
	}
	else if ((addr >= MODBUS_WIREGUARD_LOCAL_IP1) &&
		(addr <= MODBUS_WIREGUARD_LOCAL_IP4))
    {
		item = addr - MODBUS_WIREGUARD_LOCAL_IP1;
		wireguard_point.reg.wireguard_local_ip[item] = value;
		return true;
	}
	else if (addr == MODBUS_WIREGUARD_PORT)
    {
		wireguard_point.reg.wireguard_port = value_word;
		return true;
	}
	else if ((addr >= MODBUS_WIREGUARD_PEER_IP1) &&
		(addr <= MODBUS_WIREGUARD_PEER_IP4))
    {
		item = addr - MODBUS_WIREGUARD_PEER_IP1;
		wireguard_point.reg.wireguard_peer_ip[item] = value;
		return true;
	}

	return false;
}

/**
 * @brief Write WireGuard data by block from Modbus register
 * Supports both single register writes and multiple register writes.
 * Automatically saves to flash after valid data is written.
 */
void wireguard_write_by_block(uint16_t addr, uint8_t HeadLen, uint8_t *pData)
{
	bool changed = false;

	if (pData == NULL)
		return;

	if (pData[HeadLen + 1] == MULTIPLE_WRITE_VARIABLES)
    {
		uint16_t quantity = ((uint16_t)pData[HeadLen + 4] << 8) | pData[HeadLen + 5];
		uint8_t byte_count = pData[HeadLen + 6];

		for (uint16_t index = 0; index < quantity; index++)
        {
			uint16_t data_offset = HeadLen + 7 + (index * 2);

			if ((index * 2 + 1) >= byte_count)
				break;

			uint16_t value_word = ((uint16_t)pData[data_offset] << 8) |
				pData[data_offset + 1];

			changed |= wireguard_write_register(addr + index, value_word);
		}
	}
	else
    {
		uint16_t value_word = ((uint16_t)pData[HeadLen + 4] << 8) |
			pData[HeadLen + 5];

		changed = wireguard_write_register(addr, value_word);
	}

	if (changed)
		save_wireguard_config_to_flash();
}

static bool wireguard_config_valid(void)
{
    if (wireguard_point.reg.wireguard_enable == 0)
        return false;

    if (!wireguard_buffer_has_text(wireguard_point.reg.wireguard_private_key,
        sizeof(wireguard_point.reg.wireguard_private_key)))
        return false;

    if (!wireguard_buffer_has_text(wireguard_point.reg.wireguard_peer_public_key,
        sizeof(wireguard_point.reg.wireguard_peer_public_key)))
        return false;

    if (wireguard_point.reg.wireguard_peer_ip[0] == 0 &&
        wireguard_point.reg.wireguard_peer_ip[1] == 0 &&
        wireguard_point.reg.wireguard_peer_ip[2] == 0 &&
        wireguard_point.reg.wireguard_peer_ip[3] == 0)
        return false;

    if (wireguard_point.reg.wireguard_local_ip[0] == 0 &&
        wireguard_point.reg.wireguard_local_ip[1] == 0 &&
        wireguard_point.reg.wireguard_local_ip[2] == 0 &&
        wireguard_point.reg.wireguard_local_ip[3] == 0)
        return false;

    return true;
}

static void wireguard_app_load_config(void)
{
    static char local_ip[16];
    static char peer_ip[16];

    /* Wait until valid configuration is available */
    while (!wireguard_config_valid())
    {
        ESP_LOGW(TAG,
            "WireGuard configuration not available. Waiting for Modbus update...");

        vTaskDelay(pdMS_TO_TICKS(5000));

        /* Reload latest data from flash */
        load_wireguard_config_from_flash();
    }

    memset(&wg_config, 0, sizeof(wg_config));

    /* Keys */
    wg_config.private_key =
        (const char *)wireguard_point.reg.wireguard_private_key;

    wg_config.public_key =
        (const char *)wireguard_point.reg.wireguard_peer_public_key;

    wireguard_buffer_has_text(wireguard_point.reg.wireguard_preshared_key,
        sizeof(wireguard_point.reg.wireguard_preshared_key));

    if (strlen((char *)wireguard_point.reg.wireguard_preshared_key))
    {
        wg_config.preshared_key =
            (const char *)wireguard_point.reg.wireguard_preshared_key;
    }
    else
    {
        wg_config.preshared_key = NULL;
    }

    /* Local VPN IP */
    snprintf(local_ip,
             sizeof(local_ip),
             "%u.%u.%u.%u",
             wireguard_point.reg.wireguard_local_ip[0],
             wireguard_point.reg.wireguard_local_ip[1],
             wireguard_point.reg.wireguard_local_ip[2],
             wireguard_point.reg.wireguard_local_ip[3]);

    wg_config.allowed_ip = local_ip;

    /* Peer Endpoint */
    snprintf(peer_ip,
             sizeof(peer_ip),
             "%u.%u.%u.%u",
             wireguard_point.reg.wireguard_peer_ip[0],
             wireguard_point.reg.wireguard_peer_ip[1],
             wireguard_point.reg.wireguard_peer_ip[2],
             wireguard_point.reg.wireguard_peer_ip[3]);

    wg_config.endpoint = peer_ip;

    /* Ports */
    wg_config.listen_port = wireguard_point.reg.wireguard_port;
    wg_config.port        = wireguard_point.reg.wireguard_port;

    /* Defaults */
    wg_config.allowed_ip_mask = "255.255.255.0";
    wg_config.persistent_keepalive = 25;

    ESP_LOGI(TAG, "WireGuard configuration loaded");
    ESP_LOGI(TAG, "VPN IP      : %s", wg_config.allowed_ip);
    ESP_LOGI(TAG, "Peer IP     : %s", wg_config.endpoint);
    ESP_LOGI(TAG, "Listen Port : %d", wg_config.listen_port);
}

esp_err_t wireguard_app_setup(wireguard_ctx_t *ctx)
{
    esp_err_t err;

    ESP_LOGI(TAG, "Initializing WireGuard.");

    /* Load configuration from flash and wait until it's valid */
    wireguard_app_load_config();

    err = esp_wireguard_init(&wg_config, ctx);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wireguard_init: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Connecting to WireGuard peer.");
    err = esp_wireguard_connect(ctx);
    if (err != ESP_OK)
    {
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

    if (lwip_getaddrinfo(WG_PING_ADDRESS, NULL, &hint, &res) != 0)
    {
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
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ping_new_session failed: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_ping_start(ping);
    if (err != ESP_OK)
    {
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

    if (err != ESP_OK)
    {
    	ESP_LOGE(TAG, "WireGuard setup failed: %s", esp_err_to_name(err));
    	vTaskDelete(NULL);
    	return;
    }

    /* Wait for WireGuard peer to be up */
    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if (wireguard_app_peer_is_up(&wg_ctx) == ESP_OK)
        {
            ESP_LOGI(TAG, "WireGuard peer is up");
            break;
        }
        ESP_LOGI(TAG, "Waiting for WireGuard peer...");
    }

    /* Start ping to verify WireGuard connectivity */
    err = wireguard_app_start_ping();
    if (err != ESP_OK)
    {
        ESP_LOGW("wireguard_gateway_task", "Ping initialization failed: %s", esp_err_to_name(err));
    }

    /* Set WireGuard as default route */
    err = wireguard_app_set_default(&wg_ctx);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to set WireGuard as default: %s", esp_err_to_name(err));
    }

    ESP_LOGI("wireguard_gateway_task", "WireGuard Gateway ready");

    /* Keep the task alive */
    while (1)
    {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

