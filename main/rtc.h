#ifndef __RTC_H
#define __RTC_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#define PCF8563_READ_ADDR               0xA3
#define PCF8563_WRITE_ADDR              0xA2

#define PCF_ALARM_FLAG                  (1<<3)
#define PCF_TIMER_FLAG                  (1<<2)
#define PCF_ALARM_INTERRUPT_ENABLE      (1<<1)
#define PCF_TIMER_INTERRUPT_ENABLE      (1<<0)

#define PCF_CLKOUT_32768HZ              0b10000000
#define PCF_CLKOUT_1024HZ               0b10000001
#define PCF_CLKOUT_32HZ                 0b10000010
#define PCF_CLKOUT_1HZ                  0b10000011
#define PCF_CLKOUT_DISABLED             0b00000000

#define PCF_TIMER_4096HZ                0b10000000
#define PCF_TIMER_64HZ                  0b10000001
#define PCF_TIMER_1HZ                   0b10000010
#define PCF_TIMER_1_60HZ                0b10000011
#define PCF_TIMER_DISABLED              0b00000011

#define PCF_DISABLE_ALARM               80


typedef struct {
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t weekday;
} PCF_Alarm;

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t weekday;
    uint8_t month;
    uint16_t day_of_year;
    uint16_t year;
} PCF_DateTime;

typedef struct _sntpheader
{
	uint8_t	flags;
	uint8_t	stratum;
	uint8_t	poll;
	uint8_t	precision;
	uint32_t	root_delay;
	uint32_t	root_dispersion;
	uint32_t	reference_ID;
	uint32_t	reference_time1;
	uint32_t	reference_time2;
	uint32_t	originate_time1;
	uint32_t	originate_time2;
	uint32_t	receive_time1;
	uint32_t	receive_time2;
	uint32_t	transmit_time1;
	uint32_t	transmit_time2;

} SNTPHeader;

typedef struct _timeinfo
{
	SNTPHeader	*sntpcPktPtr;
	uint32_t		timestamp;
	uint32_t		second_remain;
	uint32_t		day_total;
	uint16_t		YY;
	uint8_t		MM;
	uint16_t		DD;
	int16_t		DD_r;
	uint8_t		HH;
	uint8_t		MI;
	uint16_t		MI_r;
	uint8_t		SS;
} TimeInfo;

extern PCF_DateTime rtc_date;
extern int16_t  timezone;
extern uint8_t  Daylight_Saving_Time;
extern uint32_t  system_timer;

int PCF_Init(uint8_t mode);

esp_err_t PCF_Write(uint8_t addr, uint8_t *data, size_t count);
esp_err_t PCF_Read(uint8_t addr, uint8_t *data, size_t count);
esp_err_t PCF_GetLastError();
int PCF_GetAndClearFlags(void);
int PCF_SetClockOut(uint8_t mode);
int PCF_SetTimer(uint8_t mode, uint8_t count);
int PCF_GetTimer(void);
int PCF_SetAlarm(PCF_Alarm *alarm);
int PCF_GetAlarm(PCF_Alarm *alarm);
int PCF_SetDateTime(PCF_DateTime *dateTime);
int PCF_GetDateTime(PCF_DateTime *dateTime);
int PCF_hctosys();
int PCF_systohc();
extern void rtc_task(void *arg);

#endif
