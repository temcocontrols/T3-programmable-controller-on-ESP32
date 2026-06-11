
/**
 * @file WireGuard_App.h
 * @brief WireGuard gateway module for Temco application.
 *
 * This module provides WireGuard initialization, time synchronization,
 * and networking helpers for the WireGuard Gateway device type.
 *
 * Configuration notes:
 * - All WireGuard configuration (keys, addresses, ports, etc.) can be configured
 *   via Modbus registers (see define.h: wireguard_point.reg.wireguard_* fields)
 * - Configuration is also stored in flash memory (see flash.h: FLASH_WIREGUARD_*)
 * - If Modbus values are not set, the #define defaults below are used as fallback
 * - For Modbus register addresses, see modbus.h (MODBUS_WIREGUARD_* enums)
 */

#ifndef WIREGUARD_APP_H
#define WIREGUARD_APP_H

#include <esp_err.h>
#include <esp_wireguard.h>

/**
 * @brief WireGuard persistent keepalive interval in seconds.
 */
#define WG_PERSISTENT_KEEP_ALIVE   25

/**
 * @brief Ping target used for WireGuard connectivity test.
 */
#define WG_PING_ADDRESS            "10.0.0.1"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief read WireGuard configuration data by block for Modbus register access
 *
 * @param addr Modbus register address corresponding to WireGuard configuration item
 * @return 16-bit value read from the specified WireGuard configuration item
 */
uint16_t wireguard_read_by_block(uint16_t addr);

/**
 * @brief write WireGuard configuration data by block from Modbus register
 *
 * @param addr Modbus register address corresponding to WireGuard configuration item
 * @param HeadLen Length of the Modbus header (used to calculate data offset)
 * @param pData Pointer to the Modbus data buffer containing the value to write
 */
void wireguard_write_by_block(uint16_t addr, uint8_t HeadLen, uint8_t *pData);

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