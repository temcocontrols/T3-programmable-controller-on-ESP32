/*
 * sntp_app.c  —  SNTP for ESP32
 *   Variables (extern):
 *     uint8_t  sntpc_Conns_State       — state read by other modules
 *     uint8_t  flag_Update_Sntp        — read by other modules
 *     uint8_t  Update_Sntp_Retry       — read by other modules
 *     uint32_t count_sntp              — kept for compatibility
 *     uint8_t  flag_send_udp_timesync  — set here, consumed elsewhere
 *     char     sntp_server[30]         — filled by caller before init
 *
 *   Functions (called from other files):
 *     void        update_sntp(void)               — call every 1 s
 *     void        sntp_select_time_server(uint8_t)— triggers a (re)start
 *     uint8_t     SNTPC_GetState(void)            — returns sntpc_Conns_State
 *     void        time_sync_notification_cb(...)  — registered with esp_sntp
 */

#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "sntp_app.h"
#include "user_data.h"
#include "define.h"
#include "wifi.h"
#include "esp_sntp.h"

static const char *TAG = "sntp";

/* ── Tunables ─────────────────────────────────────────────────────────────── */
#define SNTP_RETRY_INTERVAL_SEC   30    /* seconds between re-init attempts    */
#define SNTP_MAX_RETRY_COUNT      10    /* retries before declaring timeout    */
#define SNTP_RESYNC_INTERVAL_SEC  (24u * 3600u)  /* daily re-sync             */
#define SNTP_MIN_VALID_YEAR       2024  /* reject timestamps older than this   */

/* ── Externally visible variables (used by other files — do NOT rename) ───── */
uint8_t  sntpc_Conns_State;
uint8_t  flag_send_udp_timesync;
uint8_t  flag_Update_Sntp;
uint8_t  Update_Sntp_Retry;
uint32_t count_sntp;                   /* kept for ABI compat; not used here  */
char     sntp_server[30];              /* caller fills this before first call  */

/* ── Forward declarations of helpers defined elsewhere ───────────────────── */
/* matches declaration in wifi.h */
void     Get_RTC_by_timestamp(U32_T timestamp, UN_Time *rtc, U8_T source);
int      PCF_systohc(void);
void     update_timers(void);
uint32_t get_current_time(void);
uint32_t Rtc_Set(uint16_t syear, uint8_t smon, uint8_t sday,
                 uint8_t hour,   uint8_t min,  uint8_t sec, uint8_t flag);
void     Send_TimeSync_Broadcast(uint8_t protocol);

/* ── Module-private state ─────────────────────────────────────────────────── */
static uint32_t s_retry_tick  = 0;   /* seconds elapsed in current attempt   */
static uint32_t s_resync_tick = 0;   /* seconds elapsed since last good sync */

/* ── Internal helpers ─────────────────────────────────────────────────────── */

static void sntp_stop_safe(void)
{
    if (esp_sntp_enabled()) {
        esp_sntp_stop();
        debug_info((char *)"SNTP STOP");
    }
}

static bool timestamp_is_sane(time_t ts)
{
    struct tm t;
    gmtime_r(&ts, &t);
    int year = t.tm_year + 1900;
    if (year < SNTP_MIN_VALID_YEAR) {
        ESP_LOGW(TAG, "Suspicious NTP year %d — ignoring", year);
        return false;
    }
    return true;
}

static void sntp_do_init(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntpc_Conns_State = SNTP_STATE_INITIAL;
    debug_info((char *)"SNTP INTIAL");

    sntp_stop_safe();

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);

    /* Slot 0: caller-supplied server (sntp_server[] set before calling us) */
    if(Modbus.en_sntp == 5 && sntp_server[0] != '\0')
    {
        ESP_LOGI(TAG, "Using custom SNTP server from Modbus config: '%s'", sntp_server);
    }
    else
    {
        /* Default to pool.ntp.org if caller didn't set a server */
        strncpy(sntp_server, "pool.ntp.org", sizeof(sntp_server));
        sntp_server[sizeof(sntp_server) - 1] = '\0';
        ESP_LOGW(TAG, "No custom SNTP server configured; defaulting to '%s'", sntp_server);
    }
    esp_sntp_setservername(0, sntp_server);
    esp_sntp_setservername(1, "cn.pool.ntp.org");
    esp_sntp_setservername(2, "time.cloudflare.com");  /* reliable anycast   */
    esp_sntp_setservername(3, "time.google.com");

    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    esp_sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif

    esp_sntp_init();

    sntpc_Conns_State = SNTP_STATE_WAIT;
    s_retry_tick      = 0;
}

/* ── SNTP callback — called from lwIP task on successful sync ─────────────── */
void time_sync_notification_cb(struct timeval *tv)
{
    if (!timestamp_is_sane(tv->tv_sec)) {
        /*
         * Bad timestamp: stay in WAIT state so the retry logic will
         * attempt another sync after SNTP_RETRY_INTERVAL_SEC seconds.
         */
        ESP_LOGW(TAG, "NTP returned bad time — will retry");
        return;
    }

    debug_info((char *)"------ntp completed");

    /* Update application RTC */
    Get_RTC_by_timestamp(tv->tv_sec, &Rtc, 1);

    /*
     * BUG FIX: original passed Rtc.Clk.mon as the seconds argument.
     * Correct parameter order: year, mon, day, hour, min, SEC, flag
     */
    Rtc_Set(Rtc.Clk.year,
            Rtc.Clk.mon,
            Rtc.Clk.day,
            Rtc.Clk.hour,
            Rtc.Clk.min,
            Rtc.Clk.sec,   /* ← fixed: was Rtc.Clk.mon */
            0);

    update_timers();

    update_sntp_last_time = get_current_time();
    Setting_Info.reg.update_sntp_last_time = update_sntp_last_time;

    sntpc_Conns_State = SNTP_STATE_GET_DONE;
    Setting_Info.reg.sync_with_ntp_result = 1;

    flag_Update_Sntp  = 1;
    Update_Sntp_Retry = 0;
    s_resync_tick     = 0;   /* reset 24-hour counter */

    sntp_stop_safe();        /* we drive re-sync from update_sntp(); stop polling */

    /* Propagate time to network peers */
    if (Modbus.network_master == 1)
        flag_send_udp_timesync = 1;

    if (Modbus.com_config[0] == BACNET_MASTER ||
        Modbus.com_config[0] == BACNET_SLAVE  ||
        Modbus.com_config[2] == BACNET_MASTER ||
        Modbus.com_config[2] == BACNET_SLAVE)
        Send_TimeSync_Broadcast(BAC_MSTP);
}

/* ── Public API ───────────────────────────────────────────────────────────── */

uint8_t SNTPC_GetState(void)
{
    return sntpc_Conns_State;
}

/*
 * sntp_select_time_server()
 * Called by application code to (re)start SNTP with the selected server type.
 * Signature unchanged from original.
 */
void sntp_select_time_server(uint8_t type)
{
    if (type < 2 || type > 5) return;
    sntp_do_init();
}

/*
 * update_sntp()
 * Call every 1 second.  Drives retry logic and daily re-sync.
 * All variable names and external behaviour identical to original.
 *
 * Key differences:
 *  - Retry fires every SNTP_RETRY_INTERVAL_SEC (30 s) not every 20 s
 *  - Won't restart while a sync is genuinely in progress within the window
 *  - Daily re-sync uses s_resync_tick (private) not the shared count_sntp
 */
void update_sntp(void)
{
    /* WireGuard gateway: force NTP on, disable PC time-sync */
    if (Modbus.mini_type == PROJECT_WIREGUARD_GATEWAY)
    {
        Modbus.en_sntp = 2;
        Setting_Info.reg.en_time_sync_with_pc = 0;
    }

    if (Modbus.en_sntp < 2)
        return;

    /* Guard: do nothing if Wi-Fi isn't ready or PC sync is active */
    if (SSID_Info.IP_Wifi_Status != WIFI_NORMAL)
        return;
    if (Setting_Info.reg.en_time_sync_with_pc != 0)
        return;

    static bool first_call = true;
    if (first_call)
    {
        s_retry_tick = SNTP_RETRY_INTERVAL_SEC;  /* trigger immediate sync on first call */
        first_call = false;
    }
    if (flag_Update_Sntp == 0) {
        /* ── Waiting for first / retry sync ─────────────────────── */
        s_retry_tick++;

        if (sntpc_Conns_State == SNTP_STATE_GET_DONE) {
            /* Callback already marked success — shouldn't normally land here,
             * but handle it cleanly just in case.                            */
            Update_Sntp_Retry = 0;
            flag_Update_Sntp  = 1;
            s_retry_tick      = 0;
            return;
        }

        if (s_retry_tick >= SNTP_RETRY_INTERVAL_SEC) {
            s_retry_tick = 0;

            if (Update_Sntp_Retry < SNTP_MAX_RETRY_COUNT) {
                Update_Sntp_Retry++;
                ESP_LOGW(TAG, "NTP retry %d/%d", Update_Sntp_Retry, SNTP_MAX_RETRY_COUNT);
                debug_info((char *)"sntp_select_time_server");
                sntp_select_time_server(Modbus.en_sntp);
            } else {
                /* All retries exhausted */
                ESP_LOGE(TAG, "NTP sync failed after %d retries", Update_Sntp_Retry);
                debug_info((char *)"update SNTP fail");

                Update_Sntp_Retry = 0;
                flag_Update_Sntp  = 1;
                sntpc_Conns_State = SNTP_STATE_TIMEOUT;
                sntp_stop_safe();
            }
        }

    } else {
        /* ── Sync done — wait for daily re-sync ─────────────────── */
        s_resync_tick++;

        if (s_resync_tick >= SNTP_RESYNC_INTERVAL_SEC) {
            ESP_LOGI(TAG, "24-hour re-sync triggered");
            debug_info((char *)"update SNTP per day");

            flag_Update_Sntp  = 0;
            Update_Sntp_Retry = 0;
            s_resync_tick     = 0;
            s_retry_tick      = 0;

            sntp_select_time_server(Modbus.en_sntp);
        }
    }
}