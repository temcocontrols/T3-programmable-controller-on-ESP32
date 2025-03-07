#include "rtc.h"
#include "driver/i2c.h"
#include "timegm.h"

#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
//#include "deviceparams.h"
#include "wifi.h"
#include "user_data.h"
#include "define.h"
//#include "i2c_task.h"

extern uint16_t Test[50];
int16_t  timezone;
uint8_t  Daylight_Saving_Time;
U32_T RTC_GetCounter(void);
void Get_Time_by_sec(u32 sec_time,UN_Time * rtc, uint8_t flag);
U32_T get_current_time_with_timezone(void);

const uint8_t	Month[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
const uint8_t	AddMonth[12] = {31,29,31,30,31,30,31,31,30,31,30,31};

static esp_err_t last_i2c_err = ESP_OK;
//UN_Time Rtc;
esp_err_t PCF_Write(uint8_t addr, uint8_t *data, size_t count) {

	last_i2c_err = ESP_OK;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, PCF8563_WRITE_ADDR, true);
	i2c_master_write_byte(cmd, addr, true);
	i2c_master_write(cmd, data, count, true);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
	last_i2c_err = ret;
//	printf("last_i2c_err %d\n", last_i2c_err);
    return ret;
}

esp_err_t PCF_Read(uint8_t addr, uint8_t *data, size_t count) {

	last_i2c_err = ESP_OK;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, PCF8563_WRITE_ADDR, true);
	i2c_master_write_byte(cmd, addr, true);
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, PCF8563_READ_ADDR, true);
	i2c_master_read(cmd, data, count, I2C_MASTER_LAST_NACK);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_1, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
	last_i2c_err = ret;
//	printf("last_i2c_err %d\n", last_i2c_err);
    return ret;
}

esp_err_t PCF_GetLastError(){
	return last_i2c_err;
}

#define BinToBCD(bin) ((((bin) / 10) << 4) + ((bin) % 10))

int PCF_Init(uint8_t mode){
	static bool init = false;
	if(!init){
		uint8_t tmp = 0b00000000;
		esp_err_t ret = PCF_Write(0x00, &tmp, 1);
		if (ret != ESP_OK){
			return -1;
		}
		mode &= 0b00010011;
		ret = PCF_Write(0x01, &mode, 1);
		if (ret != ESP_OK){
			return -2;
		}
		init = true;
	}
	return 0;
}

int PCF_GetAndClearFlags(){
	uint8_t flags;

	esp_err_t ret = PCF_Read(0x01, &flags, 1);
	if (ret != ESP_OK){
		return -1;
	}
	uint8_t cleared = flags & 0b00010011;
	ret = PCF_Write(0x01, &cleared, 1);
	if (ret != ESP_OK){
		return -1;
	}

	return flags & 0x0C;
}

int PCF_SetClockOut(uint8_t mode){

	mode &= 0b10000011;
	esp_err_t ret = PCF_Write(0x0D, &mode, 1);
	if (ret != ESP_OK) {
		return -1;
	}
	return 0;
}

int PCF_SetTimer(uint8_t mode, uint8_t count){

	mode &= 0b10000011;
	esp_err_t ret = PCF_Write(0x0E, &mode, 1);
	if (ret != ESP_OK) {
		return -1;
	}
	ret = PCF_Write(0x0F, &count, 1);
	if (ret != ESP_OK) {
		return -1;
	}
	return 0;
}

int PCF_GetTimer(){
	uint8_t count;

	esp_err_t ret = PCF_Read(0x0F, &count, 1);
	if (ret != ESP_OK) {
		return -1;
	}
	return (int) count;
}

int PCF_SetAlarm(PCF_Alarm *alarm){
	if ((alarm->minute >= 60 && alarm->minute != 80) || (alarm->hour >= 24 && alarm->hour != 80) || (alarm->day > 32 && alarm->day != 80) || (alarm->weekday > 6 && alarm->weekday != 80))
	{
		return -2;
	}

	uint8_t buffer[4];

	buffer[0] = BinToBCD(alarm->minute) & 0xFF;
	buffer[1] = BinToBCD(alarm->hour) & 0xBF;
	buffer[2] = BinToBCD(alarm->day) & 0xBF;
	buffer[3] = BinToBCD(alarm->weekday) & 0x87;

	esp_err_t ret = PCF_Write(0x09, buffer, sizeof(buffer));
	if (ret != ESP_OK) {
		return -1;
	}

	return 0;
}

int PCF_GetAlarm(PCF_Alarm *alarm) {
	uint8_t buffer[4];

	esp_err_t ret = PCF_Read(0x09, buffer, sizeof(buffer));
	if (ret != ESP_OK) {
		return -1;
	}

	alarm->minute = (((buffer[0] >> 4) & 0x0F) * 10) + (buffer[0] & 0x0F);
	alarm->hour = (((buffer[1] >> 4) & 0x0B) * 10) + (buffer[1] & 0x0F);
	alarm->day = (((buffer[2] >> 4) & 0x0B) * 10) + (buffer[2] & 0x0F);
	alarm->weekday = (((buffer[3] >> 4) & 0x08) * 10) + (buffer[3] & 0x07);

	return 0;
}

int PCF_SetDateTime(PCF_DateTime *dateTime) {
	if (dateTime->second >= 60 || dateTime->minute >= 60 || dateTime->hour >= 24 || dateTime->day > 32 || dateTime->weekday > 6 || dateTime->month > 12 || dateTime->year < 1900 || dateTime->year >= 2100)
	{
		return -2;
	}

	uint8_t buffer[7];

	buffer[0] = BinToBCD(dateTime->second) & 0x7F;
	buffer[1] = BinToBCD(dateTime->minute) & 0x7F;
	buffer[2] = BinToBCD(dateTime->hour) & 0x3F;
	buffer[3] = BinToBCD(dateTime->day) & 0x3F;
	buffer[4] = BinToBCD(dateTime->weekday) & 0x07;
	buffer[5] = BinToBCD(dateTime->month) & 0x1F;

	if (dateTime->year >= 2000)
	{
		buffer[5] |= 0x80;
		buffer[6] = BinToBCD(dateTime->year - 2000);
	}
	else
	{
		buffer[6] = BinToBCD(dateTime->year - 1900);
	}

	esp_err_t ret = PCF_Write(0x02, buffer, sizeof(buffer));
	if (ret != ESP_OK) {
		return -1;
	}

	return 0;
}

int PCF_GetDateTime(PCF_DateTime *dateTime) {
	uint8_t buffer[7];
	esp_err_t ret;

	ret = PCF_Read(0x02, buffer, sizeof(buffer));
	if (ret != ESP_OK) {
		return -1;
	}

	dateTime->second = (((buffer[0] >> 4) & 0x07) * 10) + (buffer[0] & 0x0F);
	dateTime->minute = (((buffer[1] >> 4) & 0x07) * 10) + (buffer[1] & 0x0F);
	dateTime->hour = (((buffer[2] >> 4) & 0x03) * 10) + (buffer[2] & 0x0F);
	dateTime->day = (((buffer[3] >> 4) & 0x03) * 10) + (buffer[3] & 0x0F);
	dateTime->weekday = (buffer[4] & 0x07);
	dateTime->month = ((buffer[5] >> 4) & 0x01) * 10 + (buffer[5] & 0x0F);
	dateTime->year = 1900 + ((buffer[6] >> 4) & 0x0F) * 10 + (buffer[6] & 0x0F);

	if (buffer[5] &  0x80)
	{
		dateTime->year += 100;
	}

	if (buffer[0] & 0x80) //Clock integrity not guaranted
	{
		return 1;
	}

	Rtc.Clk.hour = dateTime->hour;
	Rtc.Clk.min = dateTime->minute;
	Rtc.Clk.sec = dateTime->second;
	Rtc.Clk.day = dateTime->day;
	Rtc.Clk.week = dateTime->weekday;
	Rtc.Clk.mon = dateTime->month;
	Rtc.Clk.year = dateTime->year;

	return 0;
}
PCF_DateTime rtc_date = {0};
int PCF_hctosys(){
	int ret;

	struct tm tm = {0};
	struct timeval tv = {0};

	ret = PCF_Init(0);
//	printf("PCF_Init %d\n", ret);
	debug_info("PCF_Init");
	if(ret == 0)
	{
		debug_info("PCF_INIT_SUCCESS");
	}
	if (ret != 0) {
		debug_info("PCF_INIT_FAILED");
		goto fail;
	}
    ret = PCF_GetDateTime(&rtc_date);
//	printf("PCF_GetDateTime %d\n", ret);
    if (ret != 0) {
		goto fail;
    }
	tm.tm_sec = rtc_date.second;
	tm.tm_min = rtc_date.minute;
	tm.tm_hour = rtc_date.hour;
	tm.tm_mday = rtc_date.day;
	tm.tm_mon = rtc_date.month - 1;
	tm.tm_year = rtc_date.year - 1900;

	tv.tv_sec = timegm(&tm);
	tv.tv_usec = 0;
	ret = settimeofday(&tv, NULL);
fail:
	return ret;
}

uint8_t const table_week[12] = {0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5};

uint8_t RTC_Get_Week(uint16_t year, uint8_t month, uint8_t day)
{
	uint16_t temp2;
	uint8_t yearH, yearL;

	yearH = year / 100;
	yearL = year % 100;

	if(yearH > 19)	//
		yearL += 100;


	temp2 = yearL + yearL / 4;
	temp2 = temp2 % 7;
	temp2 = temp2 + day + table_week[month - 1];

	if(yearL % 4 == 0 && month < 3)
		temp2--;

	return(temp2 % 7);
}

int PCF_systohc(){
	int ret;
//	PCF_DateTime date = {0};
	struct tm tm = {0};

	ret = PCF_Init(0);
	if (ret != 0) {
		goto fail;
	}

	time_t now = time(NULL);
	gmtime_r(&now, &tm);
	rtc_date.second = tm.tm_sec;
	rtc_date.minute = tm.tm_min;
	rtc_date.hour = tm.tm_hour;
	rtc_date.day = tm.tm_mday;
	rtc_date.month = tm.tm_mon + 1;
	rtc_date.year = tm.tm_year + 1900;
	rtc_date.weekday = RTC_Get_Week(rtc_date.year,rtc_date.month,rtc_date.day);//tm.tm_wday;
	ret = PCF_SetDateTime(&rtc_date);


fail:
	return ret;
}



void update_timers( void )
{
	int  i, year = 0;

	uint32_t  ora_current_sec;  /* seconds since the beginning of the day */
	uint16_t day_of_year;
	uint8_t month_length[12];

	month_length[0] = 31;
	month_length[1] = 28;
	month_length[2] = 31;
	month_length[3] = 30;
	month_length[4] = 31;
	month_length[5] = 30;
	month_length[6] = 31;
	month_length[7] = 31;
	month_length[8] = 30;
	month_length[9] = 31;
	month_length[10] = 30;
	month_length[11] = 31;

	if(rtc_date.year % 4 == 0)
		month_length[1] = 29;
	else
		month_length[1] = 28;
	/* seconds since the beginning of the day */

	ora_current_sec = 3600L * rtc_date.hour;
	ora_current_sec += 60L * rtc_date.minute;
	ora_current_sec += rtc_date.second;

	day_of_year = 0;
	//if(Rtc.Clk.mon > 0)
	{
		for( i=0; i< rtc_date.month - 1; i++ )
		{
			day_of_year += month_length[i];
		}
	}
	day_of_year +=  rtc_date.day;
	rtc_date.day_of_year = day_of_year;
/*	timestart = 0;*/ /* seconds since the beginning of the year */
	timestart = 86400L * (day_of_year - 1); /* 86400L = 3600L * 24;*/
	timestart += ora_current_sec;

	time_since_1970 = 0; /* seconds since 1970 */
	if( rtc_date.year < 70 )
		year = 100 + rtc_date.year;
	else
		year =  rtc_date.year - 1900;

	for( i = 70; i < year; i++ )
	{
		time_since_1970 += 31536000L;
		if(i % 4 == 0)  // leap year
			time_since_1970 += 86400L;
	}

	if(timezone >= 0)
		time_since_1970 -= (U16_T)timezone * 36;
	else
		time_since_1970 -= (S16_T)timezone * 36;


	if(Daylight_Saving_Time)
	{
		time_since_1970 += 3600;
	}

	time_since_1970 += timestart;
	Get_Time_by_sec(get_current_time_with_timezone(),&Rtc,1);

}

U32_T get_current_time(void)  // orignal data
{
//	Test[20]++;
//	memcpy(&Test[22],time_since_1970,4);
	return time_since_1970 + system_timer / 1000;
}

U32_T get_current_time_with_timezone(void)
{
	if(Daylight_Saving_Time)  // timezone : +8 ---> 800
	{
		if((rtc_date.day_of_year >= start_day) && (rtc_date.day_of_year <= end_day))
		{
			return time_since_1970 + system_timer / 1000 - (S16_T)timezone * 36 - 3600;
		}
		else
			return time_since_1970 + system_timer / 1000 - (S16_T)timezone * 36;

	}
	else
		return time_since_1970 + system_timer / 1000 - (S16_T)timezone * 36;

	return 0;

}

U32_T RTC_GetCounter(void)
{
	return time_since_1970 + system_timer / 1000;
}



u8 Is_Leap_Year(u16 year)
{
	if(year % 4 == 0)
	{
		if(year % 100 == 0)
		{
			if(year % 400 == 0)
				return 1;
			else
				return 0;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		return 0;
	}
}

const u8 mon_table[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
void Get_Time_by_sec(u32 sec_time,UN_Time * rtc, uint8_t flag)
{
	static u16 daycnt = 0;
	u32 temp = 0;
	u16 temp1 = 0;


 	temp = sec_time / 86400;
	if(flag == 0)
	{
		daycnt = 0;
	}
	if(daycnt != temp)
	{
		daycnt = temp;
		temp1 = 1970;
		while(temp >= 365)
		{
			if(Is_Leap_Year(temp1))
			{
				if(temp >= 366)
				{
					temp -= 366;
				}
				else
				{
					break;
				}
			}
			else
			{
				temp -= 365;
			}
			temp1++;
		}
		rtc->Clk.year = temp1 - 2000;
		rtc->Clk.day_of_year = temp + 1;  // get day of year, added by chelsea
		temp1 = 0;
		while(temp >= 28)
		{
			if(Is_Leap_Year(rtc->Clk.year) && temp1 == 1)
			{
				if(temp >= 29)
					temp -=	29;
				else
					break;
			}
			else
			{
				if(temp >= mon_table[temp1])
					temp -= mon_table[temp1];
				else
					break;
			}
			temp1++;
		}
		rtc->Clk.mon = temp1 + 1;
		rtc->Clk.day = temp + 1;
	}
	temp = sec_time % 86400;
	rtc->Clk.hour = temp / 3600;
	rtc->Clk.min = (temp % 3600) / 60;
	rtc->Clk.sec = (temp % 3600) % 60;
	rtc->Clk.week = RTC_Get_Week(2000 + rtc->Clk.year, rtc->Clk.mon,rtc->Clk.day);

	if(flag == 1)
	{
	Local_Date.year = rtc_date.year;
	Local_Date.month = rtc_date.month;
	Local_Date.day = rtc_date.day;
	Local_Date.wday = rtc_date.weekday;

	Local_Time.hour = rtc_date.hour;
	Local_Time.min = rtc_date.minute;
	Local_Time.sec = rtc_date.second;
	}
}

//U32_T Rtc_Set(U16_T syear, U8_T smon, U8_T sday, U8_T hour, U8_T min, U8_T sec, U8_T flag)
uint32_t Rtc_Set(uint16_t syear, uint8_t smon, uint8_t sday, uint8_t hour, uint8_t min, uint8_t sec, uint8_t flag)
{
	if(flag == 0)
	{
		rtc_date.second = sec;
		rtc_date.minute = min;
		rtc_date.hour = hour;
		rtc_date.month = smon;
		rtc_date.year = syear + 2000;
		rtc_date.day = sday;
		system_timer = 0;
		PCF_SetDateTime(&rtc_date);
	}
	// if no rtc chip

	update_timers();
	//PCF_systohc();
	return time_since_1970 + system_timer / 1000;
}




// source -- 0: timer server(since 1900)	1: T3000 timesync
void Get_RTC_by_timestamp(U32_T timestamp,UN_Time* rtc,U8_T source)
{
	S8_T	signhour, signmin;
	U8_T	hour, min;
	U8_T	i;
	U16_T temp_YY;
	TimeInfo tt;


	i = 0;
	tt.timestamp = timestamp;

	signhour = timezone / 100;
	signmin = timezone % 100;

	if (signhour < 0)
	{
		hour = -signhour;
		min = -signmin;
		tt.timestamp -= (hour*3600 + min*60);
	}
	else
	{
		hour = signhour;
		min = signmin;
		tt.timestamp += (hour*3600 + min*60);
	}


	if(Daylight_Saving_Time)
	{
		if((rtc_date.day_of_year >= start_day) && (rtc_date.day_of_year <= end_day))
		{
			tt.timestamp += 3600;
		}
	}

	tt.second_remain = tt.timestamp % 86400;
	tt.day_total = tt.timestamp / 86400;
	tt.HH = tt.second_remain / 3600;
	tt.MI_r = tt.second_remain % 3600;
	tt.MI = tt.MI_r / 60;
	tt.SS = tt.MI_r % 60;
	tt.YY = tt.day_total / 365.2425;

	temp_YY = tt.YY;
	if(source == 0)  // time server
		temp_YY += 1900;
	else  // PC
		temp_YY += 1970;
	if((temp_YY % 4) == 0)
	{
		tt.DD_r = tt.day_total-(tt.YY*365)-(tt.YY/4);
		tt.DD_r++;
		if(source == 0)
		{
			tt.DD_r++;
		}
		while(tt.DD_r>0)
		{
			tt.DD = tt.DD_r;
			tt.DD_r -= AddMonth[i];
			i++;
		}
	}
	else
	{
		tt.DD_r = tt.day_total-(tt.YY*365)-(tt.YY/4);
		if(((temp_YY - 1) % 4) == 0)
		{

		}
		else
			tt.DD_r++;
		/*if(tt.DD_r > 365){
			tt.DD_r = 1;
			tt.YY++;
		}*/
		while(tt.DD_r > 0)
		{
			tt.DD = tt.DD_r;
			tt.DD_r -= Month[i];
			i++;
		}
	}
	tt.MM = i;
	if(source == 0)  // time server
		tt.YY += 1900;
	else  // PC
		tt.YY += 1970;


	rtc->Clk.sec = tt.SS;
	rtc->Clk.min = tt.MI;
	rtc->Clk.hour = tt.HH;
	rtc->Clk.day = tt.DD;
//	Rtc.Clk.week = tt.SS;
	rtc->Clk.mon = tt.MM;
	rtc->Clk.year = tt.YY - 2000;
//	rtc->Clk.day_of_year = tt->day_total;
	rtc->Clk.is_dst = Daylight_Saving_Time;


//#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI)
//	Rtc_Set(rtc->Clk.year,rtc->Clk.mon,rtc->Clk.day,rtc->Clk.hour,rtc->Clk.min,rtc->Clk.sec,0);
//#endif

//#if (ASIX_MINI || ASIX_CM5)
//		flag_Updata_Clock = 1;
//#endif
}


extern uint16_t 	start_day;
extern uint16_t		end_day;
void Calculate_DSL_Time(void)
{
	// calculate stat_time and end_time of DSL
	uint8_t loop;
	start_day = 0;
	end_day = 0;
	for ( loop = 0;loop < Modbus.start_month - 1; loop++)
	{
		start_day += mon_table[loop];
	}
	for ( loop = 0;loop < Modbus.end_month - 1; loop++)
	{
		end_day += mon_table[loop];
	}
	start_day += Modbus.start_day;
	end_day += Modbus.end_day;
	if(Is_Leap_Year(rtc_date.year + 2000))
	{
		start_day++;
		end_day++;
	}

}



void Sync_timestamp(S16_T newTZ,S16_T oldTZ,S8_T newDLS,S8_T oldDLS)
{
	U32_T current;
	UN_Time rtc;

	current = get_current_time();
	current += (newTZ - oldTZ) * 36;
	if((rtc_date.day_of_year >= start_day) && (rtc_date.day_of_year <= end_day))
		current += (newDLS - oldDLS) * 3600;
	rtc.Clk.year = rtc_date.year;
	rtc.Clk.mon = rtc_date.month;
	rtc.Clk.day = rtc_date.day;
	rtc.Clk.hour = rtc_date.hour;
	rtc.Clk.min = rtc_date.minute;
	rtc.Clk.sec = rtc_date.second;
	rtc.Clk.day_of_year = rtc_date.day_of_year;


	Get_RTC_by_timestamp(current,&rtc,1);

	Rtc_Set(rtc_date.year,rtc_date.month,rtc_date.day,rtc_date.hour,rtc_date.minute,rtc_date.second,0);

}


#ifdef main

#include "PCF8563.h"

void iic_set_up()
{
    i2c_config_t conf = {0};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = 21;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = 22;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000;
    esp_err_t ret = i2c_param_config(I2C_NUM_0, &conf);
    assert(ret == ESP_OK);
    ret = i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
    assert(ret == ESP_OK);
}

static const char *LOG_TAG = "PCF8563";

void app_main(void)
{
    iic_set_up();

    while (true) {

        int ret = PCF_hctosys();
        if (ret != 0) {

            PCF_systohc();

            ESP_LOGE(LOG_TAG, "Error reading hardware clock: %d", ret);
        }

        printf("ret %d time %ld\n ", ret, time(NULL));

        vTaskDelay(5000 / portTICK_RATE_MS);

    }

    return;
}
#endif
