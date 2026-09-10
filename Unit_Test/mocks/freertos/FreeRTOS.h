#ifndef MOCK_FREERTOS_H
#define MOCK_FREERTOS_H

#include <stdint.h>
#include <stddef.h>

#define pdFALSE 0
#define pdTRUE 1
#define pdPASS 1
#define pdFAIL 0

typedef int32_t BaseType_t;
typedef uint32_t UBaseType_t;
typedef uint32_t TickType_t;

typedef void* TaskHandle_t;
typedef void* QueueHandle_t;
typedef void* EventGroupHandle_t;
typedef void* SemaphoreHandle_t;

typedef struct {
    int dummy;
} portMUX_TYPE;

#define portMUX_INITIALIZER_UNLOCKED {0}

#define configTICK_RATE_HZ 100
#define portTICK_PERIOD_MS (1000 / configTICK_RATE_HZ)
#define portMAX_DELAY (TickType_t)0xffffffffUL
#define pdMS_TO_TICKS(ms) ((TickType_t)((ms) / portTICK_PERIOD_MS))

#endif // MOCK_FREERTOS_H

#ifndef MOCK_FREERTOS_INCLUDES_H
#define MOCK_FREERTOS_INCLUDES_H
#include "semphr.h"
#include "event_groups.h"
#endif
