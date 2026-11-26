/*
 * Deploy-transaction REST endpoints.
 * Contract: docs/t3000/t3-eez-studio/device-interface-deployment-via-bacnet-design.md
 * Guarantees: docs/t3000/t3-eez-studio/screen-deploy-lifecycle-design.md
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include "esp_log.h"
#include "esp_random.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"
#include "screen_flash_store.h"
#include "screen_transaction.h"
#include "screen_deploy_api.h"

#define TAG "ScreenDeployApi"
#define PREFIX "/api/eez-device/deploy/"
#define MAX_BODY_BYTES (512 * 1024)

/* ---------- renderer hooks ---------- */

static screen_ui_status_fn s_status_fn;
static screen_ui_apply_fn s_apply_fn;
static screen_ui_set_mode_fn s_set_mode_fn;
static screen_ui_goto_fn s_goto_fn;
static screen_ui_debug_fn s_debug_fn;

void screen_deploy_set_ui_mode_hook(screen_ui_set_mode_fn set_mode_fn)
{
    s_set_mode_fn = set_mode_fn;
}

void screen_deploy_set_ui_goto_hook(screen_ui_goto_fn goto_fn)
{
    s_goto_fn = goto_fn;
}

void screen_deploy_set_ui_debug_hook(screen_ui_debug_fn debug_fn)
{
    s_debug_fn = debug_fn;
}

void screen_deploy_set_ui_hooks(screen_ui_status_fn status_fn, screen_ui_apply_fn apply_fn)
{
    s_status_fn = status_fn;
    s_apply_fn = apply_fn;
}

void screen_deploy_notify_applied(void)
{
    if (s_apply_fn) s_apply_fn(screen_txn_active_slot(), screen_txn_revision());
}

/* ---------- helpers ---------- */

static char *read_body(httpd_req_t *req, size_t *out_len)
{
    size_t total = req->content_len;
    if (total == 0 || total > MAX_BODY_BYTES) {
        return NULL;
    }
    char *buf = (char *)malloc(total + 1);
    if (!buf) {
        return NULL;
    }
    size_t got = 0;
    int ret = httpd_req_recv(req, buf, total);
    if (ret <= 0) {
        free(buf);
        return NULL;
    }
    got = (size_t)ret;
    size_t guard = 0;
    while (got < total && guard++ < 64) {
        ret = httpd_req_recv(req, buf + got, total - got);
        if (ret <= 0) break;
        got += (size_t)ret;
    }
    buf[got] = '\0';
    if (out_len) *out_len = got;
    return buf;
}

static esp_err_t send_json(httpd_req_t *req, const char *status, const char *body)
{
    httpd_resp_set_status(req, status);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, body);
}

static esp_err_t send_jsonf(httpd_req_t *req, const char *status, const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    return send_json(req, status, buf);
}

/* httpd_query_key_value() returns the raw value; clients such as
 * URLSearchParams send %2F for the area separator. */
static void url_decode(char *s)
{
    char *o = s;
    for (const char *p = s; *p; p++) {
        if (*p == '%' && isxdigit((unsigned char)p[1]) && isxdigit((unsigned char)p[2])) {
            char hex[3] = {p[1], p[2], 0};
            *o++ = (char)strtol(hex, NULL, 16);
            p += 2;
        } else {
            *o++ = *p;
        }
    }
    *o = '\0';
}

/* splits "<id>/<action>" out of the URI tail; returns action or NULL */
static const char *uri_tail(httpd_req_t *req, char *id, size_t id_len)
{
    static char action[32];
    size_t plen = strlen(PREFIX);
    if (strncmp(req->uri, PREFIX, plen) != 0) {
        return NULL;
    }
    const char *tail = req->uri + plen;
    const char *slash = strchr(tail, '/');
    size_t n = slash ? (size_t)(slash - tail) : strlen(tail);
    /* req->uri still carries the query string ("<id>/file?path=..."); an id or
     * action never contains '?' */
    const char *q = memchr(tail, '?', n);
    if (q) {
        n = (size_t)(q - tail);
    }
    if (n == 0 || n >= id_len) {
        return NULL;
    }
    memcpy(id, tail, n);
    id[n] = '\0';
    if (!slash) {
        action[0] = '\0';
        return action;
    }
    const char *a = slash + 1;
    n = strcspn(a, "?");
    if (n >= sizeof(action)) n = sizeof(action) - 1;
    memcpy(action, a, n);
    action[n] = '\0';
    return action;
}

static const char *txn_error_name(screen_txn_error_t e)
{
    switch (e) {
    case SCREEN_TXN_E_ARG:        return "invalid_argument";
    case SCREEN_TXN_E_NO_TXN:     return "no_open_transaction";
    case SCREEN_TXN_E_BAD_JSON:   return "invalid_json";
    case SCREEN_TXN_E_TOO_LARGE:  return "file_too_large";
    case SCREEN_TXN_E_NO_SPACE:   return "no_space";
    case SCREEN_TXN_E_IO:         return "storage_io";
    default:                      return "unknown";
    }
}

/* ---------- GET /storage/capacity ---------- */

static esp_err_t capacity_handler(httpd_req_t *req)
{
    screen_txn_capacity_t cap;
    if (screen_txn_capacity(&cap) != ESP_OK) {
        return send_jsonf(req, "500 Internal Server Error", "{\"error\":\"storage_unavailable\"}");
    }
    return send_jsonf(req, "200 OK",
        "{\"partition\":\"%s\",\"total\":%u,\"used\":%u,\"free\":%u,\"slot_free\":%u,"
        "\"reserved\":%u,\"max_file\":%u,\"active_slot\":\"%c\",\"revision\":%d}",
        SCREEN_TXN_PARTITION, (unsigned)cap.total, (unsigned)cap.used, (unsigned)cap.free,
        (unsigned)cap.slot_free, (unsigned)cap.reserved, (unsigned)cap.max_file,
        cap.active_slot, cap.revision);
}

/* ---------- GET /device/status ---------- */

static esp_err_t device_status_handler(httpd_req_t *req)
{
    /* cheap on purpose: this endpoint is polled (deploy drawer, T3000) and the
     * full capacity walk costs ~0.8 s on this flash */
    size_t total = 0, used = 0, freeb = 0;
    screen_txn_usage(&total, &used, &freeb);

    screen_ui_status_t ui = {0};
    if (s_status_fn) {
        s_status_fn(&ui);
    }
    return send_jsonf(req, "200 OK",
        "{\"ui_mode\":\"%s\",\"mode\":\"%s\",\"mode_failed\":%s,\"tier\":%d,\"slot\":\"%c\",\"revision\":%d,"
        "\"current_screen\":\"%s\",\"build_ok\":%s,\"forced_compiled\":%s,\"last_error\":\"%s\","
        "\"heap\":{\"free\":%u,\"internal_free\":%u,\"internal_min\":%u},"
        "\"storage\":{\"total\":%u,\"used\":%u,\"free\":%u}}",
        ui.ui_json ? "json" : "compiled", ui.mode, ui.mode_failed ? "true" : "false",
        ui.ui_json ? 1 : 0,
        screen_txn_active_slot()[0], screen_txn_revision(), ui.screen,
        ui.build_ok ? "true" : "false", ui.forced_compiled ? "true" : "false", ui.last_error,
        (unsigned)esp_get_free_heap_size(),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
        (unsigned)total, (unsigned)used, (unsigned)freeb);
}

/* ---------- GET|POST /ui-mode ---------- */

static esp_err_t ui_mode_handler(httpd_req_t *req)
{    if (req->method == HTTP_POST || req->method == HTTP_PUT) {
        size_t len = 0;
        char *body = read_body(req, &len);
        if (!body) {
            return send_jsonf(req, "400 Bad Request", "{\"error\":\"missing_body\"}");
        }
        cJSON *root = cJSON_ParseWithLength(body, len);
        free(body);
        if (!root) {
            return send_jsonf(req, "400 Bad Request", "{\"error\":\"invalid_json\"}");
        }
        const cJSON *mode = cJSON_GetObjectItem(root, "mode");
        int want;
        char label[12] = "json";
        if (cJSON_IsString(mode)) {
            snprintf(label, sizeof(label), "%s", mode->valuestring);
            if (!strcmp(label, "json")) want = 0;
            else if (!strcmp(label, "compiled")) want = 1;
            else if (!strcmp(label, "auto")) want = 2;
            else want = -1;
        } else if (cJSON_IsBool(mode)) {
            want = cJSON_IsTrue(mode) ? 0 : 1;   /* {"mode":true} = json */
            snprintf(label, sizeof(label), "%s", want ? "compiled" : "json");
        } else {
            want = -1;
        }
        cJSON_Delete(root);
        if (want < 0) {
            return send_jsonf(req, "400 Bad Request",
                              "{\"error\":\"bad_mode\",\"expect\":[\"auto\",\"json\",\"compiled\"]}");
        }
        if (!s_set_mode_fn) {
            return send_jsonf(req, "501 Not Implemented", "{\"error\":\"renderer_absent\"}");
        }
        int state = s_set_mode_fn(want);
        return send_jsonf(req, "200 OK", "{\"ok\":true,\"ui_mode\":\"%s\",\"mode\":\"%s\"}",
                          state ? "compiled" : "json", label);
    }

    screen_ui_status_t ui = {0};
    if (s_status_fn) s_status_fn(&ui);
    return send_jsonf(req, "200 OK",
                      "{\"ui_mode\":\"%s\",\"mode\":\"%s\",\"mode_failed\":%s,\"forced_compiled\":%s}",
                      ui.ui_json ? "json" : "compiled", ui.mode,
                      ui.mode_failed ? "true" : "false",
                      ui.forced_compiled ? "true" : "false");
}

/* ---------- POST /ui-screen {"name":"parameters"} ---------- */

static esp_err_t ui_screen_handler(httpd_req_t *req)
{
    if (!s_goto_fn) {
        return send_jsonf(req, "501 Not Implemented", "{\"error\":\"renderer_absent\"}");
    }

    size_t len = 0;
    char *body = read_body(req, &len);
    if (!body) {
        return send_jsonf(req, "400 Bad Request", "{\"error\":\"missing_body\"}");
    }
    cJSON *root = cJSON_ParseWithLength(body, len);
    free(body);
    if (!root) {
        return send_jsonf(req, "400 Bad Request", "{\"error\":\"invalid_json\"}");
    }

    const cJSON *name = cJSON_GetObjectItem(root, "name");
    const char *target = cJSON_IsString(name) ? name->valuestring : NULL;
    char want[40] = {0};
    if (target) snprintf(want, sizeof(want), "%s", target);
    cJSON_Delete(root);

    if (!want[0] || !s_goto_fn(want)) {
        return send_jsonf(req, "400 Bad Request", "{\"error\":\"bad_screen\"}");
    }
    return send_jsonf(req, "200 OK", "{\"ok\":true,\"navigating\":\"%s\"}", want);
}

/* ---------- GET|POST /ui-debug {"layout":true} ---------- */

static esp_err_t ui_debug_handler(httpd_req_t *req)
{
    if (!s_debug_fn) {
        return send_jsonf(req, "501 Not Implemented", "{\"error\":\"renderer_absent\"}");
    }

    int enable = -1;
    if (req->method == HTTP_POST || req->method == HTTP_PUT) {
        size_t len = 0;
        char *body = read_body(req, &len);
        if (!body) {
            return send_jsonf(req, "400 Bad Request", "{\"error\":\"missing_body\"}");
        }
        cJSON *root = cJSON_ParseWithLength(body, len);
        free(body);
        if (!root) {
            return send_jsonf(req, "400 Bad Request", "{\"error\":\"invalid_json\"}");
        }
        const cJSON *layout = cJSON_GetObjectItem(root, "layout");
        if (cJSON_IsBool(layout)) {
            enable = cJSON_IsTrue(layout) ? 1 : 0;
        } else if (cJSON_IsNumber(layout)) {
            enable = layout->valueint ? 1 : 0;
        } else if (layout) {
            cJSON_Delete(root);
            return send_jsonf(req, "400 Bad Request", "{\"error\":\"bad_layout\"}");
        }
        cJSON_Delete(root);
    }

    int state = s_debug_fn(enable);
    return send_jsonf(req, "200 OK", "{\"ok\":true,\"layout_dump\":%s}",
                      state ? "true" : "false");
}

/* ---------- POST /deploy/begin ---------- */

static esp_err_t begin_handler(httpd_req_t *req)
{
    size_t len = 0;
    char *body = read_body(req, &len);
    if (!body) {
        return send_jsonf(req, "400 Bad Request", "{\"error\":\"missing_body\"}");
    }
    cJSON *root = cJSON_ParseWithLength(body, len);
    free(body);
    if (!root) {
        return send_jsonf(req, "400 Bad Request", "{\"error\":\"invalid_json\"}");
    }

    const cJSON *schema = cJSON_GetObjectItem(root, "schema");
    const cJSON *rev = cJSON_GetObjectItem(root, "revision");
    const cJSON *start = cJSON_GetObjectItem(root, "start_screen");
    int revision = cJSON_IsNumber(rev) ? rev->valueint : 0;
    const char *start_screen = cJSON_IsString(start) ? start->valuestring : "";

    /* a client that omits the revision (0) gets the next one; an explicitly
       older revision is still refused by screen_txn_begin() with 422 */
    if (revision <= 0) {
        revision = screen_txn_revision() + 1;
    }

    if (cJSON_IsNumber(schema) && schema->valueint != 1) {
        cJSON_Delete(root);
        return send_jsonf(req, "422 Unprocessable Entity",
                          "{\"error\":\"schema_unsupported\",\"supported\":[1]}");
    }

    char deploy_id[17];
    uint32_t r = esp_random();
    snprintf(deploy_id, sizeof(deploy_id), "%08x%08x", (unsigned)r, (unsigned)(r ^ 0x9e3779b9u));

    esp_err_t err = screen_txn_begin(deploy_id, revision, start_screen);
    cJSON_Delete(root);

    if (err == ESP_ERR_INVALID_VERSION) {
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "{\"error\":\"revision_not_newer\",\"device_revision\":%d}",
                 screen_txn_revision());
        return send_json(req, "422 Unprocessable Entity", buf);
    }
    if (err == ESP_ERR_INVALID_STATE) {
        return send_jsonf(req, "409 Conflict", "{\"error\":\"deploy_in_progress\"}");
    }
    if (err == ESP_ERR_NO_MEM) {
        return send_jsonf(req, "507 Insufficient Storage", "{\"error\":\"no_space\"}");
    }
    if (err != ESP_OK) {
        return send_jsonf(req, "500 Internal Server Error", "{\"error\":\"begin_failed\"}");
    }

    return send_jsonf(req, "200 OK",
        "{\"deploy_id\":\"%s\",\"slot\":\"%c\",\"revision\":%d,\"resume_from\":[]}",
        deploy_id, screen_txn_active_slot()[0] == 'A' ? 'B' : 'A',
        revision > 0 ? revision : screen_txn_revision() + 1);
}

/* ---------- PUT /deploy/<id>/file?path=<area>/<name> ---------- */

static esp_err_t file_handler(httpd_req_t *req)
{
    char deploy_id[24];
    const char *action = uri_tail(req, deploy_id, sizeof(deploy_id));
    if (!action || strcmp(action, "file") != 0) {
        return send_jsonf(req, "404 Not Found", "{\"error\":\"unknown_deploy_action\"}");
    }

    char query[128] = {0};
    char path[96] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        if (httpd_query_key_value(query, "path", path, sizeof(path)) != ESP_OK) {
            path[0] = '\0';
        }
    }
    if (!path[0]) {
        return send_jsonf(req, "400 Bad Request", "{\"error\":\"missing_path\"}");
    }
    url_decode(path);

    char area[16] = {0};
    const char *name = "";
    const char *slash = strchr(path, '/');
    if (slash) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof(area)) n = sizeof(area) - 1;
        memcpy(area, path, n);
        name = slash + 1;
    }
    if (strcmp(area, "screens") && strcmp(area, "images")) {
        return send_jsonf(req, "400 Bad Request", "{\"error\":\"bad_area\"}");
    }
    size_t name_len = strlen(name);
    if (name_len == 0 || name_len > 60) {
        return send_jsonf(req, "400 Bad Request", "{\"error\":\"bad_name\"}");
    }
    for (size_t i = 0; i < name_len; i++) {
        char c = name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
              c == '_' || c == '-')) {
            return send_jsonf(req, "400 Bad Request", "{\"error\":\"bad_name\"}");
        }
    }

    size_t len = 0;
    char *body = read_body(req, &len);
    if (!body) {
        return send_jsonf(req, "400 Bad Request", "{\"error\":\"missing_body\"}");
    }

    char sha[65] = {0};
    screen_txn_error_t ecode = SCREEN_TXN_OK;
    esp_err_t err = screen_txn_stage(deploy_id, area, name, body, len, sha, sizeof(sha), &ecode);
    free(body);

    if (err != ESP_OK) {
        const char *name_err = txn_error_name(ecode);
        const char *status = "500 Internal Server Error";
        if (ecode == SCREEN_TXN_E_BAD_JSON || ecode == SCREEN_TXN_E_ARG) status = "422 Unprocessable Entity";
        if (ecode == SCREEN_TXN_E_TOO_LARGE) status = "413 Payload Too Large";
        if (ecode == SCREEN_TXN_E_NO_SPACE) status = "507 Insufficient Storage";
        if (ecode == SCREEN_TXN_E_NO_TXN) status = "409 Conflict";
        return send_jsonf(req, status,
                          "{\"error\":\"%s\",\"path\":\"%s/%s\"}", name_err, area, name);
    }

    return send_jsonf(req, "200 OK",
        "{\"path\":\"%s/%s\",\"bytes\":%u,\"sha256\":\"%s\",\"staged\":true}",
        area, name, (unsigned)len, sha);
}

/* ---------- POST /deploy/<id>/commit | /abort ---------- */

static esp_err_t commit_abort_handler(httpd_req_t *req)
{
    char deploy_id[24];
    const char *action = uri_tail(req, deploy_id, sizeof(deploy_id));
    if (!action) {
        return send_jsonf(req, "404 Not Found", "{\"error\":\"unknown_deploy_action\"}");
    }

    if (strcmp(action, "abort") == 0) {
        screen_txn_abort(deploy_id);
        return send_jsonf(req, "200 OK", "{\"ok\":true,\"revision\":%d}", screen_txn_revision());
    }
    if (strcmp(action, "commit") != 0) {
        return send_jsonf(req, "404 Not Found", "{\"error\":\"unknown_deploy_action\"}");
    }

    int revision = 0;
    char problems[512] = {0};
    esp_err_t err = screen_txn_commit(deploy_id, &revision, problems, sizeof(problems));

    if (err == ESP_ERR_INVALID_STATE) {
        char buf[640];
        snprintf(buf, sizeof(buf),
                 "{\"ok\":false,\"error\":\"validation_failed\",\"problems\":[%s]}", problems);
        return send_json(req, "422 Unprocessable Entity", buf);
    }
    if (err != ESP_OK) {
        return send_jsonf(req, "500 Internal Server Error",
                          "{\"ok\":false,\"error\":\"commit_failed\"}");
    }

    int counts_screens = 0;
    int counts_images = 0;
    screen_txn_last_counts(&counts_screens, &counts_images);
    bool applied = (s_apply_fn != NULL) && s_apply_fn(screen_txn_active_slot(), revision);

    return send_jsonf(req, "200 OK",
        "{\"ok\":true,\"revision\":%d,\"slot\":\"%c\",\"active_slot\":\"%c\","
        "\"applied\":%s,\"screens\":%d,\"images\":%d}",
        revision, screen_txn_active_slot()[0], screen_txn_active_slot()[0],
        applied ? "true" : "false", counts_screens, counts_images);
}

/* ---------- GET /deploy/<id> ---------- */

static esp_err_t deploy_status_handler(httpd_req_t *req)
{
    char deploy_id[24];
    const char *action = uri_tail(req, deploy_id, sizeof(deploy_id));
    if (!action) {
        return send_jsonf(req, "404 Not Found", "{\"error\":\"unknown_deploy_id\"}");
    }
    return send_jsonf(req, "200 OK",
        "{\"deploy_id\":\"%s\",\"active_slot\":\"%c\",\"revision\":%d}",
        deploy_id, screen_txn_active_slot()[0], screen_txn_revision());
}

/* ---------- POST /reboot ---------- */

static esp_err_t reboot_handler(httpd_req_t *req)
{
    send_jsonf(req, "200 OK", "{\"rebooting\":true}");
    vTaskDelay(pdMS_TO_TICKS(300));
    esp_restart();
    return ESP_OK;
}

/* ---------- registration ---------- */

esp_err_t screen_deploy_register_handlers(httpd_handle_t server)
{
    static const httpd_uri_t uris[] = {
        {.uri = "/api/eez-device/storage/capacity", .method = HTTP_GET,  .handler = capacity_handler},
        {.uri = "/api/eez-device/device/status",    .method = HTTP_GET,  .handler = device_status_handler},
    {.uri = "/api/eez-device/ui-mode",          .method = HTTP_GET,  .handler = ui_mode_handler},
    {.uri = "/api/eez-device/ui-mode",          .method = HTTP_POST, .handler = ui_mode_handler},
    {.uri = "/api/eez-device/ui-screen",        .method = HTTP_POST, .handler = ui_screen_handler},
{.uri = "/api/eez-device/ui-debug",         .method = HTTP_GET,  .handler = ui_debug_handler},
{.uri = "/api/eez-device/ui-debug",         .method = HTTP_POST, .handler = ui_debug_handler},
        {.uri = "/api/eez-device/reboot",           .method = HTTP_POST, .handler = reboot_handler},
        {.uri = "/api/eez-device/deploy/begin",     .method = HTTP_POST, .handler = begin_handler},
        {.uri = "/api/eez-device/deploy/*",         .method = HTTP_PUT,  .handler = file_handler},
        {.uri = "/api/eez-device/deploy/*",         .method = HTTP_POST, .handler = commit_abort_handler},
        {.uri = "/api/eez-device/deploy/*",         .method = HTTP_GET,  .handler = deploy_status_handler},
    };
    for (size_t i = 0; i < sizeof(uris) / sizeof(uris[0]); ++i) {
        esp_err_t err = httpd_register_uri_handler(server, &uris[i]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "failed to register %s (%s)", uris[i].uri, esp_err_to_name(err));
            return err;
        }
    }
    ESP_LOGI(TAG, "deploy transaction endpoints registered");
    return ESP_OK;
}
