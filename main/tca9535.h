// TCA9535 simple driver for ESP32 (I2C)
#pragma once
#include "driver/i2c.h"
#include "esp_err.h"
#include <stdint.h>


// Event codes placed on the driver queue
#define TCA_EVT_INT_FALL     1
#define TCA_EVT_BUTTON_BASE  0x100  // button events: TCA_EVT_BUTTON_BASE + button_index (0..9)
/**
 * Initialize I2C for TCA9535 usage.
 * @param i2c_num I2C port (I2C_NUM_0 or I2C_NUM_1)
 * @param sda_pin SDA GPIO number
 * @param scl_pin SCL GPIO number
 * @param addr 7-bit I2C address of the TCA9535
 * @param clk_speed_hz I2C clock speed (e.g., 100000)
 */
esp_err_t tca9535_init(i2c_port_t i2c_num, gpio_num_t sda_pin, gpio_num_t scl_pin, uint8_t addr, int clk_speed_hz);

// Write both output registers (port0, port1)
esp_err_t tca9535_write_outputs(uint8_t out0, uint8_t out1);

// Read both input registers (port0, port1)
esp_err_t tca9535_read_inputs(uint8_t *in0, uint8_t *in1);

// Set configuration registers (1 = input, 0 = output)
esp_err_t tca9535_set_config(uint8_t cfg0, uint8_t cfg1);

// Get address used by driver
uint8_t tca9535_get_address(void);

// FreeRTOS event queue exposure
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
typedef QueueHandle_t tca9535_evt_queue_t;

// Start internal service: creates event queue, ISR and background tasks.
// int_gpio: GPIO number connected to TCA9535 INT (input-only pins are supported)
// queue_len: length of internal event queue
esp_err_t tca9535_start_service(gpio_num_t int_gpio, size_t queue_len);

// Get the internal event queue so main can create additional tasks that consume events
tca9535_evt_queue_t tca9535_get_event_queue(void);

void key_task(void);
