#ifndef HUB_W5500_H
#define HUB_W5500_H

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_eth.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HUB_W5500_SPI_HOST
#define HUB_W5500_SPI_HOST              SPI3_HOST
#endif

#ifndef HUB_W5500_MISO_GPIO
#define HUB_W5500_MISO_GPIO             GPIO_NUM_12
#endif

#ifndef HUB_W5500_MOSI_GPIO
#define HUB_W5500_MOSI_GPIO             GPIO_NUM_11
#endif

#ifndef HUB_W5500_SCLK_GPIO
#define HUB_W5500_SCLK_GPIO             GPIO_NUM_13
#endif

#ifndef HUB_W5500_CS_GPIO
#define HUB_W5500_CS_GPIO               GPIO_NUM_14
#endif

#ifndef HUB_W5500_INT_GPIO
#define HUB_W5500_INT_GPIO              GPIO_NUM_10
#endif

#ifndef HUB_W5500_RST_GPIO
#define HUB_W5500_RST_GPIO              GPIO_NUM_9
#endif

#ifndef HUB_W5500_SPI_CLOCK_HZ
#define HUB_W5500_SPI_CLOCK_HZ          (10 * 1000 * 1000)
#endif

#ifndef HUB_W5500_POLL_PERIOD_MS
#define HUB_W5500_POLL_PERIOD_MS        100
#endif

esp_err_t hub_w5500_install(esp_eth_handle_t *eth_handle);

#ifdef __cplusplus
}
#endif

#endif