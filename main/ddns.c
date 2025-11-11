/*
 * ddns.c
 *
 *  Created on: 2025年8月12日
 *      Author: Administrator
 */


#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mbedtls/base64.h"
#include <netdb.h>

#include "wifi.h"
#include "driver/uart.h"
extern uint16_t Test[50];




#if DDNS

#define TAG "DDNS"

// 动态 DNS 配置
//#define DDNS_UPDATE_URL "http://dynupdate.no-ip.com/nic/update?hostname=yourhostname.ddns.net"
//#define DDNS_UPDATE_URL "https://dynupdate.no-ip.com/nic/update?hostname=yourhostname.ddns.net"
#define DDNS_UPDATE_URL "http://dynupdate.no-ip.com/nic/update?hostname=yourhostname.ddns.net&myip=1.2.3.4"
#define DDNS_USERNAME "57440569@qq.com" // 替换为你的用户名
#define DDNS_PASSWORD "903000lwh" // 替换为你的密码



// 函数：解析域名并获取 IP 地址
void resolve_domain_to_ip(const char *domain_name, char *ip_address, size_t ip_len) {
    struct addrinfo hints;
    struct addrinfo *res;
    struct sockaddr_in *addr;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // 仅解析 IPv4 地址
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(domain_name, NULL, &hints, &res);
    if (err != 0 || res == NULL) {
      //  ESP_LOGE(TAG, "DNS lookup failed for %s: %s", domain_name, gai_strerror(err));
        return;
    }

    addr = (struct sockaddr_in *)res->ai_addr;
    inet_ntop(AF_INET, &(addr->sin_addr), ip_address, ip_len);

    ESP_LOGI(TAG, "Resolved IP for %s: %s", domain_name, ip_address);
#if 1
    sprintf(debug_array,"resolve_domain_to_ip %s: %s\r", domain_name, ip_address);
    uart_write_bytes(UART_NUM_0, (const char *)debug_array, strlen(debug_array));
#endif
    freeaddrinfo(res); // 释放内存
}


void dns_lookup_lwip_gethostbyname(const char *domain_name) {
    struct hostent *he;
    struct in_addr **addr_list;

    he = lwip_gethostbyname(domain_name);
    if (he == NULL) {
        ESP_LOGE(TAG, "DNS lookup failed for %s", domain_name);
        return;
    }

    addr_list = (struct in_addr **)he->h_addr_list;

    // 打印所有解析到的 IP 地址
    for (int i = 0; addr_list[i] != NULL; i++) {
        ESP_LOGI(TAG, "Resolved IP: %s", inet_ntoa(*addr_list[i]));
#if 0
    sprintf(debug_array,"Resolved IP: %s", inet_ntoa(*addr_list[i]));
    uart_write_bytes(UART_NUM_0, (const char *)debug_array, strlen(debug_array));
#endif
    }
}

// 更新动态 DNS 的函数
void update_ddns(const char *ip_address) {
    char url[256];
    snprintf(url, sizeof(url), "%s&myip=%s", DDNS_UPDATE_URL, ip_address);
    const char *domain_name = "temcocontrols.ddns.net";
    esp_http_client_config_t config = {
        .url = url,
        .username = DDNS_USERNAME,
        .password = DDNS_PASSWORD,
		.auth_type = HTTP_AUTH_TYPE_BASIC, // 使用 HTTP Basic Authentication
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    Test[10]++;
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) { Test[11]++;
    Test[12] = esp_http_client_get_status_code(client);
 /*       ESP_LOGI(TAG, "DDNS update successful, HTTP status = %d",
                 esp_http_client_get_status_code(client));*/
    } else { Test[31]++;
        ESP_LOGE(TAG, "DDNS update failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);

    dns_lookup_lwip_gethostbyname(domain_name);
}

/*
void update_ddns1() {
    char auth_header[120];
    char auth_data[64];
    size_t auth_len;

    // 构造 "username:password" 格式的字符串
    snprintf(auth_data, sizeof(auth_data), "%s:%s", DDNS_USERNAME, DDNS_PASSWORD);

    // 对 "username:password" 进行 Base64 编码
    mbedtls_base64_encode((unsigned char *)auth_header, sizeof(auth_header), &auth_len,
                          (unsigned char *)auth_data, strlen(auth_data));

    // 添加 "Basic " 前缀
    char authorization[256];
    snprintf(authorization, sizeof(authorization), "Basic %s", auth_header);

    // 配置 HTTP 客户端
    esp_http_client_config_t config = {
        .url = DDNS_UPDATE_URL,
        .method = HTTP_METHOD_GET,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // 设置 Authorization 头
    esp_http_client_set_header(client, "Authorization", authorization);
    Test[10]++;
    // 发送请求
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {Test[11] = 10;
        ESP_LOGI(TAG, "DDNS update successful, HTTP status = %d",
                 esp_http_client_get_status_code(client));
    } else {Test[11] = 20;
        ESP_LOGE(TAG, "DDNS update failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}
*/

// 获取外部 IP 地址的函数（示例）
void get_external_ip(char *ip_address, size_t len) {
    // 示例：假设外部服务返回设备的外部 IP 地址
    // 你可以使用类似 "http://api.ipify.org" 的服务
    esp_http_client_config_t config = {
        .url = "http://api.ipify.org",
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    Test[12]++;
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {Test[13] = 10;
        int content_length = esp_http_client_get_content_length(client);
        if (content_length > 0 && content_length < len) {Test[13] = 10;
            esp_http_client_read(client, ip_address, content_length);
            ip_address[content_length] = '\0'; // 确保字符串以 NULL 结尾

        }
    } else {Test[13] = 20;
        ESP_LOGE(TAG, "Failed to get external IP: %s", esp_err_to_name(err));
#if 1
    sprintf(debug_array,"Failed to get external IP: %s", esp_err_to_name(err));
    uart_write_bytes(UART_NUM_0, (const char *)debug_array, strlen(debug_array));
#endif

    }

    esp_http_client_cleanup(client);
}
void uart_init(uint8_t uart);
// 动态 DNS 任务
void ddns_task(void *pvParameters) {
	char ip_address[64];// = "114.86.81.158";
    Test[10] = 0;
    while (1) {
        // 获取外部 IP 地址
        //get_external_ip(ip_address, sizeof(ip_address));
        //ESP_LOGI(TAG, "External IP: %s", ip_address);

        // 更新动态 DNS
        update_ddns(ip_address);

        memcpy(&Test[20],&ip_address,20);
#if 0
    sprintf(debug_array,"DNS: ip address %s, end",ip_address);
    uart_write_bytes(UART_NUM_0, (const char *)debug_array, strlen(debug_array));
#endif
        // 每 10 分钟更新一次
        vTaskDelay(pdMS_TO_TICKS(600000));
    }
}

//void app_main(void) {
    // 启动动态 DNS 任务
    //xTaskCreate(ddns_task, "ddns_task", 4096, NULL, 5, NULL);
//}

#endif
