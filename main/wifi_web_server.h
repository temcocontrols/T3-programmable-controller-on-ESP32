#ifndef WIFI_WEB_SERVER_H
#define WIFI_WEB_SERVER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize mDNS service (accessible via http://tstat.local)
void init_mdns_service(void);

// Start SoftAP mode (SSID: T3_Admin, Pass: T3_Admin)
esp_err_t wifi_start_softap(void);

// Start HTTP Web Server on port 80
esp_err_t wifi_web_server_start(void);

// Stop HTTP Web Server
void wifi_web_server_stop(void);

// Save new Wi-Fi credentials and attempt connection
void wifi_web_save_credentials(const char *ssid, const char *password);

#ifdef __cplusplus
}
#endif

#endif // WIFI_WEB_SERVER_H
