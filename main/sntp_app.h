#ifndef SNTP_APP_H
#define SNTP_APP_H

#include <types.h>

extern uint8_t sntpc_Conns_State;
extern uint8_t flag_send_udp_timesync;

#define SNTP_STATE_NOTREADY		0
#define SNTP_STATE_INITIAL		1
#define SNTP_STATE_WAIT			2
#define SNTP_STATE_TIMEOUT		3
#define SNTP_STATE_GET_DONE		4


#define MAX_SNTP_RETRY_COUNT 5
extern uint32_t count_sntp;
extern uint8_t flag_Update_Sntp;
extern uint8_t Update_Sntp_Retry;
extern char sntp_server[30];

void sntp_select_time_server(uint8_t type);

void update_sntp(void);
void Send_TimeSync_Broadcast(uint8_t protocal);
u8_t sntp_getoperatingmode(void);


#endif
