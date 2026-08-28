#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "mdns.h"

#include "wifi.h"
#include "flash.h"
#include "wifi_web_server.h"

static const char *TAG = "WIFI_WEB";
static const char *WIFI_DIAG_TAG = "WIFI_DIAG";
static httpd_handle_t server = NULL;
static bool mdns_initialized = false;

// Small, minimal HTML webpage for Wi-Fi credentials configuration
static const char INDEX_HTML[] =
"<!DOCTYPE html><html><head><meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1.0'>"
"<title>T3 Wi-Fi Setup</title>"
"<style>"
"body{font-family:sans-serif;background:#121212;color:#fff;margin:0;padding:20px;display:flex;justify-content:center}"
".box{background:#1e1e1e;padding:20px;border-radius:10px;width:100%;max-width:320px;box-shadow:0 4px 10px rgba(0,0,0,0.5)}"
"h3{margin-top:0;color:#38bdf8;text-align:center}"
"label{font-size:12px;color:#aaa;display:block;margin-top:10px}"
"input,select{width:100%;padding:10px;margin-top:4px;background:#2a2a2a;border:1px solid #444;color:#fff;border-radius:5px;box-sizing:border-box}"
"button{width:100%;padding:10px;margin-top:15px;background:#38bdf8;border:none;color:#000;font-weight:bold;border-radius:5px;cursor:pointer}"
"button.scan{background:#444;color:#fff;margin-top:5px}"
"#status{margin-top:10px;font-size:13px;text-align:center;color:#4ade80}"
"</style></head><body>"
"<div class='box'>"
"<h3>T3 Wi-Fi Setup</h3>"
"<button class='scan' onclick='scan()'>Scan Wi-Fi</button>"
"<label>Available Networks</label>"
"<select id='net' onchange='document.getElementById(\"ssid\").value=this.value'><option value=''>-- Select Network --</option></select>"
"<form id='f' onsubmit='save(event)'>"
"<label>SSID</label><input type='text' id='ssid' name='ssid' required>"
"<label>Password</label><input type='password' id='pass' name='password'>"
"<button type='submit'>Save & Connect</button>"
"</form>"
"<div id='status'></div>"
"</div>"
"<script>"
"function scan(){"
"document.getElementById('status').innerText='Scanning...';"
"fetch('/scan').then(r=>r.json()).then(d=>{"
"let s=document.getElementById('net');s.innerHTML='<option value=\"\">-- Select Network --</option>';"
"if(d.networks){d.networks.forEach(n=>{let o=document.createElement('option');o.value=n;o.innerText=n;s.appendChild(o);});}"
"document.getElementById('status').innerText='Scan complete';"
"}).catch(()=>document.getElementById('status').innerText='Scan failed');"
"}"
"function save(e){"
"e.preventDefault();"
"document.getElementById('status').innerText='Saving...';"
"let body='ssid='+encodeURIComponent(document.getElementById('ssid').value)+'&password='+encodeURIComponent(document.getElementById('pass').value);"
"fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body})"
".then(r=>r.text()).then(t=>document.getElementById('status').innerText='Saved! Device rebooting...')"
".catch(()=>document.getElementById('status').innerText='Done');"
"}"
"</script></body></html>";

// Initialize mDNS service (accessible via http://tstat.local)
void init_mdns_service(void)
{
    if (mdns_initialized) return;

    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mDNS Init failed: %d", err);
        return;
    }

    mdns_hostname_set("tstat");
    mdns_instance_name_set("Temco TStat Controller");
    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);

    mdns_initialized = true;
    ESP_LOGE(TAG, "mDNS initialized. Hostname: http://tstat.local");
}

// Start SoftAP mode (SSID: T3_Admin, Password: T3_Admin)
esp_err_t wifi_start_softap(void)
{
    wifi_mode_t mode;
    if (esp_wifi_get_mode(&mode) == ESP_OK && (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA)) {
        return ESP_OK; // SoftAP already running
    }

    esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (!ap_netif) {
        ap_netif = esp_netif_create_default_wifi_ap();
    }

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = "T3_Admin",
            .password = "T3_Admin",
            .ssid_len = strlen("T3_Admin"),
            .channel = 1,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .max_connection = 4,
        },
    };

    ESP_LOGI(WIFI_DIAG_TAG, "before esp_wifi_set_mode mode=WIFI_MODE_APSTA");
    esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_APSTA);
    ESP_LOGI(WIFI_DIAG_TAG, "after esp_wifi_set_mode ret=%s mode=WIFI_MODE_APSTA", esp_err_to_name(ret));
    esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config);
    ESP_LOGI(TAG, "SoftAP started. SSID: T3_Admin, Password: T3_Admin");
    return ESP_OK;
}

// Save credentials and reboot
void wifi_web_save_credentials(const char *ssid, const char *password)
{
    if (!ssid || strlen(ssid) == 0) return;

    memset(SSID_Info.name, 0, sizeof(SSID_Info.name));
    memset(SSID_Info.password, 0, sizeof(SSID_Info.password));

    strncpy(SSID_Info.name, ssid, sizeof(SSID_Info.name) - 1);
    if (password) {
        strncpy(SSID_Info.password, password, sizeof(SSID_Info.password) - 1);
    }
    SSID_Info.MANUEL_EN = 1;

    save_wifi_info();
    ESP_LOGE(TAG, "Wi-Fi credentials saved. SSID: %s. Rebooting in 2s...", SSID_Info.name);

    // Delay then reboot so flash write completes
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
}

// GET /
static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

// GET /scan
static esp_err_t scan_get_handler(httpd_req_t *req)
{
    // Small delay to let Wi-Fi radio settle before first scan
    vTaskDelay(pdMS_TO_TICKS(500));

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true
    };

    esp_err_t scan_err = esp_wifi_scan_start(&scan_config, true);
    if (scan_err != ESP_OK) {
        // Retry once after a short delay
        vTaskDelay(pdMS_TO_TICKS(1000));
        scan_err = esp_wifi_scan_start(&scan_config, true);
    }

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    if (ap_count > 10) ap_count = 10;

    wifi_ap_record_t *ap_info = malloc(sizeof(wifi_ap_record_t) * (ap_count > 0 ? ap_count : 1));
    if (!ap_info) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    if (ap_count > 0) {
        esp_wifi_scan_get_ap_records(&ap_count, ap_info);
    }

    char *buf = malloc(1024);
    if (!buf) {
        free(ap_info);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    int offset = snprintf(buf, 1024, "{\"networks\":[");
    for (int i = 0; i < ap_count; i++) {
        offset += snprintf(buf + offset, 1024 - offset, "%s\"%s\"", (i == 0) ? "" : ",", (char*)ap_info[i].ssid);
    }
    snprintf(buf + offset, 1024 - offset, "]}");

    free(ap_info);

    httpd_resp_set_type(req, "application/json");
    esp_err_t res = httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
    free(buf);
    return res;
}

// POST /save
static esp_err_t save_post_handler(httpd_req_t *req)
{
    char content[256] = {0};
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char ssid[64] = {0};
    char password[32] = {0};

    char *s = strstr(content, "ssid=");
    if (s) {
        s += 5;
        char *end = strchr(s, '&');
        if (end) {
            int len = end - s;
            if (len > 63) len = 63;
            strncpy(ssid, s, len);
        } else {
            strncpy(ssid, s, 63);
        }
    }

    char *p = strstr(content, "password=");
    if (p) {
        p += 9;
        strncpy(password, p, 31);
    }

    // URL decode basic space '+'
    for (char *c = ssid; *c; c++) if (*c == '+') *c = ' ';
    for (char *c = password; *c; c++) if (*c == '+') *c = ' ';

    wifi_web_save_credentials(ssid, password);
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

// Start HTTP Web Server
esp_err_t wifi_web_server_start(void)
{
    if (server) return ESP_OK;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;
    config.lru_purge_enable = true;

    ESP_LOGE(TAG, "Starting HTTP Web Server on port %d", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root_uri = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = root_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &root_uri);

        httpd_uri_t scan_uri = {
            .uri       = "/scan",
            .method    = HTTP_GET,
            .handler   = scan_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &scan_uri);

        httpd_uri_t save_uri = {
            .uri       = "/save",
            .method    = HTTP_POST,
            .handler   = save_post_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &save_uri);

        return ESP_OK;
    }

    ESP_LOGE(TAG, "Failed to start HTTP Web Server!");
    return ESP_FAIL;
}

// Stop HTTP Web Server
void wifi_web_server_stop(void)
{
    if (server) {
        httpd_stop(server);
        server = NULL;
        ESP_LOGE(TAG, "HTTP Web Server stopped.");
    }
}
