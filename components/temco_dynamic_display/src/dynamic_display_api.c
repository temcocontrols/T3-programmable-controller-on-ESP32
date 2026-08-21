#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "cJSON.h"

#include "dynamic_display_api.h"
#include "screen_flash_store.h"

#define TAG "DynDisplayAPI"
#define MAX_SCREENS_LIST 32
#define MAX_IMAGES_LIST 64

static httpd_handle_t s_server = NULL;
static bool s_started = false;

static void set_cors_headers(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
}

static esp_err_t send_json_str(httpd_req_t *req, const char *body)
{
    set_cors_headers(req);
    return httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t send_error(httpd_req_t *req, int status_code, const char *message)
{
    char body[256];
    snprintf(body, sizeof(body), "{\"status\":\"error\",\"message\":\"%s\"}", message ? message : "request failed");
    set_cors_headers(req);
    if (status_code == 404) {
        httpd_resp_set_status(req, "404 Not Found");
    } else if (status_code == 400) {
        httpd_resp_set_status(req, "400 Bad Request");
    } else if (status_code == 503) {
        httpd_resp_set_status(req, "503 Service Unavailable");
    } else {
        httpd_resp_set_status(req, "500 Internal Server Error");
    }
    return httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t read_body_dynamic(httpd_req_t *req, char **out_buf, size_t *out_len)
{
    size_t content_len = req->content_len;
    if (content_len == 0) {
        *out_buf = NULL;
        if (out_len) *out_len = 0;
        return ESP_OK;
    }

    if (content_len > 512 * 1024) {
        ESP_LOGE(TAG, "Request payload too large (%u bytes)", (unsigned)content_len);
        return ESP_ERR_NO_MEM;
    }

    char *buf = (char *)malloc(content_len + 1);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate memory for request body (%u bytes)", (unsigned)content_len);
        return ESP_ERR_NO_MEM;
    }

    size_t received = 0;
    while (received < content_len) {
        int ret = httpd_req_recv(req, buf + received, content_len - received);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            free(buf);
            ESP_LOGE(TAG, "Socket error receiving body at %u/%u", (unsigned)received, (unsigned)content_len);
            return ESP_FAIL;
        }
        received += ret;
    }

    buf[received] = '\0';
    *out_buf = buf;
    if (out_len) *out_len = received;
    return ESP_OK;
}

static const char *extract_last_path_component(const char *uri, const char *prefix)
{
    if (!uri) return NULL;
    const char *p = strstr(uri, prefix);
    if (p) {
        p += strlen(prefix);
        if (*p == '/') p++;
        // If there's a trailing query parameter, cut it off conceptually by searching next char
        return p;
    }
    // Fallback: get after last slash
    p = strrchr(uri, '/');
    if (p && *(p + 1) != '\0') {
        return p + 1;
    }
    return NULL;
}

static void clean_name(char *dst, const char *src, size_t max_len)
{
    if (!src) {
        dst[0] = '\0';
        return;
    }
    size_t i = 0;
    while (src[i] != '\0' && src[i] != '?' && src[i] != '/' && i < max_len - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

// --------------------------------------------------------------------------
// REST Handlers
// --------------------------------------------------------------------------

static esp_err_t info_handler(httpd_req_t *req)
{
    char screen_names[MAX_SCREENS_LIST][64];
    size_t screen_count = 0;
    screen_store_list_screens(screen_names, MAX_SCREENS_LIST, &screen_count);

    char image_names[MAX_IMAGES_LIST][64];
    size_t image_count = 0;
    screen_store_list_images(image_names, MAX_IMAGES_LIST, &image_count);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "panel_name", "T3-ESP32");
    cJSON_AddNumberToObject(root, "serial_number", 0);

    cJSON *sz = cJSON_CreateObject();
    cJSON_AddNumberToObject(sz, "width", 480);
    cJSON_AddNumberToObject(sz, "height", 320);
    cJSON_AddItemToObject(root, "screen_size", sz);

    cJSON_AddNumberToObject(root, "screen_count", screen_count);

    cJSON *scr_arr = cJSON_CreateArray();
    for (size_t i = 0; i < screen_count; i++) {
        cJSON_AddItemToArray(scr_arr, cJSON_CreateString(screen_names[i]));
    }
    cJSON_AddItemToObject(root, "screens", scr_arr);

    cJSON_AddNumberToObject(root, "image_count", image_count);
    cJSON *img_arr = cJSON_CreateArray();
    for (size_t i = 0; i < image_count; i++) {
        cJSON_AddItemToArray(img_arr, cJSON_CreateString(image_names[i]));
    }
    cJSON_AddItemToObject(root, "images", img_arr);

    cJSON_AddNumberToObject(root, "font_count", 7);
    cJSON *fnt_arr = cJSON_CreateArray();
    cJSON_AddItemToArray(fnt_arr, cJSON_CreateString("lv_font_montserrat_10"));
    cJSON_AddItemToArray(fnt_arr, cJSON_CreateString("lv_font_montserrat_16"));
    cJSON_AddItemToArray(fnt_arr, cJSON_CreateString("lv_font_montserrat_18"));
    cJSON_AddItemToArray(fnt_arr, cJSON_CreateString("lv_font_montserrat_30"));
    cJSON_AddItemToArray(fnt_arr, cJSON_CreateString("lv_font_montserrat_36"));
    cJSON_AddItemToArray(fnt_arr, cJSON_CreateString("lv_font_montserrat_40"));
    cJSON_AddItemToArray(fnt_arr, cJSON_CreateString("Arial80"));
    cJSON_AddItemToObject(root, "fonts", fnt_arr);

    cJSON_AddStringToObject(root, "firmware_version", "1.0.0");
    cJSON_AddStringToObject(root, "lvgl_version", "9.1.0");
    cJSON_AddBoolToObject(root, "dark_theme", true);
    cJSON_AddStringToObject(root, "color_format", "RGB565");

    char *json_out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    esp_err_t res = send_json_str(req, json_out ? json_out : "{}");
    if (json_out) free(json_out);
    return res;
}

static esp_err_t screens_get_all_handler(httpd_req_t *req)
{
    char screen_names[MAX_SCREENS_LIST][64];
    size_t count = 0;
    screen_store_list_screens(screen_names, MAX_SCREENS_LIST, &count);

    set_cors_headers(req);
    httpd_resp_send_chunk(req, "{\"screens\":[", HTTPD_RESP_USE_STRLEN);

    for (size_t i = 0; i < count; i++) {
        char *raw_json = NULL;
        size_t raw_len = 0;

        if (screen_store_get_screen(screen_names[i], &raw_json, &raw_len) == ESP_OK && raw_json) {

            char head[128];
            snprintf(head, sizeof(head), "%s{\"name\":\"%s\",\"json\":", (i > 0) ? "," : "", screen_names[i]);

            esp_err_t ret = httpd_resp_send_chunk(req, head, HTTPD_RESP_USE_STRLEN);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Header send failed at screen %s: %s", screen_names[i], esp_err_to_name(ret));
                free(raw_json);
                return ret;
            }

            // --- FAST-PATH CHECK GOES HERE ---
            char wrapper_prefix[80];
            snprintf(wrapper_prefix, sizeof(wrapper_prefix), "{\"%s\":", screen_names[i]);

            if (strncmp(raw_json, wrapper_prefix, strlen(wrapper_prefix)) == 0) {
                // Slow path: only for the double-wrapped screen(s), e.g. schedule_screen
                cJSON *parsed = cJSON_Parse(raw_json);
                cJSON *inner_json = NULL;

                if (parsed) {
                    cJSON *wrapper = cJSON_GetObjectItem(parsed, screen_names[i]);
                    inner_json = (wrapper && wrapper->type == cJSON_Object) ? wrapper : parsed;
                }

                char *inner_str = inner_json ? cJSON_PrintUnformatted(inner_json) : strdup(raw_json);

                ret = httpd_resp_send_chunk(req, inner_str ? inner_str : "{}", HTTPD_RESP_USE_STRLEN);
                if (inner_str) free(inner_str);
                if (parsed) cJSON_Delete(parsed);
            } else {
                // Fast path: stream raw_json straight through, no cJSON overhead at all
                ret = httpd_resp_send_chunk(req, raw_json, HTTPD_RESP_USE_STRLEN);
            }

            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Body send failed at screen %s: %s", screen_names[i], esp_err_to_name(ret));
                free(raw_json);
                return ret;
            }

            ret = httpd_resp_send_chunk(req, "}", 1);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Closing brace send failed at screen %s: %s", screen_names[i], esp_err_to_name(ret));
                free(raw_json);
                return ret;
            }

            free(raw_json);
        }
    }

    httpd_resp_send_chunk(req, "],\"meta\":{\"panel_name\":\"T3-ESP32\",\"serial_number\":0}}", HTTPD_RESP_USE_STRLEN);
    return httpd_resp_send_chunk(req, NULL, 0);
}

static esp_err_t screen_get_one_handler(httpd_req_t *req)
{
    char name[64];
    const char *raw_name = extract_last_path_component(req->uri, "/api/eez-device/screens/");
    clean_name(name, raw_name, sizeof(name));

    if (name[0] == '\0') {
        return send_error(req, 400, "Screen name missing");
    }

    char *raw_json = NULL;
    size_t raw_len = 0;
    esp_err_t err = screen_store_get_screen(name, &raw_json, &raw_len);
    if (err != ESP_OK || !raw_json) {
        return send_error(req, 404, "Screen not found");
    }

    cJSON *parsed = cJSON_Parse(raw_json);
    cJSON *inner_json = NULL;
    if (parsed) {
        cJSON *wrapper = cJSON_GetObjectItem(parsed, name);
        if (wrapper && wrapper->type == cJSON_Object) {
            inner_json = wrapper;
        } else {
            inner_json = parsed;
        }
    }

    char *inner_str = inner_json ? cJSON_PrintUnformatted(inner_json) : strdup(raw_json);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "name", name);
    cJSON *val = cJSON_Parse(inner_str ? inner_str : "{}");
    cJSON_AddItemToObject(resp, "json", val ? val : cJSON_CreateObject());

    char *resp_str = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    if (parsed) cJSON_Delete(parsed);
    if (inner_str) free(inner_str);
    free(raw_json);

    esp_err_t ret = send_json_str(req, resp_str ? resp_str : "{}");
    if (resp_str) free(resp_str);
    return ret;
}

static esp_err_t screens_put_all_handler(httpd_req_t *req)
{
    char *body = NULL;
    size_t len = 0;
    if (read_body_dynamic(req, &body, &len) != ESP_OK || !body) {
        return send_error(req, 400, "Failed to read body");
    }

    cJSON *root = cJSON_Parse(body);
    free(body);
    if (!root) {
        return send_error(req, 400, "Invalid JSON payload");
    }

    cJSON *screens_arr = cJSON_GetObjectItem(root, "screens");
    int deployed = 0;
    int failed = 0;

    if (screens_arr && cJSON_IsArray(screens_arr)) {
        int sz = cJSON_GetArraySize(screens_arr);
        for (int i = 0; i < sz; i++) {
            cJSON *item = cJSON_GetArrayItem(screens_arr, i);
            cJSON *name_obj = cJSON_GetObjectItem(item, "name");
            cJSON *json_obj = cJSON_GetObjectItem(item, "json");

            if (name_obj && name_obj->valuestring && json_obj) {
                char *json_str = cJSON_PrintUnformatted(json_obj);
                if (json_str) {
                    if (screen_store_save_screen(name_obj->valuestring, json_str, strlen(json_str)) == ESP_OK) {
                        deployed++;
                    } else {
                        failed++;
                    }
                    free(json_str);
                } else {
                    failed++;
                }
            } else {
                failed++;
            }
        }
    }

    cJSON_Delete(root);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "deployed", deployed);
    cJSON_AddNumberToObject(resp, "failed", failed);
    cJSON_AddStringToObject(resp, "status", failed == 0 ? "ok" : "partial");
    cJSON_AddNullToObject(resp, "errors");

    char *resp_str = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);

    esp_err_t ret = send_json_str(req, resp_str ? resp_str : "{}");
    if (resp_str) free(resp_str);
    return ret;
}

static esp_err_t single_screen_put_handler(httpd_req_t *req)
{
    char name[64];
    const char *raw_name = extract_last_path_component(req->uri, "/api/eez-device/screens/");
    clean_name(name, raw_name, sizeof(name));

    if (name[0] == '\0') {
        return send_error(req, 400, "Screen name missing");
    }

    char *body = NULL;
    size_t len = 0;
    if (read_body_dynamic(req, &body, &len) != ESP_OK || !body) {
        return send_error(req, 400, "Failed to read body");
    }

    cJSON *root = cJSON_Parse(body);
    free(body);

    const char *save_data = NULL;
    char *allocated_save_data = NULL;

    if (root) {
        cJSON *json_obj = cJSON_GetObjectItem(root, "json");
        if (json_obj) {
            allocated_save_data = cJSON_PrintUnformatted(json_obj);
            save_data = allocated_save_data;
        } else {
            allocated_save_data = cJSON_PrintUnformatted(root);
            save_data = allocated_save_data;
        }
    }

    esp_err_t err = ESP_FAIL;
    if (save_data) {
        err = screen_store_save_screen(name, save_data, strlen(save_data));
    }

    if (allocated_save_data) free(allocated_save_data);
    if (root) cJSON_Delete(root);

    if (err != ESP_OK) {
        return send_error(req, 500, "Failed to save screen to flash");
    }

    char resp[128];
    snprintf(resp, sizeof(resp), "{\"name\":\"%s\",\"status\":\"ok\",\"error\":null}", name);
    return send_json_str(req, resp);
}

static esp_err_t screen_patch_handler(httpd_req_t *req)
{
    char name[64];
    const char *raw_name = extract_last_path_component(req->uri, "/api/eez-device/screens/");
    clean_name(name, raw_name, sizeof(name));

    if (name[0] == '\0') {
        return send_error(req, 400, "Screen name missing");
    }

    char *body = NULL;
    size_t len = 0;
    if (read_body_dynamic(req, &body, &len) != ESP_OK || !body) {
        return send_error(req, 400, "Failed to read body");
    }

    cJSON *patch_root = cJSON_Parse(body);
    free(body);
    if (!patch_root) {
        return send_error(req, 400, "Invalid patch JSON");
    }

    char *raw_screen = NULL;
    size_t screen_len = 0;
    if (screen_store_get_screen(name, &raw_screen, &screen_len) != ESP_OK || !raw_screen) {
        cJSON_Delete(patch_root);
        return send_error(req, 404, "Target screen not found");
    }

    cJSON *screen_root = cJSON_Parse(raw_screen);
    free(raw_screen);
    if (!screen_root) {
        cJSON_Delete(patch_root);
        return send_error(req, 500, "Corrupted stored screen JSON");
    }

    int applied = 0;
    int rejected = 0;

    cJSON *changes = cJSON_GetObjectItem(patch_root, "changes");
    if (changes && cJSON_IsArray(changes)) {
        int sz = cJSON_GetArraySize(changes);
        for (int i = 0; i < sz; i++) {
            cJSON *ch = cJSON_GetArrayItem(changes, i);
            cJSON *path_obj = cJSON_GetObjectItem(ch, "path");
            cJSON *val_obj = cJSON_GetObjectItem(ch, "value");

            if (path_obj && path_obj->valuestring && val_obj) {
                // Apply dot-path update
                char path_buf[128];
                strncpy(path_buf, path_obj->valuestring, sizeof(path_buf) - 1);
                path_buf[sizeof(path_buf) - 1] = '\0';

                cJSON *curr = screen_root;
                char *token = strtok(path_buf, ".");
                char *last_token = NULL;

                while (token != NULL) {
                    last_token = token;
                    char *next_token = strtok(NULL, ".");
                    if (next_token == NULL) {
                        // Found leaf key
                        cJSON_DeleteItemFromObject(curr, last_token);
                        cJSON_AddItemToObject(curr, last_token, cJSON_Duplicate(val_obj, true));
                        applied++;
                        break;
                    } else {
                        cJSON *child = cJSON_GetObjectItem(curr, last_token);
                        if (!child) {
                            child = cJSON_CreateObject();
                            cJSON_AddItemToObject(curr, last_token, child);
                        }
                        curr = child;
                        token = next_token;
                    }
                }
            } else {
                rejected++;
            }
        }
    }

    cJSON_Delete(patch_root);

    char *updated_str = cJSON_PrintUnformatted(screen_root);
    cJSON_Delete(screen_root);

    if (updated_str) {
        screen_store_save_screen(name, updated_str, strlen(updated_str));
        free(updated_str);
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "applied", applied);
    cJSON_AddNumberToObject(resp, "rejected", rejected);
    cJSON_AddStringToObject(resp, "status", rejected == 0 ? "ok" : "partial");
    cJSON_AddNullToObject(resp, "errors");

    char *resp_str = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);

    esp_err_t ret = send_json_str(req, resp_str ? resp_str : "{}");
    if (resp_str) free(resp_str);
    return ret;
}

static esp_err_t image_push_handler(httpd_req_t *req)
{
    char *body = NULL;
    size_t len = 0;
    if (read_body_dynamic(req, &body, &len) != ESP_OK || !body) {
        return send_error(req, 400, "Failed to read body");
    }

    cJSON *root = cJSON_Parse(body);
    if (!root) {
        free(body);
        return send_error(req, 400, "Invalid JSON payload");
    }

    cJSON *name_obj = cJSON_GetObjectItem(root, "name");
    char name[64] = {0};

    if (name_obj && name_obj->valuestring) {
        strncpy(name, name_obj->valuestring, sizeof(name) - 1);
    } else {
        const char *raw_name = extract_last_path_component(req->uri, "/api/eez-device/images/push/");
        clean_name(name, raw_name, sizeof(name));
    }

    cJSON_Delete(root);

    if (name[0] == '\0') {
        free(body);
        return send_error(req, 400, "Image name missing");
    }

    esp_err_t err = screen_store_save_image(name, body);
    free(body);

    if (err != ESP_OK) {
        return send_error(req, 500, "Failed to store image in flash");
    }

    char resp[128];
    snprintf(resp, sizeof(resp), "{\"name\":\"%s\",\"status\":\"ok\"}", name);
    return send_json_str(req, resp);
}

static esp_err_t image_pull_handler(httpd_req_t *req)
{
    char name[64];
    const char *raw_name = extract_last_path_component(req->uri, "/api/eez-device/images/pull/");
    clean_name(name, raw_name, sizeof(name));

    if (name[0] == '\0') {
        return send_error(req, 400, "Image name missing");
    }

    char *buf = NULL;
    size_t size = 0;
    esp_err_t err = screen_store_get_image(name, &buf, &size);
    if (err != ESP_OK || !buf) {
        return send_error(req, 404, "Image not found");
    }

    esp_err_t ret = send_json_str(req, buf);
    free(buf);
    return ret;
}

static esp_err_t image_delete_handler(httpd_req_t *req)
{
    char name[64];
    const char *raw_name = extract_last_path_component(req->uri, "/api/eez-device/images/");
    clean_name(name, raw_name, sizeof(name));

    if (name[0] == '\0') {
        return send_error(req, 400, "Image name missing");
    }

    esp_err_t err = screen_store_delete_image(name);
    if (err != ESP_OK) {
        return send_error(req, 404, "Image not found");
    }

    return send_json_str(req, "{\"status\":\"ok\"}");
}

static esp_err_t reset_defaults_handler(httpd_req_t *req)
{
    esp_err_t err = dynamic_display_reset_to_defaults();
    if (err != ESP_OK) {
        return send_error(req, 500, "Failed to reset display storage");
    }

    return send_json_str(req, "{\"status\":\"ok\",\"screens_restored\":11}");
}

static esp_err_t options_handler(httpd_req_t *req)
{
    set_cors_headers(req);
    httpd_resp_set_status(req, "204 No Content");
    return httpd_resp_send(req, NULL, 0);
}

// --------------------------------------------------------------------------
// Public API Functions
// --------------------------------------------------------------------------

esp_err_t dynamic_display_reset_to_defaults(void)
{
    return screen_store_reset_to_defaults();
}

void dynamic_display_task(void *pvParameters)
{
    if (s_started)
    {
        ESP_LOGW(TAG, "Dynamic Display REST Server already started, skipping");
        vTaskDelete(NULL);
        return;
    }
    s_started = true;
    // Initialize SPIFFS screen storage and seed default screens if needed
    esp_err_t ret = screen_store_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize screen flash storage");
    }
    else
    {
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.max_uri_handlers = 20;
        config.lru_purge_enable = true;
        config.stack_size = 20480; // Increase stack size for JSON processing
        config.uri_match_fn = httpd_uri_match_wildcard;

        ESP_LOGI(TAG, "Starting Dynamic Display REST Server on port %d", config.server_port);
        if (httpd_start(&s_server, &config) != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to start HTTP server");
            s_started = false;
        }
        else
        {
            httpd_uri_t uris[] = {
                {.uri = "/api/eez-device/device/info", .method = HTTP_GET, .handler = info_handler},
                {.uri = "/api/eez-device/screens", .method = HTTP_GET, .handler = screens_get_all_handler},
                {.uri = "/api/eez-device/screens", .method = HTTP_PUT, .handler = screens_put_all_handler},
                {.uri = "/api/eez-device/screens/*", .method = HTTP_GET, .handler = screen_get_one_handler},
                {.uri = "/api/eez-device/screens/*", .method = HTTP_PUT, .handler = single_screen_put_handler},
                {.uri = "/api/eez-device/screens/*", .method = HTTP_PATCH, .handler = screen_patch_handler},
                {.uri = "/api/eez-device/images/push/*", .method = HTTP_POST, .handler = image_push_handler},
                {.uri = "/api/eez-device/images/push", .method = HTTP_POST, .handler = image_push_handler},
                {.uri = "/api/eez-device/images/pull/*", .method = HTTP_GET, .handler = image_pull_handler},
                {.uri = "/api/eez-device/images/*", .method = HTTP_DELETE, .handler = image_delete_handler},
                {.uri = "/api/eez-device/screens/push/*", .method = HTTP_POST, .handler = screens_put_all_handler},
                {.uri = "/api/eez-device/screens/push", .method = HTTP_POST, .handler = screens_put_all_handler},
                {.uri = "/api/eez-device/screens/pull/*", .method = HTTP_POST, .handler = screens_get_all_handler},
                {.uri = "/api/eez-device/screens/pull", .method = HTTP_POST, .handler = screens_get_all_handler},
                {.uri = "/api/eez-device/reset-defaults", .method = HTTP_POST, .handler = reset_defaults_handler},
                {.uri = "/api/eez-device/set-default-screens", .method = HTTP_POST, .handler = reset_defaults_handler},
                {.uri = "/api/eez-device/load-default-screens", .method = HTTP_POST, .handler = reset_defaults_handler},
                {.uri = "/api/eez-device/*", .method = HTTP_OPTIONS, .handler = options_handler},
            };

            for (size_t i = 0; i < sizeof(uris) / sizeof(uris[0]); ++i) {
                httpd_register_uri_handler(s_server, &uris[i]);
            }

            ESP_LOGI(TAG, "Dynamic Display REST Server initialized successfully");
        }
    }
    vTaskDelete(NULL); // Delete this task as it's no longer needed
}

esp_err_t dynamic_display_api_start(void)
{
    xTaskCreate(dynamic_display_task, "dynamic_display_task", 4096, NULL, 5, NULL);
    return ESP_OK;
}

void dynamic_display_api_stop(void)
{
    if (s_server) {
        esp_err_t ret = httpd_stop(s_server);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Dynamic Display REST Server completely stopped");
        } else {
            ESP_LOGE(TAG, "Failed to stop HTTP server: %s",
                     esp_err_to_name(ret));
        }
        s_server = NULL;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    s_started = false;
}
