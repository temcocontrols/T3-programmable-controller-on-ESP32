#ifndef MOCK_USB_SERIAL_JTAG_H
#define MOCK_USB_SERIAL_JTAG_H

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int tx_buffer_size;
    int rx_buffer_size;
} usb_serial_jtag_driver_config_t;

#define USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT() {256, 256}

static inline esp_err_t usb_serial_jtag_driver_install(const usb_serial_jtag_driver_config_t *config)
{
    (void)config;
    return ESP_OK;
}

static inline int usb_serial_jtag_read_bytes(void *buf, uint32_t length, uint32_t ticks_to_wait)
{
    (void)buf;
    (void)length;
    (void)ticks_to_wait;
    return 0;
}

static inline int usb_serial_jtag_write_bytes(const void *src, size_t size, uint32_t ticks_to_wait)
{
    (void)src;
    (void)ticks_to_wait;
    return (int)size;
}

#ifdef __cplusplus
}
#endif

#endif
