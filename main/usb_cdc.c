#include "usb_cdc.h"
#include "define.h"

#ifdef USE_USB_CDC_MAIN

#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include <string.h>

static const char *TAG = "USB_CDC";

/* 接收环形缓冲区：1 KB，足以缓冲一帧 Modbus 数据 */
#define USB_CDC_RX_BUF_SIZE 1024
static RingbufHandle_t s_rx_ringbuf = NULL;

/* USB CDC 接收回调（在 TinyUSB 任务上下文中调用） */
static void cdc_rx_callback(int itf, cdcacm_event_t *event)
{
    (void)itf;
    (void)event;
    uint8_t buf[64];
    size_t rx_size = 0;

    esp_err_t ret = tinyusb_cdcacm_read(TINYUSB_CDC_ACM_0, buf, sizeof(buf), &rx_size);
    if (ret == ESP_OK && rx_size > 0 && s_rx_ringbuf) {
        /* 发送到环形缓冲区，不阻塞（从中断/回调调用） */
        xRingbufferSend(s_rx_ringbuf, buf, rx_size, 0);
    }
}

void usb_cdc_init(void)
{
    /* 创建接收缓冲区 */
    s_rx_ringbuf = xRingbufferCreate(USB_CDC_RX_BUF_SIZE, RINGBUF_TYPE_BYTEBUF);
    if (!s_rx_ringbuf) {
        ESP_LOGE(TAG, "Failed to create RX ring buffer");
        return;
    }

    /* 安装 TinyUSB 驱动（使用默认描述符） */
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor        = NULL,
        .string_descriptor        = NULL,
        .string_descriptor_count  = 0,
        .external_phy             = false,
        .configuration_descriptor = NULL,
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    /* 配置 CDC-ACM 端口 */
    const tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev                    = TINYUSB_USBDEV_0,
        .cdc_port                   = TINYUSB_CDC_ACM_0,
        .rx_unread_buf_sz           = 64,
        .callback_rx                = cdc_rx_callback,
        .callback_rx_wanted_char    = NULL,
        .callback_line_state_changed= NULL,
        .callback_line_coding_changed = NULL,
    };
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));

    ESP_LOGI(TAG, "USB CDC-ACM initialized (replaces RS485 Main port)");
}

int usb_cdc_write(const uint8_t *buf, size_t len)
{
    if (!buf || len == 0) return 0;

    esp_err_t ret = tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, buf, len);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "write_queue failed: %s", esp_err_to_name(ret));
        return -1;
    }
    /* 最多等 10 ms 发送完毕 */
    tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, pdMS_TO_TICKS(10));
    return (int)len;
}

int usb_cdc_read(uint8_t *buf, size_t max_len, uint32_t timeout_ms)
{
    if (!s_rx_ringbuf || !buf) return 0;

    size_t received = 0;
    uint8_t *data = xRingbufferReceiveUpTo(s_rx_ringbuf, &received,
                                            pdMS_TO_TICKS(timeout_ms), max_len);
    if (data) {
        memcpy(buf, data, received);
        vRingbufferReturnItem(s_rx_ringbuf, data);
        return (int)received;
    }
    return 0;
}

bool usb_cdc_connected(void)
{
    return tud_cdc_connected();
}

#endif /* USE_USB_CDC_MAIN */
