#ifndef MOCK_FREERTOS_SEMPHR_H
#define MOCK_FREERTOS_SEMPHR_H

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* SemaphoreHandle_t;

static inline SemaphoreHandle_t xSemaphoreCreateMutex(void)
{
    return (SemaphoreHandle_t)1;
}

static inline SemaphoreHandle_t xSemaphoreCreateCounting(UBaseType_t uxMaxCount, UBaseType_t uxInitialCount)
{
    (void)uxMaxCount; (void)uxInitialCount;
    return (SemaphoreHandle_t)1;
}

static inline BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait)
{
    (void)xSemaphore; (void)xTicksToWait;
    return pdPASS;
}

static inline BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore)
{
    (void)xSemaphore;
    return pdPASS;
}

static inline void vSemaphoreDelete(SemaphoreHandle_t xSemaphore) { (void)xSemaphore; }

static inline UBaseType_t uxSemaphoreGetCount(SemaphoreHandle_t xSemaphore)
{
    (void)xSemaphore;
    return 1;
}

static inline SemaphoreHandle_t xSemaphoreCreateBinary(void)
{
    return (SemaphoreHandle_t)1;
}

static inline BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken)
{
    (void)xSemaphore; (void)pxHigherPriorityTaskWoken;
    return pdPASS;
}

static inline BaseType_t xSemaphoreTakeFromISR(SemaphoreHandle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken)
{
    (void)xSemaphore; (void)pxHigherPriorityTaskWoken;
    return pdPASS;
}

#ifdef __cplusplus
}
#endif

#endif // MOCK_FREERTOS_SEMPHR_H
