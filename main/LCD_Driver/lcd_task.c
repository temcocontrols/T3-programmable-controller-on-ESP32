#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include <string.h>
#include "lcd_drv.h"
#include "touch_drv.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "Menu.h"
#include "lvgl.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "TemcoScreen/ui.h"
#include "ud_str.h"
#include "controls.h"
#include "rtc.h"
#include "lv_UserPeram.h"

#define DISP_BUF_LINES 40
#define LCD_TASK_DELAY_MS 10

char debugbuf[100];

void my_disp_flush(lv_display_t  *disp,
                   const lv_area_t *area,
                   uint8_t *color_p)
{
    uint16_t w = area->x2 - area->x1 + 1;
    uint16_t h = area->y2 - area->y1 + 1;

    // memset(debugbuf, 0, sizeof(debugbuf));
    // sprintf(debugbuf,
    //         "FLUSH: w = %d, h = %d at (%d,%d)-(%d,%d)\r\n",
    //         w, h,
    //         area->x1, area->y1,
    //         area->x2, area->y2);
    // uart_write_bytes(UART_NUM_0, debugbuf, strlen(debugbuf));

    /* Send LVGL pixel buffer to LCD */
    LCD_WriteBitmap(
        area->x1,
        area->y1,
        w,
        h,
        color_p
    );

    lv_disp_flush_ready(disp);
}

void my_touch_read_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
    uint16_t x, y;
    bool touched = TouchGetInputs(&x, &y);

    // memset(debugbuf, 0, sizeof(debugbuf));
    // sprintf(debugbuf,
    //         "my_touch_read_cb: touched=%d x=%d y=%d\r\n",
    //         touched, x, y);
    // uart_write_bytes(UART_NUM_0, debugbuf, strlen(debugbuf));
    if (touched) {
        /* Adjust if needed */
        data->point.x = TFT_HOR_RES - y;
        data->point.y = x;
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

uint32_t my_get_millis(void)
{
    return esp_timer_get_time() / 1000;
}

void LCD_RegesterlvglDriver()
{
    lv_tick_set_cb(my_get_millis);

    lv_display_t * display = lv_display_create(TFT_HOR_RES, TFT_VER_RES);

    lv_display_set_default(display);

    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565_SWAPPED);
#if 1
    size_t fb_size = LCD_SPI_BUFFER_SIZE;

    uint16_t *buf1 = heap_caps_malloc(
        fb_size,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );

    assert(buf1);   // IMPORTANT

    lv_display_set_buffers(display, buf1, NULL, fb_size, LV_DISPLAY_RENDER_MODE_FULL);
#else

    size_t buf_size = LCD_SPI_BUFFER_SIZE/10;

    lv_color_t *buf1 = heap_caps_malloc(buf_size,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    lv_color_t *buf2 = heap_caps_malloc(buf_size,
            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    assert(buf1 && buf2);

    lv_display_set_buffers(display,
            buf1, buf2,
            buf_size,
            LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif
    uart_write_bytes(UART_NUM_0, "lvgl display created\r\n", strlen("lvgl display created\r\n"));
    // flush callback
    lv_display_set_flush_cb(display, my_disp_flush);

    /* Touch input device */
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touch_read_cb);

    uart_write_bytes(UART_NUM_0, "lvgl indev created\r\n", strlen("lvgl indev created\r\n"));

    uart_write_bytes(UART_NUM_0, "lvgl task started\r\n", strlen("lvgl task started\r\n"));
}

void print_all_ram_usage(void)
{
    size_t total, free;

    /* PSRAM */
    total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    free  = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    memset(debugbuf, 0, sizeof(debugbuf));
    sprintf(debugbuf,
            "PSRAM  total:%u bytes free:%u bytes\r\n",
            (unsigned)total, (unsigned)free);
    uart_write_bytes(UART_NUM_0, debugbuf, strlen(debugbuf));

    /* Internal RAM */
    total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    free  = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    memset(debugbuf, 0, sizeof(debugbuf));
    sprintf(debugbuf,
            "INT RAM total:%u bytes free:%u bytes\r\n",
            (unsigned)total, (unsigned)free);
    uart_write_bytes(UART_NUM_0, debugbuf, strlen(debugbuf));

    /* DMA-capable RAM */
    total = heap_caps_get_total_size(MALLOC_CAP_DMA);
    free  = heap_caps_get_free_size(MALLOC_CAP_DMA);
    memset(debugbuf, 0, sizeof(debugbuf));
    sprintf(debugbuf,
            "DMA RAM total:%u bytes free:%u bytes\r\n",
            (unsigned)total, (unsigned)free);
    uart_write_bytes(UART_NUM_0, debugbuf, strlen(debugbuf));

    /* Overall heap */
    memset(debugbuf, 0, sizeof(debugbuf));
    sprintf(debugbuf,
            "HEAP free:%u bytes min ever:%u bytes\r\n",
            (unsigned)esp_get_free_heap_size(),
            (unsigned)esp_get_minimum_free_heap_size());
    uart_write_bytes(UART_NUM_0, debugbuf, strlen(debugbuf));


    uint32_t flash_size = spi_flash_get_chip_size();
    memset(debugbuf,0,sizeof(debugbuf));
    sprintf(debugbuf,"Flash size: %u bytes\r\n", flash_size);
    uart_write_bytes(UART_NUM_0, debugbuf, strlen(debugbuf));

    uint32_t taskMemory = uxTaskGetStackHighWaterMark(NULL);
    memset(debugbuf,0,sizeof(debugbuf));
    sprintf(debugbuf,"Task free stack: %u bytes\r\n", taskMemory);
    uart_write_bytes(UART_NUM_0, debugbuf, strlen(debugbuf));
}

void Lcd_Task(void *pvParameters)
{
    // Initialize the LCD
    print_all_ram_usage();

    uart_write_bytes(UART_NUM_0, "Initializing LVGL...\r\n", strlen("Initializing LVGL...\r\n"));

    lv_init();

    uart_write_bytes(UART_NUM_0, "LVGL initialized\r\n", strlen("LVGL initialized\r\n"));

    LCD_RegesterlvglDriver();

    uart_write_bytes(UART_NUM_0, "LVGL driver registered\r\n", strlen("LVGL driver registered\r\n"));

    vTaskDelay(pdMS_TO_TICKS(2000));

    ui_init();

    lv_Init_UserParameters();

    uart_write_bytes(UART_NUM_0, "LVGL Loop Start..\r\n", strlen("LVGL Loop Start..\r\n"));
    while (1)
    {
        lv_timer_handler();
        lv_Lcd_UpdateData();

        vTaskDelay(pdMS_TO_TICKS(LCD_TASK_DELAY_MS));
    }
}

// End of File
