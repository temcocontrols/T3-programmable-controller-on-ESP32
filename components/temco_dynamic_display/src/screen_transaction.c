/*
 * Screen deploy transactions (design: docs/t3000/t3-eez-studio/screen-deploy-lifecycle-design.md)
 *
 * Two slots, one commit record. The inactive slot is filled first; commit flips
 * the record, so a failure at any point leaves the active slot untouched.
 *
 *   /spiffs/deploy.json            open transaction marker (deploy_id, revision, start_screen)
 *   /spiffs/commit.json            active slot + revision  (the atomic switch)
 *   /spiffs/<A|B>/index.json       [{area,name,bytes,sha256},...]
 *   /spiffs/<A|B>/screens/<n>.json
 *   /spiffs/<A|B>/images/<n>.json
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_rom_crc.h"
#include "mbedtls/sha256.h"
#include "cJSON.h"
#include "screen_transaction.h"

#define TAG "ScreenTxn"

#define MOUNT_PATH   "/spiffs"
#define COMMIT_FILE  MOUNT_PATH "/commit.json"
#define MARKER_FILE  MOUNT_PATH "/deploy.json"
#define MAX_FILE_BYTES (256 * 1024)
#define RESERVED_BYTES (64 * 1024)

static char s_active = 'A';
static int  s_revision = 0;
static char s_start_screen[40];
static int  s_last_screens = 0;
static int  s_last_images = 0;
static bool s_ready = false;

/* ---------- helpers ---------- */

static void slot_dir(char slot, const char *area, char *buf, size_t n)
{
    if (area && area[0]) {
        snprintf(buf, n, "%s/%c/%s", MOUNT_PATH, slot, area);
    } else {
        snprintf(buf, n, "%s/%c", MOUNT_PATH, slot);
    }
}

static void slot_file(char slot, const char *area, const char *name, char *buf, size_t n)
{
    snprintf(buf, n, "%s/%c/%s/%s.json", MOUNT_PATH, slot, area, name);
}

static void slot_index(char slot, char *buf, size_t n)
{
    snprintf(buf, n, "%s/%c/index.json", MOUNT_PATH, slot);
}

static char other_slot(char slot)
{
    return (slot == 'A') ? 'B' : 'A';
}

static esp_err_t ensure_dir(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
        return (S_ISDIR(st.st_mode)) ? ESP_OK : ESP_FAIL;
    }
    /* SPIFFS has no real directories: a path exists only as the prefix of a
     * stored file and mkdir() reports ENOTSUP. Writing below the prefix is
     * what creates it, so ENOTSUP is success here. */
    if (mkdir(path, 0755) == 0 || errno == EEXIST || errno == ENOTSUP) {
        return ESP_OK;
    }
    return ESP_FAIL;
}

static esp_err_t ensure_slot_dirs(char slot)
{
    char path[64];
    slot_dir(slot, NULL, path, sizeof(path));
    if (ensure_dir(path) != ESP_OK) return ESP_FAIL;
    slot_dir(slot, "screens", path, sizeof(path));
    if (ensure_dir(path) != ESP_OK) return ESP_FAIL;
    slot_dir(slot, "images", path, sizeof(path));
    return ensure_dir(path);
}

static esp_err_t wipe_slot(char slot)
{
    char dir[64];
    slot_dir(slot, NULL, dir, sizeof(dir));
    DIR *d = opendir(dir);
    if (!d) {
        return ensure_slot_dirs(slot);
    }
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        if (!strcmp(e->d_name, "index.json")) continue;
        if (!strcmp(e->d_name, "screens") || !strcmp(e->d_name, "images")) {
            char area_dir[320];
            snprintf(area_dir, sizeof(area_dir), "%s/%.120s", dir, e->d_name);
            DIR *ad = opendir(area_dir);
            if (ad) {
                struct dirent *ae;
                while ((ae = readdir(ad)) != NULL) {
                    if (!strcmp(ae->d_name, ".") || !strcmp(ae->d_name, "..")) continue;
                    char f[480];
                    snprintf(f, sizeof(f), "%s/%.150s", area_dir, ae->d_name);
                    unlink(f);
                }
                closedir(ad);
            }
            continue;
        }
        /* SPIFFS reports nested names flat, e.g. "screens/home_screen.json" */
        char f[480];
        snprintf(f, sizeof(f), "%s/%.150s", dir, e->d_name);
        unlink(f);
    }
    closedir(d);
    char idx[96];
    slot_index(slot, idx, sizeof(idx));
    unlink(idx);
    return ensure_slot_dirs(slot);
}

/* ---------- directory accounting ---------- */

/* total bytes of every file below `dir` (slot layout is only two levels deep) */
static size_t dir_bytes(const char *dir)
{
    size_t sum = 0;
    DIR *d = opendir(dir);
    if (!d) return 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        char p[320];
        snprintf(p, sizeof(p), "%s/%.150s", dir, e->d_name);
        struct stat st;
        if (stat(p, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            sum += dir_bytes(p);
        } else {
            sum += (size_t)st.st_size;
        }
    }
    closedir(d);
    return sum;
}

static size_t slot_bytes(char slot)
{
    char dir[64];
    slot_dir(slot, NULL, dir, sizeof(dir));
    return dir_bytes(dir);
}

static void sha256_hex(const char *data, size_t len, char *out, size_t out_len)
{
    unsigned char digest[32];
    mbedtls_sha256((const unsigned char *)data, len, digest, 0);
    if (out_len < 65) {
        if (out_len) out[0] = '\0';
        return;
    }
    for (int i = 0; i < 32; i++) {
        snprintf(out + i * 2, 3, "%02x", digest[i]);
    }
    out[64] = '\0';
}

/* reads a whole file into a heap buffer (caller frees) */
static char *read_whole_file(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) {
        fclose(f);
        return NULL;
    }
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[got] = '\0';
    if (out_len) *out_len = got;
    return buf;
}

static esp_err_t write_file_atomic(const char *path, const char *data, size_t len)
{
    char tmp[320];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    FILE *f = fopen(tmp, "wb");
    if (!f) {
        return ESP_FAIL;
    }
    size_t written = 0;
    while (written < len) {
        size_t chunk = len - written;
        if (chunk > 4096) chunk = 4096;
        size_t n = fwrite(data + written, 1, chunk, f);
        if (n == 0) break;
        written += n;
    }
    int flush_err = fflush(f);
    int sync_err = 0;
    int fd = fileno(f);
    if (fd >= 0) sync_err = fsync(fd);
    int stream_err = ferror(f);
    fclose(f);

    if (written != len || flush_err != 0 || sync_err != 0 || stream_err) {
        unlink(tmp);
        return ESP_FAIL;
    }
    unlink(path);
    if (rename(tmp, path) != 0) {
        unlink(tmp);
        return ESP_FAIL;
    }
    return ESP_OK;
}

/* ---------- commit record ---------- */

static uint32_t commit_crc(char slot, int revision)
{
    char body[48];
    int n = snprintf(body, sizeof(body), "1|%c|%d", slot, revision);
    return esp_rom_crc32_le(0, (const uint8_t *)body, (uint32_t)n);
}

static esp_err_t write_commit(char slot, int revision, const char *start_screen)
{
    char json[192];
    snprintf(json, sizeof(json),
             "{\"record\":1,\"slot\":\"%c\",\"revision\":%d,\"start\":\"%.39s\",\"crc\":%u}",
             slot, revision, start_screen ? start_screen : "",
             (unsigned)commit_crc(slot, revision));
    esp_err_t err = write_file_atomic(COMMIT_FILE, json, strlen(json));
    if (err == ESP_OK) {
        s_active = slot;
        s_revision = revision;
        snprintf(s_start_screen, sizeof(s_start_screen), "%s", start_screen ? start_screen : "");
        ESP_LOGI(TAG, "commit record -> slot %c revision %d", slot, revision);
    }
    return err;
}

static esp_err_t read_commit(char *slot_out, int *revision_out)
{
    size_t len = 0;
    char *text = read_whole_file(COMMIT_FILE, &len);
    if (!text) return ESP_ERR_NOT_FOUND;

    esp_err_t err = ESP_FAIL;
    cJSON *root = cJSON_Parse(text);
    if (root) {
        const cJSON *rec = cJSON_GetObjectItem(root, "record");
        const cJSON *slot = cJSON_GetObjectItem(root, "slot");
        const cJSON *rev = cJSON_GetObjectItem(root, "revision");
        const cJSON *crc = cJSON_GetObjectItem(root, "crc");
        const cJSON *start = cJSON_GetObjectItem(root, "start");
        if (cJSON_IsNumber(rec) && rec->valueint == 1 && cJSON_IsString(slot) &&
            slot->valuestring && slot->valuestring[0] && cJSON_IsNumber(rev) && cJSON_IsNumber(crc)) {
            char s = slot->valuestring[0];
            int r = rev->valueint;
            if ((s == 'A' || s == 'B') && (unsigned)crc->valuedouble == (unsigned)commit_crc(s, r)) {
                if (slot_out) *slot_out = s;
                if (revision_out) *revision_out = r;
                snprintf(s_start_screen, sizeof(s_start_screen), "%s",
                         (cJSON_IsString(start) && start->valuestring) ? start->valuestring : "");
                err = ESP_OK;
            }
        }
        cJSON_Delete(root);
    }
    free(text);
    return err;
}

/* ---------- deploy marker ---------- */

typedef struct {
    char id[32];
    char slot;
    int  revision;
    char start_screen[40];
} marker_info_t;

/* Reads and validates the open-deploy marker. Fails when no transaction is
 * open, so stage/commit/abort can never act on a stale or foreign deploy. */
static esp_err_t read_marker(marker_info_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));

    char *text = read_whole_file(MARKER_FILE, NULL);
    if (!text) return ESP_ERR_NOT_FOUND;

    esp_err_t err = ESP_ERR_NOT_FOUND;
    cJSON *root = cJSON_Parse(text);
    if (root) {
        const cJSON *id = cJSON_GetObjectItem(root, "id");
        const cJSON *slot = cJSON_GetObjectItem(root, "slot");
        const cJSON *rev = cJSON_GetObjectItem(root, "revision");
        const cJSON *start = cJSON_GetObjectItem(root, "start_screen");
        if (cJSON_IsString(id) && id->valuestring && id->valuestring[0] &&
            cJSON_IsString(slot) && slot->valuestring &&
            (slot->valuestring[0] == 'A' || slot->valuestring[0] == 'B')) {
            snprintf(out->id, sizeof(out->id), "%s", id->valuestring);
            out->slot = slot->valuestring[0];
            out->revision = cJSON_IsNumber(rev) ? rev->valueint : 0;
            if (cJSON_IsString(start) && start->valuestring) {
                snprintf(out->start_screen, sizeof(out->start_screen), "%s",
                         start->valuestring);
            }
            err = ESP_OK;
        }
        cJSON_Delete(root);
    }
    free(text);
    return err;
}

/* ---------- index ---------- */

static esp_err_t write_index(char slot, cJSON *entries)
{
    char path[96];
    slot_index(slot, path, sizeof(path));
    char *text = cJSON_PrintUnformatted(entries);
    if (!text) return ESP_FAIL;
    esp_err_t err = write_file_atomic(path, text, strlen(text));
    free(text);
    return err;
}

static esp_err_t index_append(char slot, const char *area, const char *name, size_t bytes, const char *sha)
{
    char path[96];
    slot_index(slot, path, sizeof(path));

    cJSON *entries = NULL;
    size_t len = 0;
    char *text = read_whole_file(path, &len);
    if (text) {
        entries = cJSON_Parse(text);
        free(text);
    }
    if (!cJSON_IsArray(entries)) {
        if (entries) cJSON_Delete(entries);
        entries = cJSON_CreateArray();
    }
    if (!entries) return ESP_ERR_NO_MEM;

    /* a retried stage of the same file replaces its entry instead of duplicating it */
    cJSON *existing = NULL;
    cJSON *it = NULL;
    cJSON_ArrayForEach(it, entries) {
        const cJSON *a = cJSON_GetObjectItem(it, "area");
        const cJSON *n = cJSON_GetObjectItem(it, "name");
        if (cJSON_IsString(a) && cJSON_IsString(n) &&
            !strcmp(a->valuestring, area) && !strcmp(n->valuestring, name)) {
            existing = it;
            break;
        }
    }
    if (existing) {
        cJSON_ReplaceItemInObject(existing, "bytes", cJSON_CreateNumber((double)bytes));
        cJSON_ReplaceItemInObject(existing, "sha256", cJSON_CreateString(sha));
    } else {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "area", area);
        cJSON_AddStringToObject(item, "name", name);
        cJSON_AddNumberToObject(item, "bytes", (double)bytes);
        cJSON_AddStringToObject(item, "sha256", sha);
        cJSON_AddItemToArray(entries, item);
    }

    esp_err_t err = write_index(slot, entries);
    cJSON_Delete(entries);
    return err;
}

static bool index_has(cJSON *entries, const char *area, const char *name)
{
    if (!cJSON_IsArray(entries)) return false;
    const cJSON *it = NULL;
    cJSON_ArrayForEach(it, entries) {
        const cJSON *a = cJSON_GetObjectItem(it, "area");
        const cJSON *n = cJSON_GetObjectItem(it, "name");
        if (cJSON_IsString(a) && cJSON_IsString(n) &&
            !strcmp(a->valuestring, area) && !strcmp(n->valuestring, name)) {
            return true;
        }
    }
    return false;
}

/* a slot with a non-empty index still holds a deployable set */
static bool slot_has_index(char slot)
{
    char path[96];
    slot_index(slot, path, sizeof(path));
    char *text = read_whole_file(path, NULL);
    if (!text) return false;
    cJSON *root = cJSON_Parse(text);
    bool ok = cJSON_IsArray(root) && cJSON_GetArraySize(root) > 0;
    if (root) cJSON_Delete(root);
    free(text);
    return ok;
}

/* appends one {"path","reason"} entry, skipping it if it would not fit whole so
 * the array handed back to the client always parses */
static void append_problem(char *problems, size_t problems_len, const char *area,
                           const char *name, const char *reason)
{
    if (!problems || problems_len == 0) return;

    char item[192];
    int n = snprintf(item, sizeof(item), "%s{\"path\":\"%s/%s\",\"reason\":\"%s\"}",
                     problems[0] ? "," : "", area, name, reason);
    if (n < 0 || (size_t)n >= sizeof(item)) return;

    size_t cur = strlen(problems);
    if (cur + (size_t)n >= problems_len) return;
    memcpy(problems + cur, item, (size_t)n + 1);
}

/* ---------- public API ---------- */

const char *screen_txn_active_slot(void)
{
    static char s[2] = {'A', 0};
    s[0] = s_active;
    return s;
}

int screen_txn_revision(void)
{
    return s_revision;
}

bool screen_txn_is_ready(void)
{
    return s_ready;
}

const char *screen_txn_start_screen(void)
{
    return s_start_screen;
}

void screen_txn_last_counts(int *screens, int *images)
{
    if (screens) *screens = s_last_screens;
    if (images) *images = s_last_images;
}

esp_err_t screen_txn_read_active(const char *area, const char *name,
                                 char **out_buf, size_t *out_len)
{
    if (!out_buf) return ESP_ERR_INVALID_ARG;
    *out_buf = NULL;
    if (!s_ready || !area || !name) return ESP_ERR_INVALID_STATE;

    char path[128];
    slot_file(s_active, area, name, path, sizeof(path));
    size_t len = 0;
    char *buf = read_whole_file(path, &len);
    if (!buf) return ESP_ERR_NOT_FOUND;
    *out_buf = buf;
    if (out_len) *out_len = len;
    return ESP_OK;
}

void screen_slot_path(const char *area, const char *name, char *buf, size_t n)
{
    slot_file(s_active, area, name, buf, n);
}

esp_err_t screen_txn_list_active(const char *area, char names[][32],
                                 size_t max_count, size_t *out_count)
{
    if (!names || !out_count) return ESP_ERR_INVALID_ARG;
    *out_count = 0;
    if (!s_ready || !area) return ESP_ERR_INVALID_STATE;

    char dir[96];
    slot_dir(s_active, area, dir, sizeof(dir));
    DIR *d = opendir(dir);
    if (!d) return ESP_ERR_NOT_FOUND;

    size_t count = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL && count < max_count) {
        const char *dot = strrchr(e->d_name, '.');
        if (!dot || strcmp(dot, ".json") != 0) continue;
        size_t len = (size_t)(dot - e->d_name);
        if (len == 0 || len >= 32) continue;
        memcpy(names[count], e->d_name, len);
        names[count][len] = '\0';
        count++;
    }
    closedir(d);

    *out_count = count;
    return ESP_OK;
}

esp_err_t screen_txn_init(void)
{
    if (s_ready) return ESP_OK;

    char slot = 0;
    int revision = 0;
    if (read_commit(&slot, &revision) == ESP_OK) {
        s_active = slot;
        s_revision = revision;
        ESP_LOGI(TAG, "active slot %c revision %d", s_active, s_revision);
    } else {
        /* Commit record missing or unreadable: first boot of this firmware, or
         * a torn write. Never wipe a slot that still holds a complete set —
         * adopt it so the panel keeps booting, and rewrite the record. */
        char adopt = 0;
        if (slot_has_index('A')) {
            adopt = 'A';
        } else if (slot_has_index('B')) {
            adopt = 'B';
        }

        if (adopt) {
            s_active = adopt;
            s_revision = 0;
            ESP_LOGW(TAG, "commit record missing — adopting slot %c at revision 0", adopt);
            if (write_commit(adopt, 0, NULL) != ESP_OK) {
                ESP_LOGE(TAG, "could not rewrite the commit record");
            }
        } else {
            wipe_slot('A');
            wipe_slot('B');
            esp_err_t err = write_commit('A', 0, NULL);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "could not write the initial commit record");
                return err;
            }
        }
    }

    ensure_slot_dirs(s_active);
    ensure_slot_dirs(other_slot(s_active));
    s_ready = true;
    return ESP_OK;
}

void screen_txn_usage(size_t *total, size_t *used, size_t *free_bytes)
{
    size_t t = 0, u = 0;
    if (esp_spiffs_info(SCREEN_TXN_PARTITION, &t, &u) != ESP_OK) {
        t = u = 0;
    }
    if (total) *total = t;
    if (used) *used = u;
    if (free_bytes) *free_bytes = (t > u) ? (t - u) : 0;
}

esp_err_t screen_txn_capacity(screen_txn_capacity_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));

    size_t total = 0, used = 0;
    esp_err_t err = esp_spiffs_info(SCREEN_TXN_PARTITION, &total, &used);
    if (err != ESP_OK) return err;

    out->total = total;
    out->used = used;
    out->free = (total > used) ? (total - used) : 0;
    out->reserved = RESERVED_BYTES;
    out->max_file = MAX_FILE_BYTES;
    out->active_slot = s_active;
    out->revision = s_revision;

    /* `begin` wipes the inactive slot before staging, so whatever it holds now
     * is reclaimable by the next deploy. */
    out->slot_free = out->free + slot_bytes(other_slot(s_active));
    return ESP_OK;
}

esp_err_t screen_txn_begin(const char *deploy_id, int revision, const char *start_screen)
{
    if (!s_ready || !deploy_id || !deploy_id[0]) return ESP_ERR_INVALID_STATE;

    if (revision != 0 && revision <= s_revision) {
        return ESP_ERR_INVALID_VERSION;
    }

    /* one transaction at a time */
    struct stat st;
    if (stat(MARKER_FILE, &st) == 0 && st.st_size > 0) {
        return ESP_ERR_INVALID_STATE;
    }

    char slot = other_slot(s_active);
    esp_err_t err = wipe_slot(slot);
    if (err != ESP_OK) return err;

    char json[256];
    snprintf(json, sizeof(json),
             "{\"id\":\"%s\",\"revision\":%d,\"start_screen\":\"%.39s\",\"slot\":\"%c\"}",
             deploy_id, revision, start_screen ? start_screen : "", slot);
    return write_file_atomic(MARKER_FILE, json, strlen(json));
}

esp_err_t screen_txn_stage(const char *deploy_id, const char *area, const char *name,
                           const char *data, size_t len, char *sha_out, size_t sha_len,
                           screen_txn_error_t *err_code)
{
    if (err_code) *err_code = SCREEN_TXN_OK;
    if (!s_ready || !deploy_id || !area || !name || !data) {
        if (err_code) *err_code = SCREEN_TXN_E_ARG;
        return ESP_ERR_INVALID_ARG;
    }

    marker_info_t marker;
    if (read_marker(&marker) != ESP_OK || strcmp(marker.id, deploy_id) != 0) {
        if (err_code) *err_code = SCREEN_TXN_E_NO_TXN;
        return ESP_ERR_INVALID_STATE;
    }

    if (len > MAX_FILE_BYTES) {
        if (err_code) *err_code = SCREEN_TXN_E_TOO_LARGE;
        return ESP_ERR_INVALID_SIZE;
    }

    /* V2: the body must be a JSON object */
    cJSON *parsed = cJSON_ParseWithLength(data, len);
    if (!parsed) {
        if (err_code) *err_code = SCREEN_TXN_E_BAD_JSON;
        return ESP_ERR_INVALID_ARG;
    }
    cJSON_Delete(parsed);

    size_t total = 0, used = 0;
    if (esp_spiffs_info(SCREEN_TXN_PARTITION, &total, &used) == ESP_OK) {
        size_t free_bytes = (total > used) ? (total - used) : 0;
        if (free_bytes < len + 2048 || free_bytes - len < RESERVED_BYTES) {
            if (err_code) *err_code = SCREEN_TXN_E_NO_SPACE;
            return ESP_ERR_NO_MEM;
        }
    }

    char path[128];
    slot_file(marker.slot, area, name, path, sizeof(path));
    esp_err_t err = write_file_atomic(path, data, len);
    if (err != ESP_OK) {
        if (err_code) *err_code = SCREEN_TXN_E_IO;
        return err;
    }

    char sha[65];
    sha256_hex(data, len, sha, sizeof(sha));
    if (sha_out && sha_len) {
        snprintf(sha_out, sha_len, "%s", sha);
    }

    if (index_append(marker.slot, area, name, len, sha) != ESP_OK) {
        if (err_code) *err_code = SCREEN_TXN_E_IO;
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t screen_txn_commit(const char *deploy_id, int *revision_out,
                            char *problems, size_t problems_len)
{
    if (!s_ready || !deploy_id) return ESP_ERR_INVALID_ARG;
    if (problems && problems_len) problems[0] = '\0';

    marker_info_t marker;
    if (read_marker(&marker) != ESP_OK || strcmp(marker.id, deploy_id) != 0) {
        return ESP_ERR_INVALID_STATE;
    }

    int revision = (marker.revision > 0) ? marker.revision : s_revision + 1;
    char slot = marker.slot;

    /* V3: every indexed file must still be present and parseable */
    char idx_path[96];
    slot_index(slot, idx_path, sizeof(idx_path));
    size_t ilen = 0;
    char *itext = read_whole_file(idx_path, &ilen);
    cJSON *entries = itext ? cJSON_Parse(itext) : NULL;
    free(itext);
    if (!cJSON_IsArray(entries)) {
        if (entries) cJSON_Delete(entries);
        return ESP_ERR_INVALID_STATE;
    }

    int bad = 0;
    int screens = 0;
    int images = 0;
    const cJSON *it = NULL;
    cJSON_ArrayForEach(it, entries) {
        const cJSON *a = cJSON_GetObjectItem(it, "area");
        const cJSON *n = cJSON_GetObjectItem(it, "name");
        if (!cJSON_IsString(a) || !cJSON_IsString(n)) continue;
        if (strcmp(a->valuestring, "screens") == 0) screens++;
        else if (strcmp(a->valuestring, "images") == 0) images++;

        char fpath[128];
        slot_file(slot, a->valuestring, n->valuestring, fpath, sizeof(fpath));
        size_t flen = 0;
        char *body = read_whole_file(fpath, &flen);
        bool ok = false;
        if (body) {
            cJSON *p = cJSON_ParseWithLength(body, flen);
            ok = (p != NULL);
            if (p) cJSON_Delete(p);
            free(body);
        }
        if (!ok) {
            bad++;
            append_problem(problems, problems_len, a->valuestring, n->valuestring,
                           "unparseable");
        }
    }

    /* V3: every bitmap a screen names must be staged */
    cJSON_ArrayForEach(it, entries) {
        const cJSON *a = cJSON_GetObjectItem(it, "area");
        const cJSON *n = cJSON_GetObjectItem(it, "name");
        if (!cJSON_IsString(a) || !cJSON_IsString(n)) continue;
        if (strcmp(a->valuestring, "screens")) continue;

        char fpath[128];
        slot_file(slot, "screens", n->valuestring, fpath, sizeof(fpath));
        size_t flen = 0;
        char *body = read_whole_file(fpath, &flen);
        if (!body) continue;
        cJSON *p = cJSON_ParseWithLength(body, flen);
        free(body);
        if (!p) continue;

        const cJSON *bitmaps = cJSON_GetObjectItem(p, "bitmaps");
        const cJSON *bm = NULL;
        cJSON_ArrayForEach(bm, bitmaps) {
            if (!cJSON_IsString(bm) || !bm->valuestring) continue;
            if (!index_has(entries, "images", bm->valuestring)) {
                bad++;
                append_problem(problems, problems_len, "images", bm->valuestring,
                               "missing");
            }
        }
        cJSON_Delete(p);
    }
    cJSON_Delete(entries);

    /* an empty slot must never become active: it would paint nothing and there
     * would be no screen to fall back to */
    if (screens == 0) {
        append_problem(problems, problems_len, "screens", "*", "empty");
        return ESP_ERR_INVALID_STATE;
    }

    if (bad > 0) {
        return ESP_ERR_INVALID_STATE;
    }

    /* the atomic switch */
    esp_err_t err = write_commit(slot, revision, marker.start_screen);
    if (err != ESP_OK) return err;

    unlink(MARKER_FILE);
    if (revision_out) *revision_out = revision;
    s_last_screens = screens;
    s_last_images = images;
    ESP_LOGI(TAG, "committed slot %c revision %d start screen \"%.40s\"", slot, revision,
             marker.start_screen);
    return ESP_OK;
}

esp_err_t screen_txn_abort(const char *deploy_id)
{
    if (!s_ready) return ESP_ERR_INVALID_STATE;

    marker_info_t marker;
    if (read_marker(&marker) != ESP_OK) {
        return ESP_ERR_NOT_FOUND;
    }
    /* A stale abort (second client, retried request) must not wipe the staging
     * area of a newer deploy. */
    if (deploy_id && deploy_id[0] && strcmp(marker.id, deploy_id) != 0) {
        return ESP_ERR_NOT_FOUND;
    }

    wipe_slot(marker.slot);
    unlink(MARKER_FILE);
    ESP_LOGW(TAG, "deploy %s aborted, slot %c discarded", marker.id, marker.slot);
    return ESP_OK;
}

esp_err_t screen_txn_reset_to_seed(void)
{
    /* Flip the commit record to the empty slot first: if that write fails the
     * panel keeps serving the previous set instead of nothing. */
    char prev = s_active;
    char target = other_slot(prev);
    wipe_slot(target);
    unlink(MARKER_FILE);

    esp_err_t err = write_commit(target, 0, "start_up_screen");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "factory reset failed, still on slot %c", prev);
        return err;
    }

    wipe_slot(prev);
    s_ready = true;
    ESP_LOGW(TAG, "reset to seed: active slot %c at revision 0", s_active);
    return ESP_OK;
}
