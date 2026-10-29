#pragma once
#include <stdint.h>
#include <assert.h>
typedef uint32_t TickType_t;
typedef int BaseType_t;
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(lock) do { assert(*(lock) == 0); ++*(lock); } while (0)
#define portEXIT_CRITICAL(lock) do { assert(*(lock) == 1); --*(lock); } while (0)
#define pdTRUE 1
#define pdFALSE 0
#define pdPASS 1
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
#define portMAX_DELAY UINT32_MAX
