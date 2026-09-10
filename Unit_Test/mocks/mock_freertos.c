#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

EventGroupHandle_t s_wifi_event_group = (EventGroupHandle_t)1;
TaskHandle_t main_task_handle[20] = {0};
