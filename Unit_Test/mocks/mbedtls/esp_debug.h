#ifndef MBEDTLS_ESP_DEBUG_H
#define MBEDTLS_ESP_DEBUG_H

#include "mbedtls/ssl.h"

void mbedtls_esp_enable_debug_log(mbedtls_ssl_config *conf, int threshold);

#endif // MBEDTLS_ESP_DEBUG_H
