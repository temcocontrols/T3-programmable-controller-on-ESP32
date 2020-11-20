#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "microphone.h"

#define SAMPLE_RATE     (48000)
#define I2S_NUM         (0)
#define WAVE_FREQ_HZ    (100)
#define I2S_BCK_IO      (GPIO_NUM_15)
#define I2S_WS_IO       (GPIO_NUM_6)
#define I2S_DO_IO       (-1)
#define I2S_DI_IO       (GPIO_NUM_13)
#define I2S_LR_IO		(GPIO_NUM_7)

void microphone_init()
{
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,                                  // Only TX
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = 24,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           //2-channels
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .dma_buf_count = 6,
        .dma_buf_len = 60,
        .use_apll = false,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1                                //Interrupt level 1
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_IO,
        .ws_io_num = I2S_WS_IO,
        .data_out_num = I2S_DO_IO,
        .data_in_num = I2S_DI_IO                                               //Not used
    };
    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);
}
