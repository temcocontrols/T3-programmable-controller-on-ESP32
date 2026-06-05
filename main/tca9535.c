#include "tca9535.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <string.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "user_data.h"

static const char *TAG = "tca9535";
static i2c_port_t s_i2c_num = I2C_NUM_1;
static uint8_t s_address = 0x24; // default, override in init
extern uint16_t Test[50];
// TCA9535 register map
#define TCA9535_REG_INPUT0     0x00
#define TCA9535_REG_INPUT1     0x01
#define TCA9535_REG_OUTPUT0    0x02
#define TCA9535_REG_OUTPUT1    0x03
#define TCA9535_REG_POLARITY0  0x04
#define TCA9535_REG_POLARITY1  0x05
#define TCA9535_REG_CONFIG0    0x06
#define TCA9535_REG_CONFIG1    0x07


#define I2C_PORT I2C_NUM_1
#define SDA_GPIO GPIO_NUM_4
#define SCL_GPIO GPIO_NUM_14
#define INT_GPIO GPIO_NUM_34
// Use the actual 7-bit I2C address of your hardware. Many boards use 0x20 or 0x21...
#define TCA9535_ADDR 0x24

esp_err_t tca9535_init(i2c_port_t i2c_num, gpio_num_t sda_pin, gpio_num_t scl_pin, uint8_t addr, int clk_speed_hz)
{
    s_i2c_num = i2c_num;
    s_address = addr;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = clk_speed_hz,
    };
    esp_err_t err = i2c_param_config(s_i2c_num, &conf);
    if (err != ESP_OK) return err;
    err = i2c_driver_install(s_i2c_num, I2C_MODE_MASTER, 0, 0, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
    ESP_LOGI(TAG, "Initialized I2C%d SDA=%d SCL=%d addr=0x%02x",
             s_i2c_num, sda_pin, scl_pin, s_address);
    return ESP_OK;
}


uint8_t tca9535_get_address(void)
{
    return s_address;
}


static esp_err_t write_reg(uint8_t reg, uint8_t val)
{
    uint8_t data[2] = {reg, val};
    esp_err_t err = ESP_FAIL;
	err = i2c_master_write_to_device(s_i2c_num, s_address, data, sizeof(data), pdMS_TO_TICKS(500));
	if (err == ESP_OK) return ESP_OK;
    return err;
}

static esp_err_t read_reg(uint8_t reg, uint8_t *val)
{
    esp_err_t err = ESP_FAIL;
	err = i2c_master_write_read_device(s_i2c_num, s_address, &reg, 1, val, 1, pdMS_TO_TICKS(500));
	if (err == ESP_OK) return ESP_OK;
    return err;
}

esp_err_t tca9535_write_outputs(uint8_t out0, uint8_t out1)
{
    esp_err_t err = write_reg(TCA9535_REG_OUTPUT0, out0);
    if (err != ESP_OK) return err;
    return write_reg(TCA9535_REG_OUTPUT1, out1);
}

esp_err_t tca9535_read_inputs(uint8_t *in0, uint8_t *in1)
{
    esp_err_t err = read_reg(TCA9535_REG_INPUT0, in0);
    if (err != ESP_OK) return err;
    return read_reg(TCA9535_REG_INPUT1, in1);
}

esp_err_t tca9535_set_config(uint8_t cfg0, uint8_t cfg1)
{
    esp_err_t err = write_reg(TCA9535_REG_CONFIG0, cfg0);
    if (err != ESP_OK) return err;
    return write_reg(TCA9535_REG_CONFIG1, cfg1);
}

// Internal event queue and tasks for INT handling
static QueueHandle_t s_int_evt_queue = NULL;
typedef enum { EVT_INT_FALL = TCA_EVT_INT_FALL } tca_evt_t;

static void IRAM_ATTR tca_int_isr(void *arg)
{
    uint32_t evt = EVT_INT_FALL;
    BaseType_t hp = pdFALSE;
    if (s_int_evt_queue) {
        xQueueSendFromISR(s_int_evt_queue, &evt, &hp);
        if (hp == pdTRUE) portYIELD_FROM_ISR();
    }
}

static void tca_int_task(void *arg)
{
    uint32_t evt;
    uint8_t in0, in1;
    while (xQueueReceive(s_int_evt_queue, &evt, portMAX_DELAY) == pdTRUE)
    {
        if (evt == EVT_INT_FALL)
        {
            if (tca9535_read_inputs(&in0, &in1) == ESP_OK) {
                ESP_LOGI(TAG, "TCA9535 INT: IN0=0x%02x IN1=0x%02x", in0, in1);
            } else {
                ESP_LOGW(TAG, "TCA9535 read inputs failed on INT");
            }
        }
    }
    vTaskDelete(NULL);
}
void i2c_scan(void);


// Button scanning task: monitors P06,P07,P10..P17 (10 buttons)
static void tca_button_task(void *arg)
{
    // Matrix per schematic:
    // COM1 = P06 (port0 bit6), COM2 = P07 (port0 bit7), COM3 = P10 (port1 bit0)
    // Columns: P13..P16 -> port1 bits 3..6
    // Button index mapping:
    // 0 S2 (P13 & COM1), 1 S3 (P14 & COM1), 2 S6 (P15 & COM1), 3 S10 (P16 & COM1)
    // 4 S4 (P13 & COM2), 5 S5 (P14 & COM2), 6 S7 (P15 & COM2), 7 S11 (P16 & COM2)
    // 8 S13 (P13 & COM3), 9 S12 (P14 & COM3)

    const TickType_t scan_delay = pdMS_TO_TICKS(20);
    const TickType_t sample_delay = pdMS_TO_TICKS(5);
    const int confirm_reads = 2; // require 3 consistent samples

    uint8_t last_state[10] = {0};
    const int col_bits[4] = {3,4,5,6}; // P13..P16 -> port1 bits

    // Read/backup current config/output; use safe defaults on failure
    uint8_t saved_cfg0 = 0xFF, saved_cfg1 = 0xFF;
    uint8_t saved_out0 = 0x00, saved_out1 = 0x00;
    if (read_reg(TCA9535_REG_CONFIG0, &saved_cfg0) != ESP_OK) saved_cfg0 = 0xFF;
    if (read_reg(TCA9535_REG_CONFIG1, &saved_cfg1) != ESP_OK) saved_cfg1 = 0xFF;
    if (read_reg(TCA9535_REG_OUTPUT0, &saved_out0) != ESP_OK) saved_out0 = 0x00;
    if (read_reg(TCA9535_REG_OUTPUT1, &saved_out1) != ESP_OK) saved_out1 = 0x00;

    // Key hold timing for K1..K4 and K_ALL (milliseconds)
    // mapping: K1=S7(idx=6)->slot0, K2=S10(idx=3)->slot1, K3=S11(idx=7)->slot2, K4=S4(idx=4)->slot3
    // slot4 reserved for K_ALL (S2 idx=0)
    uint32_t k_press_start[5] = {0};
    uint32_t k_press_dur[5] = {0};

    while (1) {
        // iterate COM lines: 0->COM1(P06),1->COM2(P07),2->COM3(P10)

        for (int com = 0; com < 3; ++com) {
            uint8_t cfg0 = saved_cfg0;
            uint8_t cfg1 = saved_cfg1;
            uint8_t out0 = saved_out0;
            uint8_t out1 = saved_out1;

            // Set all COMs to input by default
            cfg0 |= (1 << 6) | (1 << 7); // P06,P07 inputs
            cfg1 |= (1 << 0); // P10 input

            // Columns P13..P16 -> inputs
            for (int k = 0; k < 4; ++k) cfg1 |= (1 << col_bits[k]);

            // Make selected COM an output and drive it LOW (active-low scan)
            if (com == 0) {
                cfg0 &= ~(1 << 6); // P06 output
                out0 &= ~(1 << 6); // drive low
            } else if (com == 1) {
                cfg0 &= ~(1 << 7); // P07 output
                out0 &= ~(1 << 7);
            } else {
                cfg1 &= ~(1 << 0); // P10 output
                out1 &= ~(1 << 0);
            }

            // Apply config and outputs
            if (tca9535_set_config(cfg0, cfg1) != ESP_OK) {
                ESP_LOGW(TAG, "tca_button_task: set_config failed");
            }
            if (tca9535_write_outputs(out0, out1) != ESP_OK) {
                ESP_LOGW(TAG, "tca_button_task: write_outputs failed");
            }

            vTaskDelay(pdMS_TO_TICKS(5));

            // For each relevant column, perform multiple reads to confirm press (active-low)
            for (int ci = 0; ci < 4; ++ci) {
                int idx = -1;
                if (com == 0) idx = ci;          // 0..3 -> S2,S3,S6,S10
                else if (com == 1) idx = 4 + ci; // 4..7 -> S4,S5,S7,S11
                else if (com == 2 && ci < 2) idx = 8 + ci; // 8..9 -> S13,S12 (only P13,P14)
                else continue;

                int consistent = 0;
                for (int r = 0; r < confirm_reads; ++r) {
                    uint8_t in0 = 0, in1 = 0;
                    if (tca9535_read_inputs(&in0, &in1) != ESP_OK) break;
                    uint8_t raw = (in1 >> col_bits[ci]) & 0x1; // column read
                    uint8_t pressed = (raw == 0) ? 1 : 0; // active-low => 0 means pressed
                    if (pressed) consistent++; else break;
                    vTaskDelay(sample_delay);
                }
                if (consistent == confirm_reads && last_state[idx] == 0) {
                    last_state[idx] = 1;
                    uint32_t bevt = TCA_EVT_BUTTON_BASE + idx;
                    xQueueSend(s_int_evt_queue, &bevt, 0);
                    ESP_LOGI(TAG, "Button event idx=%d -> evt=%u pressed", idx, (unsigned)bevt);
                    // record press start for K1-K4
                    int slot = -1;
                    // K_ALL (S2)
                    if (idx == 0) slot = 4;
                    else if (idx == 6) slot = 0; // K1 (S7)
                    else if (idx == 3) slot = 1; // K2 (S10)
                    else if (idx == 7) slot = 2; // K3 (S11)
                    else if (idx == 4) slot = 3; // K4 (S4)
                    if (slot >= 0) {
                        k_press_start[slot] = (uint32_t)(esp_timer_get_time() / 1000ULL);
                        ESP_LOGI(TAG, "K%d press start ms=%u", slot==4?0:slot+1, k_press_start[slot]);
                        // update VARs on press: K_ALL -> VAR6, K1..K4 -> VAR1..VAR4
                        int target_var = (slot == 4) ? 5 : slot; // use 0..3 for VAR1..VAR4
                        Str_points_ptr ptr = put_io_buf(VAR, target_var);
                        if (ptr.pvar) {
                            ptr.pvar->value = 1;
                            ptr.pvar->control = 1;
                        }
                    }
                } else if (consistent == 0 && last_state[idx] == 1) {
                    // check for release: require confirm_reads reads of not-pressed
                    int cons_rel = 0;
                    for (int r=0;r<confirm_reads;r++) {
                        uint8_t in0=0,in1=0;
                        if (tca9535_read_inputs(&in0,&in1) != ESP_OK) break;
                        uint8_t raw = (in1 >> col_bits[ci]) & 0x1;
                        uint8_t pressed = (raw == 0) ? 1 : 0;
                        if (!pressed) cons_rel++; else break;
                        vTaskDelay(sample_delay);
                    }
                    if (cons_rel == confirm_reads) {
                        last_state[idx] = 0;
                        ESP_LOGI(TAG, "Button idx=%d released", idx);
                        // if this is K1-K4 compute hold duration
                        int slot = -1;
                        if (idx == 0) slot = 4; // K_ALL
                        else if (idx == 6) slot = 0; // K1
                        else if (idx == 3) slot = 1; // K2
                        else if (idx == 7) slot = 2; // K3
                        else if (idx == 4) slot = 3; // K4
                        if (slot >= 0 && k_press_start[slot] != 0) {
                            uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
                            uint32_t dur = now_ms - k_press_start[slot];
                            k_press_dur[slot] = dur; // store last hold duration
                            k_press_start[slot] = 0; // reset start
                            ESP_LOGI(TAG, "slot=%d hold duration = %u ms", slot, dur);
                            // write hold durations into VAR19..VAR23
                            // mapping: slot0->VAR19 (K1), slot1->VAR20 (K2), slot2->VAR21 (K3), slot3->VAR22 (K4), slot4->VAR23 (K_ALL)
                            // only write hold durations for K1..K4 (slot 0..3) -> put_io_buf(VAR,19..22) => VAR20..VAR23
                            if (slot >= 0 && slot <= 3) {
                                int var_index = 19 + slot; // put_io_buf index
                                Str_points_ptr ptrd = put_io_buf(VAR, var_index);
                                if (ptrd.pvar) {
                                    ptrd.pvar->value = (int32_t)dur;
                                }
                            }
                        }
                        // Also update VARs for button state (set to 0 on release)
                        int clear_var = -1;
                        if (idx == 0) clear_var = 5; // K_ALL -> VAR6 (keep existing mapping)
                        else if (idx == 6) clear_var = 0; // K1 -> put_io_buf(VAR,0)
                        else if (idx == 3) clear_var = 1; // K2 -> put_io_buf(VAR,1)
                        else if (idx == 7) clear_var = 2; // K3 -> put_io_buf(VAR,2)
                        else if (idx == 4) clear_var = 3; // K4 -> put_io_buf(VAR,3)
                        if (clear_var >= 0) {
                            Str_points_ptr ptrc = put_io_buf(VAR, clear_var);
                            if (ptrc.pvar) {
                                ptrc.pvar->value = 0;
                                ptrc.pvar->control = 0;
                            }
                        }
                    }
                }
            }

            // restore saved outputs/config before next com
            tca9535_write_outputs(saved_out0, saved_out1);
            tca9535_set_config(saved_cfg0, saved_cfg1);
            vTaskDelay(pdMS_TO_TICKS(2));
        }

        vTaskDelay(scan_delay);
    }
}

esp_err_t tca9535_start_service(gpio_num_t int_gpio, size_t queue_len)
{
    if (s_int_evt_queue != NULL) {
        return ESP_ERR_INVALID_STATE; // already started
    }

    s_int_evt_queue = xQueueCreate(queue_len ? queue_len : 10, sizeof(uint32_t));
    if (s_int_evt_queue == NULL) return ESP_ERR_NO_MEM;

    // create tasks
    if (xTaskCreate(tca_int_task, "tca_int_task", 4096, NULL, configMAX_PRIORITIES - 5, NULL) != pdPASS) {
        vQueueDelete(s_int_evt_queue); s_int_evt_queue = NULL; return ESP_ERR_NO_MEM;
    }

    xTaskCreate(tca_button_task, "tca_btn", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
 /*   if (xTaskCreate(tca_blink_task, "tca_blink", 3072, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        // blink task non-critical; just log
        ESP_LOGW(TAG, "Failed to create blink task");
    }*/

    // Try to clear any pending INT by reading input registers first
    {
        uint8_t r0 = 0, r1 = 0;
        if (tca9535_read_inputs(&r0, &r1) == ESP_OK) {
            ESP_LOGI(TAG, "Cleared pending INT by reading inputs IN0=0x%02x IN1=0x%02x", r0, r1);
        } else {
            ESP_LOGW(TAG, "Failed to read inputs while clearing INT");
        }
    }

    // Initialize VAR labels/descriptions if empty:
    // VAR1-4 -> put_io_buf(VAR,0..3) labels: K_ON,K_UP,K_OFF,K_DOWN
    const char *var_labels[4] = {"K_ON","K_UP","K_OFF","K_DOWN"};
    for (int i = 0; i < 4; ++i) {
        Str_points_ptr p = put_io_buf(VAR, i);
        if (p.pvar) {
            if (p.pvar->label[0] == 0) {
                strncpy((char*)p.pvar->label, var_labels[i], sizeof(p.pvar->label)-1);
                p.pvar->label[sizeof(p.pvar->label)-1] = 0;
            }
            if (p.pvar->description[0] == 0) {
                strncpy((char*)p.pvar->description, var_labels[i], sizeof(p.pvar->description)-1);
                p.pvar->description[sizeof(p.pvar->description)-1] = 0;
            }
            // set variable metadata: OFF_ON, digital, control default 0
            p.pvar->range = OFF_ON;
            p.pvar->digital_analog = 0;
            p.pvar->control = 0;
        }
    }

    // VAR6 -> K_ALL (use put_io_buf(VAR,6) per existing mapping)
    {
        Str_points_ptr p5 = put_io_buf(VAR, 5);
        if (p5.pvar) {
            if (p5.pvar->label[0] == 0) {
                strncpy((char*)p5.pvar->label, "K_ALL", sizeof(p5.pvar->label)-1);
                p5.pvar->label[sizeof(p5.pvar->label)-1] = 0;
            }
            if (p5.pvar->description[0] == 0) {
                strncpy((char*)p5.pvar->description, "K_ALL", sizeof(p5.pvar->description)-1);
                p5.pvar->description[sizeof(p5.pvar->description)-1] = 0;
            }
            p5.pvar->range = OFF_ON;
            p5.pvar->digital_analog = 0;
            p5.pvar->control = 0;
        }
    }

    // VAR20..VAR23 -> put_io_buf(VAR,19..22) labels: HT_K1..HT_K4
    for (int s = 0; s < 4; ++s) {
        Str_points_ptr ph = put_io_buf(VAR, 19 + s);
        if (ph.pvar) {
            char lab[8];
            snprintf(lab, sizeof(lab), "HT_K%d", s+1);
            if (ph.pvar->label[0] == 0) {
                strncpy((char*)ph.pvar->label, lab, sizeof(ph.pvar->label)-1);
                ph.pvar->label[sizeof(ph.pvar->label)-1] = 0;
            }
            if (ph.pvar->description[0] == 0) {
                strncpy((char*)ph.pvar->description, lab, sizeof(ph.pvar->description)-1);
                ph.pvar->description[sizeof(ph.pvar->description)-1] = 0;
            }
        }
    }

    // Configure INT GPIO (edge: falling). If INT is level-held at startup, reading above should clear it.
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << int_gpio),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    esp_err_t r = gpio_config(&io_conf);
    ESP_LOGI(TAG, "gpio_config -> %d", r);
    r = gpio_install_isr_service(0);
    ESP_LOGI(TAG, "gpio_install_isr_service -> %d", r);
    r = gpio_isr_handler_add(int_gpio, tca_int_isr, NULL);
    ESP_LOGI(TAG, "gpio_isr_handler_add -> %d", r);

    int level = gpio_get_level(int_gpio);
    ESP_LOGI(TAG, "TCA9535 service started on INT GPIO %d (queue_len=%d) initial_level=%d", int_gpio, (int)queue_len, level);
    return ESP_OK;
}


tca9535_evt_queue_t tca9535_get_event_queue(void)
{
    return s_int_evt_queue;
}

void key_task(void)
{

    esp_err_t err = tca9535_init(I2C_PORT, SDA_GPIO, SCL_GPIO, TCA9535_ADDR, 100000);

	if (err != ESP_OK) {
		ESP_LOGE(TAG, "tca9535_init failed: %d", err);
		return;
	}

   // 1) Configure ports (example: port0 outputs, port1 inputs)
	err = tca9535_set_config(0x00, 0xFF);
	if(err != ESP_OK)
	{
		return;
	}
	// 2) Start internal service: ISR + internal tasks + queue
	err = tca9535_start_service(GPIO_NUM_34, 10); // queue length 10
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "tca9535_start_service failed: %d", err);
		return;
	}
	// 3) (Optional) create a user task to consume events from the same queue
   // xTaskCreate(user_event_task, "tca_user_evt", 3072, NULL, tskIDLE_PRIORITY+1, NULL);

}
