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
#define HUB_W5500_INT_GPIO              GPIO_NUM_NC
#endif

#ifndef HUB_W5500_RST_GPIO
#define HUB_W5500_RST_GPIO              GPIO_NUM_9
#endif

#ifndef HUB_W5500_SPI_CLOCK_HZ
#define HUB_W5500_SPI_CLOCK_HZ          (1 * 1000 * 1000)
#endif

#ifndef HUB_W5500_SPI_ONLY_TEST
#define HUB_W5500_SPI_ONLY_TEST         0
#endif

// Read-only raw SPI test mode: install does not start esp_eth driver,
// it only logs MR/VERSIONR/PHYCFGR by direct SPI reads.
#ifndef HUB_W5500_RAW_SPI_READONLY_TEST
#define HUB_W5500_RAW_SPI_READONLY_TEST 0
#endif

// Optional raw SPI-only PHY reset test. When enabled, the raw test asserts
// PHYCFGR.RST once after SPI device setup, then continues read-only polling.
#ifndef HUB_W5500_RAW_PHY_RESET_TEST
#define HUB_W5500_RAW_PHY_RESET_TEST    1
#endif

#ifndef HUB_W5500_DRIVER_REG_DEBUG
#define HUB_W5500_DRIVER_REG_DEBUG      0
#endif

#ifndef HUB_W5500_PHYCFGR_READ_DEBUG
#define HUB_W5500_PHYCFGR_READ_DEBUG    0
#endif

#ifndef HUB_W5500_POLL_PERIOD_MS
#define HUB_W5500_POLL_PERIOD_MS        100
#endif

esp_err_t hub_w5500_install(esp_eth_handle_t *eth_handle);

// Direct SPI debug read of common W5500 registers (MR, VERSIONR, PHYCFGR).
// Reads are performed directly over SPI (common block read) and do not use
// ETH driver PHY ioctls. Useful to detect SPI/CS read failures that make
// PHY ioctl reads return bogus values (0xff).
// Returns ESP_OK on success; values are written to pointers when non-NULL.
esp_err_t hub_w5500_read_common_regs(uint8_t *mr, uint8_t *versionr, uint8_t *phycfg);

#ifdef __cplusplus
}
#endif

#endif