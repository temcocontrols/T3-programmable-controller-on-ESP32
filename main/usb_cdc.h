#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief  初始化 USB CDC-ACM 设备（虚拟串口）。
 *         必须在使用 usb_cdc_read / usb_cdc_write 之前调用。
 *         需要在 sdkconfig 中启用：
 *           CONFIG_TINYUSB_ENABLED=y
 *           CONFIG_TINYUSB_CDC_ENABLED=y
 */
void usb_cdc_init(void);

/**
 * @brief  向 USB CDC 发送数据。
 * @return 实际发送字节数，出错返回 -1。
 */
int usb_cdc_write(const uint8_t *buf, size_t len);

/**
 * @brief  从 USB CDC 读取数据，带超时。
 * @param  timeout_ms  等待超时（毫秒），0 表示不等待。
 * @return 实际读取字节数（超时返回 0）。
 */
int usb_cdc_read(uint8_t *buf, size_t max_len, uint32_t timeout_ms);

/**
 * @brief  返回 USB 主机是否已连接。
 */
bool usb_cdc_connected(void);
