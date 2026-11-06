
/**
 * @file  touch_drv.c
 * @brief Touch Driver Implementation for the LCD Subsystem.
 * Detailed description:
 * - This module implements the touch driver for the LCD subsystem.
 * Responsibilities:
 * - Initialize the touch driver.
 * - Handle I2C communication with the touch controller.
 * Implement touch event reading and processing.
 *
 * @author  Bhavik Panchal
 * @date    22-12-2025
 * @version 1.0
 *
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "i2c_task.h"

#include "touch_drv.h"

static const char *TAG = "touch_drv";

#define TOUCH_PIN_SCL_IO           GPIO_NUM_47         /*!< GPIO number used for I2C master clock */
#define TOUCH_PIN_SDA_IO           GPIO_NUM_48         /*!< GPIO number used for I2 master data  */

#define TOUCH_I2C_NUM              I2C_NUM_0   /*!< I2C port number for master dev */
#define TOUCH_I2C_FREQ_HZ          100000      /*!< I2C master clock frequency */
#define TOUCH_I2C_TX_BUF_DISABLE   0           /*!< I2C master doesn't need buffer */
#define TOUCH_I2C_RX_BUF_DISABLE   0           /*!< I2C master doesn't need buffer */

#define TOUCH_ACK_CHECK_EN         0x1         /*!< I2C master will check ack from slave*/
#define TOUCH_ACK_CHECK_DIS        0x0         /*!< I2C master will not check ack from slave */
#define TOUCH_ACK_VAL              0x0         /*!< I2C ack value */
#define TOUCH_NACK_VAL             0x1         /*!< I2C nack value */

#define GT911_INT_PIN   GPIO_NUM_45
#define GT911_RST_PIN   GPIO_NUM_38

#define GT911_I2C_ADDR  0x14
#define I2C_TIMEOUT_MS  100
#define GOODIX_REG_ID   0x8140

/* Lower = more sensitive. 0x18 felt like factory; 0x0A is a real step. Raise if ghost taps. */
#define GT911_TOUCH_THRESHOLD  0x02
#define GT911_CFG_START        0x8047
#define GT911_CFG_LEN          186
#define GT911_CFG_BODY_LEN     184
#define GT911_CFG_CHUNK        16
#define GT911_OFFS_TOUCH_TH    0x0C  /* 0x8053 */
#define GT911_OFFS_LEAVE_TH    0x0D  /* 0x8054 */

#define TOUCH_MAX_X  320
#define TOUCH_MAX_Y  480

esp_err_t Touch_I2C_Init()
{
    int i2c_master_port = TOUCH_I2C_NUM;
    static i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;

    conf.sda_io_num = TOUCH_PIN_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = TOUCH_PIN_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = TOUCH_I2C_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode,
                              TOUCH_I2C_RX_BUF_DISABLE,
                              TOUCH_I2C_TX_BUF_DISABLE, 0);
}

esp_err_t Touch_i2c_Write(uint8_t addr, uint16_t reg,
                      const uint8_t *data, uint16_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | WRITE_BIT, ACK_CHECK_EN);

    i2c_master_write_byte(cmd, reg >> 8, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg & 0xFF, ACK_CHECK_EN);
    if ((data != NULL) && (len > 0)) {
        i2c_master_write(cmd, data, len, ACK_CHECK_EN);
    }

    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(TOUCH_I2C_NUM, cmd, pdMS_TO_TICKS(1000));

    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t Touch_i2c_Read(uint8_t addr, uint16_t reg,
                     uint8_t *data, uint16_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Write register address
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg >> 8, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg & 0xFF, ACK_CHECK_EN);

    // Repeated START + Read
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | READ_BIT, ACK_CHECK_EN);

    if (len > 1) {
        i2c_master_read(cmd, data, (size_t)(len - 1), I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, &data[len - 1], I2C_MASTER_NACK);

    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(
        TOUCH_I2C_NUM, cmd, pdMS_TO_TICKS(100));

    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t TouchReadConfig( void )
{
   	esp_err_t ret;
    uint8_t addr = GT911_I2C_ADDR;
	uint8_t id[4] = {0};

	ESP_LOGI(TAG, "GT911 trying address 0x%02X", addr);

	ret = Touch_i2c_Read(addr, GOODIX_REG_ID, id, 4);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "GT911 ID read failed");
		return ret;
	}

	ESP_LOGI(TAG, "GT911 ID = %c%c%c%c", id[0], id[1], id[2], id[3]);
	ESP_LOGI(TAG, "GT911 ID @0x%02X = %c%c%c%c", addr, id[0], id[1], id[2], id[3]);

	uint8_t fw[2];

	Touch_i2c_Read(addr, 0x8144, fw, 2);

	uint16_t fw_ver = fw[0] | (fw[1] << 8);

	ESP_LOGI(TAG, "GT911 FW version: 0x%04X", fw_ver);

	uint8_t cfg_ver;

	Touch_i2c_Read(addr, 0x8047, &cfg_ver, 1);

	ESP_LOGI(TAG, "GT911 CFG version: 0x%02X", cfg_ver);

	uint8_t status;

	Touch_i2c_Read(addr, 0x814E, &status, 1);

	ESP_LOGI(TAG, "GT911 Status: 0x%02X", status);

    uint8_t clear = 0x00;
	Touch_i2c_Write(addr, 0x814E, &clear, 1);

	uint8_t touch[8];

	Touch_i2c_Read(addr, 0x8150, touch, 8);

	ESP_LOGD(TAG, "GT911 Touch Data: %02X %02X %02X %02X %02X %02X %02X %02X",
			touch[0], touch[1], touch[2], touch[3],
			touch[4], touch[5], touch[6], touch[7]);

	Touch_i2c_Write(addr, 0x814E, &clear, 1);

	return ESP_OK;
}

static esp_err_t gt911_write_cfg(const uint8_t *cfg)
{
    uint8_t addr = GT911_I2C_ADDR;
    uint16_t off = 0;
    uint8_t fresh = 1;

    while (off < GT911_CFG_BODY_LEN) {
        uint16_t n = (uint16_t)(GT911_CFG_BODY_LEN - off);
        if (n > GT911_CFG_CHUNK) {
            n = GT911_CFG_CHUNK;
        }
        if (Touch_i2c_Write(addr, (uint16_t)(GT911_CFG_START + off), cfg + off, n) != ESP_OK) {
            return ESP_FAIL;
        }
        off = (uint16_t)(off + n);
    }

    if (Touch_i2c_Write(addr, 0x80FF, &cfg[184], 1) != ESP_OK) {
        return ESP_FAIL;
    }
    return Touch_i2c_Write(addr, 0x8100, &fresh, 1);
}

static void gt911_apply_touch_threshold(void)
{
    uint8_t cfg[GT911_CFG_LEN];
    uint8_t addr = GT911_I2C_ADDR;
    uint8_t th = 0;

    if (Touch_i2c_Read(addr, GT911_CFG_START, cfg, GT911_CFG_LEN) != ESP_OK) {
        ESP_LOGE(TAG, "GT911 cfg read fail");
        return;
    }

    ESP_LOGI(TAG, "GT911 Touch_Level was 0x%02X, set 0x%02X",
             cfg[GT911_OFFS_TOUCH_TH], GT911_TOUCH_THRESHOLD);

    if (cfg[0] < 0xFF) {
        cfg[0]++;
    } else {
        cfg[0] = 1;
    }
    cfg[GT911_OFFS_TOUCH_TH] = GT911_TOUCH_THRESHOLD;
    cfg[GT911_OFFS_LEAVE_TH] = (GT911_TOUCH_THRESHOLD > 4)
                                   ? (uint8_t)(GT911_TOUCH_THRESHOLD - 4) : 1;

    uint8_t sum = 0;
    for (int i = 0; i < GT911_CFG_BODY_LEN; i++) {
        sum += cfg[i];
    }
    cfg[184] = (uint8_t)((~sum) + 1);
    cfg[185] = 1;

    if (gt911_write_cfg(cfg) != ESP_OK) {
        ESP_LOGE(TAG, "GT911 cfg write fail");
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(80));

    if (Touch_i2c_Read(addr, 0x8053, &th, 1) != ESP_OK) {
        ESP_LOGE(TAG, "GT911 Touch_Level readback fail");
        return;
    }
    if (th != GT911_TOUCH_THRESHOLD) {
        ESP_LOGE(TAG, "GT911 Touch_Level now 0x%02X (wanted 0x%02X)",
                 th, GT911_TOUCH_THRESHOLD);
        return;
    }
    ESP_LOGI(TAG, "GT911 Touch_Level now 0x%02X", th);
}

static void Touch_Gpio_Init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GT911_INT_PIN) | (1ULL << GT911_RST_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

static void TouchReset(void)
{
    gpio_set_direction(GT911_INT_PIN, GPIO_MODE_OUTPUT);

    gpio_set_direction(GT911_RST_PIN, GPIO_MODE_OUTPUT);

    gpio_set_level(GT911_RST_PIN, 1);

    gpio_set_level(GT911_INT_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(50));

    gpio_set_level(GT911_RST_PIN, 0);

    vTaskDelay(pdMS_TO_TICKS(20));

	gpio_set_level(GT911_RST_PIN, 1);

    vTaskDelay(pdMS_TO_TICKS(1000));

    gpio_set_direction(GT911_INT_PIN, GPIO_MODE_INPUT);
    vTaskDelay(pdMS_TO_TICKS(100));
}

bool TouchGetInputs(uint16_t *x, uint16_t *y)
{
    uint8_t status;
    uint8_t addr = GT911_I2C_ADDR;
    uint8_t buf[8] = {0};
    uint8_t clear = 0x00;

    if (!x || !y)
        return false;

    /* 1. Read touch status */
    if (Touch_i2c_Read(addr, 0x814E, &status, 1) != ESP_OK)
    {
        ESP_LOGW(TAG, "TouchGetInputs: Failed to read status");
        return false;
    }

    /* Data not ready */
    if (!(status & 0x80))
    {
        return false;
    }

    uint8_t points = status & 0x0F;

        /* No touch points */
    if (points == 0) {
        Touch_i2c_Write(GT911_I2C_ADDR, 0x814E, &clear, 1);
        return false;
    }

        /* Read first touch point */
    if (Touch_i2c_Read(GT911_I2C_ADDR, 0x8150, buf, 8) != ESP_OK) {
        ESP_LOGW(TAG, "TouchGetInputs: Failed to read touch point");
        Touch_i2c_Write(GT911_I2C_ADDR, 0x814E, &clear, 1);
        return false;
    }

    *x = (((uint16_t)buf[GT911_P1_XH_TP_BIT_POSITION] & GT911_P1_XH_TP_BIT_MASK) << 8U) | ((uint16_t)buf[GT911_P1_XL_TP_BIT_POSITION] & GT911_P1_XL_TP_BIT_MASK);
      /* Send back first ready Y position to caller */
    *y = (((uint16_t)buf[GT911_P1_YH_TP_BIT_POSITION] & GT911_P1_YH_TP_BIT_MASK) << 8U) | ((uint16_t)buf[GT911_P1_YL_TP_BIT_POSITION] & GT911_P1_YL_TP_BIT_MASK);

    *x = TOUCH_MAX_X - *x;
    *y = TOUCH_MAX_Y - *y;
    if (*x > TOUCH_MAX_X) *x = TOUCH_MAX_X;
    if (*y >TOUCH_MAX_Y) *y = TOUCH_MAX_Y;

    /* 3. Clear status (MANDATORY) */
    Touch_i2c_Write(addr, 0x814E, &clear, 1);

    return true;
}

void gt911_print_touch( void )
{
    uint8_t status;
    uint8_t addr = GT911_I2C_ADDR;
    uint8_t buf[8];
    uint8_t clear = 0x00;
    uint16_t x, y;

    /* 1. Read touch status */
    if (Touch_i2c_Read(addr, 0x814E, &status, 1) != ESP_OK)
        return;

    /* Data not ready */
    if (!(status & 0x80))
        return;

    uint8_t points = status & 0x0F;

    /* 2. Read first touch point (only if touched) */
    if (points > 0) {
        if (Touch_i2c_Read(addr, 0x8150, buf, 8) == ESP_OK) {

            x = (((uint16_t)buf[GT911_P1_XH_TP_BIT_POSITION] & GT911_P1_XH_TP_BIT_MASK) << 8U) | ((uint16_t)buf[GT911_P1_XL_TP_BIT_POSITION] & GT911_P1_XL_TP_BIT_MASK);
            y = (((uint16_t)buf[GT911_P1_YH_TP_BIT_POSITION] & GT911_P1_YH_TP_BIT_MASK) << 8U) | ((uint16_t)buf[GT911_P1_YL_TP_BIT_POSITION] & GT911_P1_YL_TP_BIT_MASK);

            x = TOUCH_MAX_X - x;
            y = TOUCH_MAX_Y - y;
            // x = buf[1] | (buf[2] << 8);
            // y = buf[3] | (buf[4] << 8);

            ESP_LOGD(TAG, "Touch: X=%d Y=%d", x, y);
        }
    }

    /* 3. Clear status (MANDATORY) */
    Touch_i2c_Write(addr, 0x814E, &clear, 1);
}


void Touch_Drv_Init(void)
{
    Touch_I2C_Init();

    Touch_Gpio_Init();
    TouchReset();

    vTaskDelay(pdMS_TO_TICKS(500));

    TouchReadConfig();
    gt911_apply_touch_threshold();

#if 0
    uint32_t counter = 0;
    while(counter < 200)
    {
        counter++;
        vTaskDelay(pdMS_TO_TICKS(100));
        gt911_print_touch();
    }
#endif

}