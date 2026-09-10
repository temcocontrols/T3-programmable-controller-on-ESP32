#ifndef ESP_SNTP_H
#define ESP_SNTP_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SNTP_OPMODE_POLL,
    SNTP_OPMODE_LISTENONLY
} esp_sntp_operatingmode_t;

typedef enum {
    SNTP_SYNC_MODE_IMMED,
    SNTP_SYNC_MODE_SMOOTH
} esp_sntp_sync_mode_t;

typedef void (*esp_sntp_time_cb_t)(struct timeval *tv);

void esp_sntp_init(void);
void esp_sntp_stop(void);
bool esp_sntp_enabled(void);
void esp_sntp_setoperatingmode(esp_sntp_operatingmode_t operating_mode);
void esp_sntp_setservername(uint8_t idx, const char *server_name);
void esp_sntp_set_time_sync_notification_cb(esp_sntp_time_cb_t callback);
void esp_sntp_set_sync_mode(esp_sntp_sync_mode_t sync_mode);

#ifdef __cplusplus
}
#endif

#endif // ESP_SNTP_H
