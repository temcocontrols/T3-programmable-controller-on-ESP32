#include "hub_w5500.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_eth.h"
#include "esp_eth_mac_spi.h"
#include "esp_eth_phy.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "sdkconfig.h"

static const char *TAG = "hub_w5500";

#define HUB_W5500_GPIO14_TEST 1

#if CONFIG_ETH_SPI_ETHERNET_W5500
extern esp_eth_phy_t *esp_eth_phy_new_w5500(const eth_phy_config_t *config);

#if HUB_W5500_GPIO14_TEST
static void hub_w5500_gpio14_test(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << GPIO_NUM_14,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO14 test config failed: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGW(TAG, "GPIO14 test mode enabled; W5500 init is paused. GPIO14 toggles every 500 ms.");
    while (1) {
        gpio_set_level(GPIO_NUM_14, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(GPIO_NUM_14, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
#endif

static esp_err_t hub_w5500_reset(void)
{
    if (HUB_W5500_RST_GPIO == GPIO_NUM_NC) {
        return ESP_OK;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << HUB_W5500_RST_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "W5500 reset GPIO config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    gpio_set_level(HUB_W5500_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(HUB_W5500_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(200));

    ESP_LOGI(TAG, "W5500 reset pulse done on GPIO%d", HUB_W5500_RST_GPIO);
    return ESP_OK;
}

static esp_err_t hub_w5500_init_spi_bus(void)
{
    spi_bus_config_t bus_config = {
        .miso_io_num = HUB_W5500_MISO_GPIO,
        .mosi_io_num = HUB_W5500_MOSI_GPIO,
        .sclk_io_num = HUB_W5500_SCLK_GPIO,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = 4096,
    };

    esp_err_t ret = spi_bus_initialize(HUB_W5500_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "SPI host already initialized, reusing it");
        return ESP_OK;
    }

    return ret;
}

static esp_err_t hub_w5500_init_gpio_isr_service(void)
{
    if (HUB_W5500_INT_GPIO == GPIO_NUM_NC) {
        return ESP_OK;
    }

    esp_err_t ret = gpio_install_isr_service(0);
    if (ret == ESP_ERR_INVALID_STATE) {
        return ESP_OK;
    }

    return ret;
}

esp_err_t hub_w5500_install(esp_eth_handle_t *eth_handle)
{
    if (eth_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (*eth_handle != NULL) {
        return ESP_OK;
    }

#if HUB_W5500_GPIO14_TEST
    hub_w5500_gpio14_test();
    return ESP_ERR_INVALID_STATE;
#endif

    esp_err_t ret = hub_w5500_reset();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = hub_w5500_init_spi_bus();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "W5500 SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = hub_w5500_init_gpio_isr_service();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "W5500 GPIO ISR service init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    spi_device_interface_config_t spi_device_config = {
        .mode = 0,
        .clock_speed_hz = HUB_W5500_SPI_CLOCK_HZ,
        .spics_io_num = HUB_W5500_CS_GPIO,
        .queue_size = 20,
    };

    eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(HUB_W5500_SPI_HOST, &spi_device_config);
    w5500_config.int_gpio_num = HUB_W5500_INT_GPIO;
    if (HUB_W5500_INT_GPIO == GPIO_NUM_NC) {
        w5500_config.poll_period_ms = HUB_W5500_POLL_PERIOD_MS;
    }

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = 0;
    phy_config.reset_gpio_num = HUB_W5500_RST_GPIO;

    esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);
    if (mac == NULL) {
        ESP_LOGE(TAG, "W5500 MAC init failed");
        return ESP_FAIL;
    }

    esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);
    if (phy == NULL) {
        ESP_LOGE(TAG, "W5500 PHY init failed");
        mac->del(mac);
        return ESP_FAIL;
    }

    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    ret = esp_eth_driver_install(&eth_config, eth_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "W5500 driver install failed: %s", esp_err_to_name(ret));
        phy->del(phy);
        mac->del(mac);
        *eth_handle = NULL;
        return ret;
    }

    uint8_t mac_addr[ETH_ADDR_LEN] = {0};
    if (esp_read_mac(mac_addr, ESP_MAC_ETH) == ESP_OK) {
        esp_eth_ioctl(*eth_handle, ETH_CMD_S_MAC_ADDR, mac_addr);
    }

    ESP_LOGI(TAG,
             "W5500 installed: host=%d miso=%d mosi=%d sclk=%d cs=%d int=%d rst=%d clock=%d",
             HUB_W5500_SPI_HOST,
             HUB_W5500_MISO_GPIO,
             HUB_W5500_MOSI_GPIO,
             HUB_W5500_SCLK_GPIO,
             HUB_W5500_CS_GPIO,
             HUB_W5500_INT_GPIO,
             HUB_W5500_RST_GPIO,
             HUB_W5500_SPI_CLOCK_HZ);

    return ESP_OK;
}
#else
esp_err_t hub_w5500_install(esp_eth_handle_t *eth_handle)
{
    (void)eth_handle;
    ESP_LOGE(TAG, "CONFIG_ETH_SPI_ETHERNET_W5500 is disabled");
    return ESP_ERR_NOT_SUPPORTED;
}
#endif