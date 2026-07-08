#ifndef ESP_EVENT_H
#define ESP_EVENT_H

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

typedef const char* esp_event_base_t;
typedef int32_t esp_event_id_t;
typedef void* esp_event_loop_handle_t;
typedef void* esp_event_handler_instance_t;
typedef void (*esp_event_handler_t)(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

#define ESP_EVENT_ANY_ID -1

static inline esp_err_t esp_event_loop_create_default(void) { return ESP_OK; }
static inline esp_err_t esp_event_handler_instance_register(esp_event_base_t event_base, int32_t event_id, esp_event_handler_t event_handler, void* event_handler_arg, esp_event_handler_instance_t* event_handler_instance) { (void)event_base; (void)event_id; (void)event_handler; (void)event_handler_arg; (void)event_handler_instance; return ESP_OK; }
#endif // ESP_EVENT_H
