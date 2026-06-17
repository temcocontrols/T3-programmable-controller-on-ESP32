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
// explicit driver includes used by debug SPI helper
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"

static const char *TAG = "hub_w5500";

static spi_device_handle_t s_w5500_debug_spi = NULL;
static SemaphoreHandle_t s_w5500_debug_lock = NULL;
static bool s_w5500_spi_bus_ready = false;

#define HUB_W5500_GPIO14_TEST 0
#define W5500_COMMON_BLOCK_CTRL_READ    0x00
#define W5500_COMMON_BLOCK_CTRL_WRITE   0x04
#define W5500_MR_ADDR                   0x0000
#define W5500_PHYCFGR_ADDR              0x002E
#define W5500_VERSIONR_ADDR             0x0039
#define W5500_EXPECTED_VERSION          0x04
#define W5500_PHYCFGR_RST_BIT           0x80

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
        ESP_LOGW(TAG, "W5500 reset skipped: reset GPIO is NC");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "W5500 reset begin on GPIO%d", HUB_W5500_RST_GPIO);

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
    ESP_LOGI(TAG,
             "W5500 SPI bus init: host=%d miso=%d mosi=%d sclk=%d max_transfer=%d",
             HUB_W5500_SPI_HOST,
             HUB_W5500_MISO_GPIO,
             HUB_W5500_MOSI_GPIO,
             HUB_W5500_SCLK_GPIO,
             4096);

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
        s_w5500_spi_bus_ready = true;
        return ESP_OK;
    }

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "W5500 SPI bus init OK");
        s_w5500_spi_bus_ready = true;
    }

    return ret;
}

static esp_err_t hub_w5500_init_debug_spi_device(void)
{
    if (s_w5500_debug_spi != NULL) {
        return ESP_OK;
    }
    if (!s_w5500_spi_bus_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    spi_device_interface_config_t spi_device_config = {
        .mode = 0,
        .clock_speed_hz = HUB_W5500_SPI_CLOCK_HZ,
        .spics_io_num = HUB_W5500_CS_GPIO,
        .queue_size = 1,
    };

    esp_err_t ret = spi_bus_add_device(HUB_W5500_SPI_HOST, &spi_device_config, &s_w5500_debug_spi);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "W5500 debug SPI add device failed: %s", esp_err_to_name(ret));
        return ret;
    }

    if (s_w5500_debug_lock == NULL) {
        s_w5500_debug_lock = xSemaphoreCreateMutex();
        if (s_w5500_debug_lock == NULL) {
            spi_bus_remove_device(s_w5500_debug_spi);
            s_w5500_debug_spi = NULL;
            return ESP_ERR_NO_MEM;
        }
    }

    ESP_LOGI(TAG, "W5500 debug SPI device ready (shared, hw CS)");
    return ESP_OK;
}

// Production helper: perform a single-byte SPI read from W5500 common block.
// Uses a shared device handle created once at install time.
static esp_err_t hub_w5500_do_spi_read(spi_device_handle_t spi, uint8_t block_ctrl, uint16_t addr, uint8_t *value)
{
    if (value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    spi_transaction_t transaction = {0};
    transaction.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    transaction.length = 32; // 4 bytes
    transaction.tx_data[0] = (uint8_t)(addr >> 8);
    transaction.tx_data[1] = (uint8_t)(addr & 0xff);
    transaction.tx_data[2] = block_ctrl;
    transaction.tx_data[3] = 0x00;

    esp_err_t ret = spi_device_polling_transmit(spi, &transaction);
    if (ret == ESP_OK) {
        *value = transaction.rx_data[3];
    }

    return ret;
}

esp_err_t hub_w5500_read_common_regs(uint8_t *mr, uint8_t *versionr, uint8_t *phycfg)
{
    esp_err_t ret = ESP_OK;

    if ((mr == NULL) && (versionr == NULL) && (phycfg == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_w5500_debug_spi == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_w5500_debug_lock == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(s_w5500_debug_lock, pdMS_TO_TICKS(50)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    ret = spi_device_acquire_bus(s_w5500_debug_spi, portMAX_DELAY);
    if (ret != ESP_OK) {
        xSemaphoreGive(s_w5500_debug_lock);
        return ret;
    }

    esp_err_t final_ret = ESP_OK;
    uint8_t mr_v = 0;
    ret = hub_w5500_do_spi_read(s_w5500_debug_spi, W5500_COMMON_BLOCK_CTRL_READ, W5500_MR_ADDR, &mr_v);
    if (ret != ESP_OK) {
        final_ret = ret;
    }

    uint8_t ver_v = 0;
    ret = hub_w5500_do_spi_read(s_w5500_debug_spi, W5500_COMMON_BLOCK_CTRL_READ, W5500_VERSIONR_ADDR, &ver_v);
    if (ret != ESP_OK) {
        final_ret = ret;
    }

    uint8_t phycfg_v = 0;
    ret = hub_w5500_do_spi_read(s_w5500_debug_spi, W5500_COMMON_BLOCK_CTRL_READ, W5500_PHYCFGR_ADDR, &phycfg_v);
    if (ret != ESP_OK) {
        final_ret = ret;
    }

    if (mr != NULL) {
        *mr = mr_v;
    }
    if (versionr != NULL) {
        *versionr = ver_v;
    }
    if (phycfg != NULL) {
        *phycfg = phycfg_v;
    }

    // Hardware-CS transaction should leave CS idle high; detect and report if not.
    if (HUB_W5500_CS_GPIO != GPIO_NUM_NC) {
        int cs_level = gpio_get_level(HUB_W5500_CS_GPIO);
        if (cs_level == 0) {
            gpio_set_level(HUB_W5500_CS_GPIO, 1);
            ESP_LOGW(TAG, "W5500 debug SPI read finished and forced CS GPIO%d back to high", HUB_W5500_CS_GPIO);
        }
    }

    spi_device_release_bus(s_w5500_debug_spi);
    xSemaphoreGive(s_w5500_debug_lock);

    return final_ret;
}

#if HUB_W5500_SPI_ONLY_TEST || HUB_W5500_RAW_SPI_READONLY_TEST
static esp_err_t hub_w5500_spi_read_u8(spi_device_handle_t spi, uint8_t block_ctrl, uint16_t addr, uint8_t *value)
{
    if ((spi == NULL) || (value == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    spi_transaction_t transaction = {0};
    transaction.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    transaction.length = 32;
    transaction.tx_data[0] = (uint8_t)(addr >> 8);
    transaction.tx_data[1] = (uint8_t)(addr & 0xff);
    transaction.tx_data[2] = block_ctrl;
    transaction.tx_data[3] = 0x00;

    esp_err_t ret = spi_device_polling_transmit(spi, &transaction);
    if (ret == ESP_OK) {
        *value = transaction.rx_data[3];
    }

    return ret;
}

static esp_err_t hub_w5500_spi_write_u8(spi_device_handle_t spi, uint8_t block_ctrl, uint16_t addr, uint8_t value)
{
    if (spi == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    spi_transaction_t transaction = {0};
    transaction.flags = SPI_TRANS_USE_TXDATA;
    transaction.length = 32;
    transaction.tx_data[0] = (uint8_t)(addr >> 8);
    transaction.tx_data[1] = (uint8_t)(addr & 0xff);
    transaction.tx_data[2] = block_ctrl;
    transaction.tx_data[3] = value;

    return spi_device_polling_transmit(spi, &transaction);
}

static void hub_w5500_raw_phy_reset_once(spi_device_handle_t spi)
{
#if HUB_W5500_RAW_PHY_RESET_TEST
    uint8_t before = 0;
    esp_err_t read_ret = hub_w5500_spi_read_u8(spi, W5500_COMMON_BLOCK_CTRL_READ, W5500_PHYCFGR_ADDR, &before);
    if (read_ret != ESP_OK) {
        ESP_LOGW(TAG, "W5500 raw PHY reset skipped: PHYCFGR read failed: %s", esp_err_to_name(read_ret));
        return;
    }

    uint8_t reset_value = before & (uint8_t)~W5500_PHYCFGR_RST_BIT;
    esp_err_t reset_ret = hub_w5500_spi_write_u8(spi, W5500_COMMON_BLOCK_CTRL_WRITE, W5500_PHYCFGR_ADDR, reset_value);
    ESP_LOGW(TAG,
             "W5500 raw PHY reset assert: before=0x%02x write=0x%02x(%s)",
             before,
             reset_value,
             esp_err_to_name(reset_ret));
    if (reset_ret != ESP_OK) {
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    uint8_t release_value = before | W5500_PHYCFGR_RST_BIT;
    esp_err_t release_ret = hub_w5500_spi_write_u8(spi, W5500_COMMON_BLOCK_CTRL_WRITE, W5500_PHYCFGR_ADDR, release_value);
    ESP_LOGW(TAG,
             "W5500 raw PHY reset release: write=0x%02x(%s)",
             release_value,
             esp_err_to_name(release_ret));

    vTaskDelay(pdMS_TO_TICKS(500));
#else
    (void)spi;
#endif
}

static const char *hub_w5500_spi_ret_name(esp_err_t ret)
{
    return ret == ESP_OK ? "OK" : esp_err_to_name(ret);
}

static void hub_w5500_spi_only_log_regs(spi_device_handle_t spi, uint32_t count)
{
    uint8_t mr = 0;
    uint8_t version = 0;
    uint8_t phycfg = 0;

    esp_err_t mr_ret = hub_w5500_spi_read_u8(spi, W5500_COMMON_BLOCK_CTRL_READ, W5500_MR_ADDR, &mr);
    esp_err_t version_ret = hub_w5500_spi_read_u8(spi, W5500_COMMON_BLOCK_CTRL_READ, W5500_VERSIONR_ADDR, &version);
    esp_err_t phycfg_ret = hub_w5500_spi_read_u8(spi, W5500_COMMON_BLOCK_CTRL_READ, W5500_PHYCFGR_ADDR, &phycfg);
    int rst_bit = phycfg_ret == ESP_OK ? (int)((phycfg >> 7) & 0x01) : -1;
    int opmd_bit = phycfg_ret == ESP_OK ? (int)((phycfg >> 6) & 0x01) : -1;
    int opmdc_bits = phycfg_ret == ESP_OK ? (int)((phycfg >> 3) & 0x07) : -1;
    int dpx_bit = phycfg_ret == ESP_OK ? (int)((phycfg >> 2) & 0x01) : -1;
    int spd_bit = phycfg_ret == ESP_OK ? (int)((phycfg >> 1) & 0x01) : -1;
    int link_bit = phycfg_ret == ESP_OK ? (int)(phycfg & 0x01) : -1;
    int cs_level = HUB_W5500_CS_GPIO == GPIO_NUM_NC ? -1 : gpio_get_level(HUB_W5500_CS_GPIO);
    int rst_level = HUB_W5500_RST_GPIO == GPIO_NUM_NC ? -1 : gpio_get_level(HUB_W5500_RST_GPIO);

    ESP_LOGW(TAG,
             "W5500 raw SPI-only[%lu]: VERSIONR=0x%02x expected=0x%02x(%s) MR=0x%02x(%s) PHYCFGR=0x%02x(%s) RST=%d OPMD=%d OPMDC=%d DPX=%d SPD=%d LNK=%d cs_gpio%d=%d rst_gpio%d=%d",
             (unsigned long)count,
             version,
             W5500_EXPECTED_VERSION,
             hub_w5500_spi_ret_name(version_ret),
             mr,
             hub_w5500_spi_ret_name(mr_ret),
             phycfg,
             hub_w5500_spi_ret_name(phycfg_ret),
             rst_bit,
             opmd_bit,
             opmdc_bits,
             dpx_bit,
             spd_bit,
             link_bit,
             HUB_W5500_CS_GPIO,
             cs_level,
             HUB_W5500_RST_GPIO,
             rst_level);
}

static esp_err_t hub_w5500_spi_only_test(void)
{
    ESP_LOGW(TAG,
             "W5500 raw SPI-only read test enabled: esp_eth driver/netif/start are paused; keep cable unplugged");
    ESP_LOGW(TAG,
             "W5500 SPI-only pins: rst=%d miso=%d mosi=%d sclk=%d cs=%d clock=%d mode=0",
             HUB_W5500_RST_GPIO,
             HUB_W5500_MISO_GPIO,
             HUB_W5500_MOSI_GPIO,
             HUB_W5500_SCLK_GPIO,
             HUB_W5500_CS_GPIO,
             HUB_W5500_SPI_CLOCK_HZ);

    esp_err_t ret = hub_w5500_reset();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = hub_w5500_init_spi_bus();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "W5500 SPI-only bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    spi_device_interface_config_t spi_device_config = {
        .mode = 0,
        .clock_speed_hz = HUB_W5500_SPI_CLOCK_HZ,
        .spics_io_num = HUB_W5500_CS_GPIO,
        .queue_size = 1,
    };

    spi_device_handle_t spi = NULL;
    ret = spi_bus_add_device(HUB_W5500_SPI_HOST, &spi_device_config, &spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "W5500 SPI-only add device failed: %s", esp_err_to_name(ret));
        return ret;
    }

    hub_w5500_raw_phy_reset_once(spi);

    hub_w5500_spi_only_log_regs(spi, 0);

    uint32_t count = 1;
    while (1) {
        hub_w5500_spi_only_log_regs(spi, count++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    return ESP_ERR_INVALID_STATE;
}
#endif

static esp_err_t hub_w5500_init_gpio_isr_service(void)
{
    if (HUB_W5500_INT_GPIO == GPIO_NUM_NC) {
        ESP_LOGW(TAG, "W5500 GPIO ISR service skipped: INT GPIO is NC");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "W5500 GPIO ISR service init for INT GPIO%d", HUB_W5500_INT_GPIO);
    esp_err_t ret = gpio_install_isr_service(0);
    if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "GPIO ISR service already installed, reusing it");
        return ESP_OK;
    }

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "W5500 GPIO ISR service init OK");
    }

    return ret;
}

esp_err_t hub_w5500_install(esp_eth_handle_t *eth_handle)
{
    if (eth_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (*eth_handle != NULL) {
        ESP_LOGW(TAG, "W5500 install skipped: eth_handle already set (%p)", *eth_handle);
        return ESP_OK;
    }

    ESP_LOGI(TAG,
             "W5500 install begin: host=%d miso=%d mosi=%d sclk=%d cs=%d int=%d rst=%d clock=%d",
             HUB_W5500_SPI_HOST,
             HUB_W5500_MISO_GPIO,
             HUB_W5500_MOSI_GPIO,
             HUB_W5500_SCLK_GPIO,
             HUB_W5500_CS_GPIO,
             HUB_W5500_INT_GPIO,
             HUB_W5500_RST_GPIO,
             HUB_W5500_SPI_CLOCK_HZ);

#if HUB_W5500_GPIO14_TEST
    hub_w5500_gpio14_test();
    return ESP_ERR_INVALID_STATE;
#endif

#if HUB_W5500_SPI_ONLY_TEST || HUB_W5500_RAW_SPI_READONLY_TEST
    return hub_w5500_spi_only_test();
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

#if HUB_W5500_PHYCFGR_READ_DEBUG
    // Full-driver direct SPI debug is disabled by default. Only create this
    // second SPI device when the monitor is explicitly enabled.
    ret = hub_w5500_init_debug_spi_device();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "W5500 debug SPI init failed: %s", esp_err_to_name(ret));
        return ret;
    }
#endif

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

    ESP_LOGI(TAG,
             "W5500 SPI device config: mode=%d clock=%d cs=%d queue=%d",
             spi_device_config.mode,
             spi_device_config.clock_speed_hz,
             spi_device_config.spics_io_num,
             spi_device_config.queue_size);

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
    ESP_LOGI(TAG, "W5500 MAC init OK");

    esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);
    if (phy == NULL) {
        ESP_LOGE(TAG, "W5500 PHY init failed");
        mac->del(mac);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "W5500 PHY init OK: phy_addr=%d reset_gpio=%d int_gpio=%d",
             phy_config.phy_addr,
             phy_config.reset_gpio_num,
             w5500_config.int_gpio_num);

    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    ret = esp_eth_driver_install(&eth_config, eth_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "W5500 driver install failed: %s", esp_err_to_name(ret));
        phy->del(phy);
        mac->del(mac);
        *eth_handle = NULL;
        return ret;
    }
    ESP_LOGI(TAG, "W5500 driver install OK, handle=%p", *eth_handle);

    uint8_t mac_addr[ETH_ADDR_LEN] = {0};
    if (esp_read_mac(mac_addr, ESP_MAC_ETH) == ESP_OK) {
        ESP_LOGI(TAG, "W5500 base ETH MAC: %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        ret = esp_eth_ioctl(*eth_handle, ETH_CMD_S_MAC_ADDR, mac_addr);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "W5500 MAC address set OK");
        } else {
            ESP_LOGW(TAG, "W5500 MAC address set failed: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGW(TAG, "W5500 base ETH MAC read failed");
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