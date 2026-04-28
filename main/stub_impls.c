/*
 * Stub implementations for excluded features
 * These functions provide minimal implementations to allow linking
 * when the full implementations have fatal compilation errors
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include "types.h"

/* Stub for LS_LED_Control - RGB LED strip control */
void LS_LED_Control(uint32_t* color)
{
    /* Stub implementation - does nothing */
    (void)color; /* Suppress unused parameter warning */
}

/* Stub for LS_led_task - FreeRTOS task for LED control */
void LS_led_task(void *pvParameters)
{
    /* Stub implementation - does nothing and returns */
    (void)pvParameters;
    vTaskDelete(NULL);
}

/* Stub for matter_light_init - Matter Light protocol initialization */
esp_err_t matter_light_init(void)
{
    /* Return success to indicate initialization is "complete" */
    return ESP_OK;
}

/* Stub for ethernet_init - Ethernet interface initialization */
esp_err_t ethernet_init(void)
{
    /* Return success (0) to indicate initialization */
    return ESP_OK;
}
