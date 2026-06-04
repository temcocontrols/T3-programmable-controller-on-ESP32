
/**
 * @file WireGuard_App.h
 * @brief WireGuard gateway module for Temco application.
 *
 * This module provides WireGuard initialization, time synchronization,
 * and networking helpers for the WireGuard Gateway device type.
 *
 * Configuration notes:
 * - All WireGuard configuration (keys, addresses, ports, etc.) can be configured
 *   via Modbus registers (see define.h: Modbus.wireguard_* fields)
 * - Configuration is also stored in flash memory (see flash.h: FLASH_WIREGUARD_*)
 * - If Modbus values are not set, the #define defaults below are used as fallback
 * - For Modbus register addresses, see modbus.h (MODBUS_WIREGUARD_* enums)
 */

#ifndef WIREGUARD_APP_H
#define WIREGUARD_APP_H

#include <esp_err.h>
#include <esp_wireguard.h>

/**
 * @brief WiFi SSID configuration.
 */
#define WG_WIFI_SSID               "4G-UFI"

/**
 * @brief WiFi password configuration.
 */
#define WG_WIFI_PASSWORD           "12345678"

/**
 * @brief WiFi maximum retry attempts.
 */
#define WG_WIFI_MAXIMUM_RETRY      5

/**
 * @brief WireGuard private key for this device.
 */
#define WG_PRIVATE_KEY             "KGktoZPaxM0edqvBXXU/tXKtMb3PA/IC3mf9BktAvV4="

/**
 * @brief WireGuard public key of the peer.
 */
#define WG_PEER_PUBLIC_KEY         "n1SJMzFgWJgb+Xvy7Skh5uVrL7PKw37Q3IVVPnFfhDg="

/**
 * @brief Optional WireGuard preshared key.
 */
#define WG_PRESHARED_KEY           "dVUoxq7kZ1WYCs8AsL6NSp0YkHMtkfWLgW1q0rrCTGw="

/**
 * @brief Local WireGuard address for the ESP device.
 */
#define WG_LOCAL_IP_ADDRESS        "10.0.0.2"

/**
 * @brief Local WireGuard netmask.
 */
#define WG_LOCAL_IP_NETMASK        "255.255.255.0"

/**
 * @brief Local WireGuard UDP listening port.
 */
#define WG_LOCAL_PORT              51821

/**
 * @brief Remote WireGuard peer endpoint address.
 */
// #define WG_PEER_ADDRESS            "223.236.26.80"
#define WG_PEER_ADDRESS            "152.59.50.144"
// #define WG_PEER_ADDRESS            "192.168.31.135"

/**
 * @brief Remote WireGuard peer UDP port.
 */
#define WG_PEER_PORT               51820

/**
 * @brief WireGuard persistent keepalive interval in seconds.
 */
#define WG_PERSISTENT_KEEP_ALIVE   25

/**
 * @brief Ping target used for WireGuard connectivity test.
 */
#define WG_PING_ADDRESS            "10.0.0.1"

/**
 * @brief Time zone for system clock (EST5EDT).
 */
#define WG_SYSTEM_TZ               "EST5EDT,M3.2.0/2,M11.1.0"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check whether the WireGuard peer is currently up.
 *
 * @param ctx WireGuard context instance.
 * @return ESP_OK if peer is up, otherwise error code.
 */
esp_err_t wireguard_app_peer_is_up(wireguard_ctx_t *ctx);

/**
 * @brief Set the WireGuard interface as the default route.
 *
 * @param ctx WireGuard context instance.
 * @return ESP_OK on success, otherwise error code.
 */
esp_err_t wireguard_app_set_default(wireguard_ctx_t *ctx);

/**
 * @brief Start a ping session against the configured WireGuard target.
 *
 * @return ESP_OK on success, otherwise error code.
 */
esp_err_t wireguard_app_start_ping(void);

/**
 * @brief WireGuard Gateway main task.
 *
 * This task initializes WiFi, synchronizes time, sets up the WireGuard interface,
 * waits for the peer to be up, starts a ping session to verify connectivity,
 */
void wireguard_gateway_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* WIREGUARD_APP_H */