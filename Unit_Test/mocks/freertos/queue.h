#ifndef MOCK_FREERTOS_QUEUE_H
#define MOCK_FREERTOS_QUEUE_H

#include "FreeRTOS.h"

typedef void* QueueHandle_t;

static inline QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize)
{
    (void)uxQueueLength; (void)uxItemSize;
    return (QueueHandle_t)1;
}

static inline BaseType_t xQueueSend(QueueHandle_t xQueue, const void * const pvItemToQueue, TickType_t xTicksToWait)
{
    (void)xQueue; (void)pvItemToQueue; (void)xTicksToWait;
    return pdPASS;
}

static inline BaseType_t xQueueReceive(QueueHandle_t xQueue, void * const pvBuffer, TickType_t xTicksToWait)
{
    (void)xQueue; (void)pvBuffer; (void)xTicksToWait;
    return pdPASS;
}

static inline BaseType_t xQueuePeek(QueueHandle_t xQueue, void * const pvBuffer, TickType_t xTicksToWait)
{
    (void)xQueue; (void)pvBuffer; (void)xTicksToWait;
    return pdPASS;
}

static inline void vQueueDelete(QueueHandle_t xQueue) { (void)xQueue; }

#endif // MOCK_FREERTOS_QUEUE_H
