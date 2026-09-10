#ifndef ESP_LOG_H
#define ESP_LOG_H

#include <stdio.h>

#define ESP_LOGE(tag, format, ...) printf("[ERROR] [%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) printf("[WARN] [%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGI(tag, format, ...) printf("[INFO] [%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGD(tag, format, ...) printf("[DEBUG] [%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGV(tag, format, ...) printf("[VERBOSE] [%s] " format "\n", tag, ##__VA_ARGS__)

#define esp_log_write(...) (void)0

typedef enum {
    ESP_LOG_NONE,
    ESP_LOG_ERROR,
    ESP_LOG_WARN,
    ESP_LOG_INFO,
    ESP_LOG_DEBUG,
    ESP_LOG_VERBOSE
} esp_log_level_t;

static inline void esp_log_level_set(const char* tag, esp_log_level_t level) { (void)tag; (void)level; }

#endif // ESP_LOG_H
