#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#define SCREEN_TXN_PARTITION "screen_data"

typedef enum {
    SCREEN_TXN_OK = 0,
    SCREEN_TXN_E_ARG,
    SCREEN_TXN_E_NO_TXN,
    SCREEN_TXN_E_BAD_JSON,
    SCREEN_TXN_E_TOO_LARGE,
    SCREEN_TXN_E_NO_SPACE,
    SCREEN_TXN_E_IO,
} screen_txn_error_t;

typedef struct {
    size_t total;
    size_t used;
    size_t free;
    size_t slot_free;
    size_t reserved;
    size_t max_file;
    char active_slot;
    int revision;
} screen_txn_capacity_t;

/* active slot / revision, valid after screen_txn_init() */
const char *screen_txn_active_slot(void);
int screen_txn_revision(void);
bool screen_txn_is_ready(void);

/* start screen of the last successful commit ("" when unset) */
const char *screen_txn_start_screen(void);

/* screen/image counts of the last successful commit */
void screen_txn_last_counts(int *screens, int *images);

/* reads <area>/<name>.json from the active slot; buffer is malloc'd and owned
 * by the caller */
esp_err_t screen_txn_read_active(const char *area, const char *name,
                                 char **out_buf, size_t *out_len);

/* path of a file inside the active slot: /spiffs/<A|B>/<area>/<name>.json */
void screen_slot_path(const char *area, const char *name, char *buf, size_t n);

esp_err_t screen_txn_init(void);
esp_err_t screen_txn_capacity(screen_txn_capacity_t *out);

esp_err_t screen_txn_begin(const char *deploy_id, int revision, const char *start_screen);
esp_err_t screen_txn_stage(const char *deploy_id, const char *area, const char *name,
                           const char *data, size_t len, char *sha_out, size_t sha_len,
                           screen_txn_error_t *err_code);
esp_err_t screen_txn_commit(const char *deploy_id, int *revision_out,
                            char *problems, size_t problems_len);
esp_err_t screen_txn_abort(const char *deploy_id);
esp_err_t screen_txn_reset_to_seed(void);
