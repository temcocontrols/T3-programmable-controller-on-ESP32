#ifndef MOCK_ESP_ERR_H
#define MOCK_ESP_ERR_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "esp_heap_caps.h"
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

void esp_restart(void);

#ifndef ESP_OK
#define ESP_OK 0
#endif

#ifndef ESP_FAIL
#define ESP_FAIL -1
#endif

#ifndef ESP_ERR_NO_MEM
#define ESP_ERR_NO_MEM 0x101
#endif

#ifndef ESP_ERR_INVALID_ARG
#define ESP_ERR_INVALID_ARG 0x102
#endif

#ifndef ESP_ERR_INVALID_STATE
#define ESP_ERR_INVALID_STATE 0x103
#endif

#ifndef ESP_ERR_INVALID_SIZE
#define ESP_ERR_INVALID_SIZE 0x104
#endif

#ifndef ESP_ERR_NOT_FOUND
#define ESP_ERR_NOT_FOUND 0x105
#endif

#ifndef ESP_ERR_NOT_SUPPORTED
#define ESP_ERR_NOT_SUPPORTED 0x106
#endif

#ifndef ESP_ERR_TIMEOUT
#define ESP_ERR_TIMEOUT 0x107
#endif

#ifndef ESP_ERR_INVALID_RESPONSE
#define ESP_ERR_INVALID_RESPONSE 0x108
#endif

#ifndef ESP_ERR_INVALID_CRC
#define ESP_ERR_INVALID_CRC 0x109
#endif

#ifndef ESP_ERR_INVALID_VERSION
#define ESP_ERR_INVALID_VERSION 0x10A
#endif

#ifndef ESP_ERR_INVALID_MAC
#define ESP_ERR_INVALID_MAC 0x10B
#endif

typedef int32_t esp_err_t;

#ifndef ESP_ERROR_CHECK
#define ESP_ERROR_CHECK(x) do { (void)(x); } while(0)
#endif

static inline const char *esp_err_to_name(esp_err_t code) { (void)code; return "MOCK_ERR"; }

#endif // MOCK_ESP_ERR_H
