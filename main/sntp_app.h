/*
 * sntp_app.h
 * All symbols identical to original — no other file needs changing.
 */

#ifndef SNTP_APP_H
#define SNTP_APP_H

#include <stdint.h>
#include "sntp.h"

/* ── SNTP state values (sntpc_Conns_State) ────────────────────────────────── */
#define SNTP_STATE_INITIAL   0
#define SNTP_STATE_WAIT      1
#define SNTP_STATE_GET_DONE  2
#define SNTP_STATE_TIMEOUT   3

/* ── Extern variables (defined in sntp_app.c, used by other files) ────────── */
extern uint8_t  sntpc_Conns_State;
extern uint8_t  flag_send_udp_timesync;
extern uint8_t  flag_Update_Sntp;
extern uint8_t  Update_Sntp_Retry;
extern uint32_t count_sntp;
extern char     sntp_server[30];

/* ── Public functions ─────────────────────────────────────────────────────── */
void    update_sntp(void);                        /* call every 1 s            */
void    sntp_select_time_server(uint8_t type);    /* trigger a (re)start       */
uint8_t SNTPC_GetState(void);                     /* returns sntpc_Conns_State */
void    Send_TimeSync_Broadcast(uint8_t protocol);/* broadcast time to peers   */

/* Registered with esp_sntp — do not rename */
void    time_sync_notification_cb(struct timeval *tv);

#endif /* SNTP_APP_H */
