/* LwIP SNTP example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
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
//#include "protocol_examples_common.h"
#include "esp_sntp.h"
#include "sntp_app.h"
#include "user_data.h"
#include "define.h"

static const char *TAG = "example";

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
RTC_DATA_ATTR static int boot_count = 0;
uint16_t Test[50];
static void obtain_time(void);
static void initialize_sntp(void);

uint8_t sntpc_Conns_State;

uint8_t flag_send_udp_timesync;

uint32_t Rtc_Set(uint16_t syear, uint8_t smon, uint8_t sday, uint8_t hour, uint8_t min, uint8_t sec, uint8_t flag);
void Send_TimeSync_Broadcast(uint8_t protocal);
void udp_client_send(uint16 time);

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_CUSTOM
void sntp_sync_time(struct timeval *tv)
{
   settimeofday(tv, NULL);
   ESP_LOGI(TAG, "Time is synchronized from custom code");
   sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
}
#endif
void debug_info(char *string);
void Get_RTC_by_timestamp(U32_T timestamp,UN_Time* rtc,U8_T source);
int PCF_systohc();
void time_sync_notification_cb(struct timeval *tv)
{
    //ESP_LOGI(TAG, "Notification of a time synchronization event");
    //struct tm timeinfo = {0};
//	ESP_LOGI(UTC_TAG, "tv_sec: %lld", (uint64_t)tv->tv_sec);
    //Test[36]++;
    debug_info("------ntp completed");

	Get_RTC_by_timestamp(tv->tv_sec,&Rtc,1);  // timer sever
	Rtc_Set(Rtc.Clk.year,Rtc.Clk.mon,Rtc.Clk.day,Rtc.Clk.hour,Rtc.Clk.min,Rtc.Clk.mon,0);
	update_timers();
	update_sntp_last_time = get_current_time();

	Setting_Info.reg.update_sntp_last_time = update_sntp_last_time;
	sntpc_Conns_State = SNTP_STATE_GET_DONE;
	Setting_Info.reg.sync_with_ntp_result = 1;
	flag_Update_Sntp = 1;
	sntp_stop();

	flag_send_udp_timesync = 1;

	if(Modbus.com_config[2] == BACNET_MASTER || Modbus.com_config[0] == BACNET_MASTER)
		Send_TimeSync_Broadcast(BAC_MSTP);

}




uint8_t SNTPC_GetState(void)
{
	return sntpc_Conns_State;
}

uint32_t count_sntp = 0;
uint8_t flag_Update_Sntp;
uint8_t Update_Sntp_Retry;
char sntp_server[30];

// update_sntp() should be called every 1s by default
void update_sntp(void)
{
	uint8_t state;
	uint8_t *time_ptr;
	uint8_t temp[48];
	time_ptr = temp;
	static uint32_t count_sntp = 0;
	if(Modbus.en_sntp >= 2)  // enable
	{
		count_sntp++;
		if(flag_Update_Sntp == 0)
		{
			state = SNTPC_GetState();
			if(SNTP_STATE_GET_DONE  == state)
			{
				Update_Sntp_Retry = 0;
				flag_Update_Sntp = 1;
				count_sntp = 0;
			}
			else
			{
				if(Update_Sntp_Retry < MAX_SNTP_RETRY_COUNT)
				{
					if(count_sntp % 20 == 0)
					{
						debug_info("sntp_select_time_server");
						if(Setting_Info.reg.en_time_sync_with_pc == 0)
						{// udpate with NTP
							sntp_select_time_server(Modbus.en_sntp);
						}
						Update_Sntp_Retry++;
					}
				}
				else
				{debug_info("update SNTP fail");
					// update SNTP fail
					//generate_common_alarm(ALARM_SNTP_FAIL);
					Update_Sntp_Retry = 0;
					flag_Update_Sntp = 1;
					sntpc_Conns_State = SNTP_STATE_TIMEOUT;
					sntp_stop();
				}


		  }
		}
		else
		{
			if(count_sntp > 24 * 3600)  // update per 1 day
			{
				debug_info("update SNTP per min");
				flag_Update_Sntp = 0;
				Update_Sntp_Retry = 0;
				count_sntp = 0;
				sntp_select_time_server(Modbus.en_sntp);
			}
		}
	}

}

static void initialize_sntp(void)
{

    ESP_LOGI(TAG, "Initializing SNTP");
    sntpc_Conns_State = SNTP_STATE_INITIAL;
    debug_info("SNTP INTIAL");
    if(sntp_enabled())
	{// sntp_pcb != null
		sntp_stop();
		debug_info("SNTP STOP");
	}

   	sntp_setoperatingmode(SNTP_OPMODE_POLL);

	sntp_setservername(0, sntp_server);
    sntp_setservername(1, "cn.pool.ntp.org");
    sntp_setservername(2, "time.nist.gov");
    sntp_setservername(3, "time.windows.com");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
	sntp_init();
    sntpc_Conns_State = SNTP_STATE_WAIT;
}


void sntp_select_time_server(uint8_t type)
{
	if(type < 2 || type > 5) return;

	initialize_sntp();
}

