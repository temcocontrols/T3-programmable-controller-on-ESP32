/**
 * @file Mqtt_Handler.h
 * @brief MQTT Client handler for HiveMQ broker integration and future trend log upload support.
 *
 * This component handles MQTT communication (connecting to HiveMQ, subscribing to topics,
 * and publishing messages) on the ESP32-S3 platform using ESP-IDF v5 (esp-mqtt).
 * It also provides future support for serializing and sending trend log data to the cloud.
 *
 * @author Antigravity AI Coding Assistant
 * @date June 2026
 */

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

#define BACNET_SUB_COV 1
#define ALL_COV        0

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief De-Initializes the MQTT Client connection and configurations.
 *
 * Disconnects the MQTT client,
 * and stops the MQTT service.
 * Deletes Mqtt handler task.
 */
void Mqtt_Deinit(void);

/**
 * @brief Initializes the MQTT Client connection and configurations.
 *
 * Configures the MQTT client to connect to the HiveMQ broker, registers event handlers,
 * and starts the MQTT service. It automatically handles reconnection and initiates
 * subscriptions after a successful connection.
 */
void Mqtt_Handler_Init(void);

/**
 * @brief Publishes a payload to a specific MQTT topic.
 *
 * Useful for general purpose data publishing, diagnostic reports, and test messages.
 *
 * @param[in] topic   The MQTT topic string.
 * @param[in] data    The payload string to be published.
 * @param[in] qos     The Quality of Service (QoS) level (0, 1, or 2).
 * @param[in] retain  True if the message should be retained by the broker, false otherwise.
 * @return int        The message ID on success, or a negative value indicating an error.
 */
int Mqtt_Handler_Publish(const char *topic, const char *data, int qos, bool retain);

struct BACnet_COV_Data;
/**
 * @brief Serializes a BACnet COV data notification and publishes it to the cloud via MQTT.
 *
 * @param[in] cov_data  The pointer to the BACnet COV data structure.
 * @return true         If successful.
 */
bool Mqtt_Handler_Send_COV(const struct BACnet_COV_Data *cov_data);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_HANDLER_H */
