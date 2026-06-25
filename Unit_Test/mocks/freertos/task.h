#ifndef MOCK_FREERTOS_TASK_H
#define MOCK_FREERTOS_TASK_H

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* TaskHandle_t;
typedef void (*TaskFunction_t)(void*);

#define tskIDLE_PRIORITY 0

static inline BaseType_t xTaskCreate(TaskFunction_t pvTaskCode, const char * const pcName, const uint32_t usStackDepth, void * const pvParameters, UBaseType_t uxPriority, TaskHandle_t * const pxCreatedTask)
{
    (void)pvTaskCode; (void)pcName; (void)usStackDepth; (void)pvParameters; (void)uxPriority; (void)pxCreatedTask;
    return pdPASS;
}

static inline BaseType_t xTaskCreatePinnedToCore(TaskFunction_t pvTaskCode, const char * const pcName, const uint32_t usStackDepth, void * const pvParameters, UBaseType_t uxPriority, TaskHandle_t * const pxCreatedTask, const BaseType_t xCoreID)
{
    (void)pvTaskCode; (void)pcName; (void)usStackDepth; (void)pvParameters; (void)uxPriority; (void)pxCreatedTask; (void)xCoreID;
    return pdPASS;
}

static inline void vTaskDelay(const TickType_t xTicksToDelay) { (void)xTicksToDelay; }
static inline void vTaskDelete(TaskHandle_t xTaskToDelete) { (void)xTaskToDelete; }
static inline TickType_t xTaskGetTickCount(void) { return 0; }
static inline void vTaskDelayUntil(TickType_t * const pxPreviousWakeTime, const TickType_t xTimeIncrement) { (void)pxPreviousWakeTime; (void)xTimeIncrement; }
static inline UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t xTask) { (void)xTask; return 2048; }

#define taskYIELD() do {} while(0)

#ifdef __cplusplus
}
#endif

#endif // MOCK_FREERTOS_TASK_H
