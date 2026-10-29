#pragma once
#include "FreeRTOS.h"
typedef void *TaskHandle_t;
TickType_t xTaskGetTickCount(void);
void vTaskDelay(TickType_t ticks);
BaseType_t xTaskCreate(void (*task)(void *), const char *name, unsigned stack,
                      void *arg, unsigned priority, TaskHandle_t *handle);
