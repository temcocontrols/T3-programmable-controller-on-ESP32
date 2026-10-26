/**
 * @file Mqtt_Handler.h
 * @brief MQTT client handler with configurable broker/topics (Hub / TSTAT).
 */

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void Mqtt_Deinit(void);
void Mqtt_Handler_Init(void);
int Mqtt_Handler_Publish(const char *topic, const char *data, int qos, bool retain);

struct BACnet_COV_Data;
bool Mqtt_Handler_Send_COV(const struct BACnet_COV_Data *cov_data);

/** Apply mqtt_point after BACnet/Modbus write (restart client if needed). */
void Mqtt_Handler_Apply_Config(void);

uint16_t mqtt_config_read_by_block(uint16_t addr);
void mqtt_config_write_by_block(uint16_t addr, uint8_t HeadLen, uint8_t *pData);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_HANDLER_H */
