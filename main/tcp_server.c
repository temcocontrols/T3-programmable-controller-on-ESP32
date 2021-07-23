/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "esp_ota_ops.h"
//#include "nvs_flash.h"
#include "tcpip_adapter.h"
//#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <store.h>


#include "deviceparams.h"
#include "modbus.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "wifi.h"
#include "ethernet_task.h"
#include "flash.h"

#include "i2c_task.h"
#include "microphone.h"
//#include "pyq1548.h"
#include "led_pwm.h"
#include "ble_mesh.h"
#include "co2_cal.h"
#include "scd4x_i2c.h"
//#include "ud_str.h"
//#include "controls.h"

#define PORT CONFIG_EXAMPLE_PORT

//static const char *TAG = "Example";
static const char *TCP_TASK_TAG = "TCP_TASK";
static const char *UDP_TASK_TAG = "UDP_TASK";

STR_SCAN_CMD Scan_Infor;
//uint8_t tcp_send_packet[1024];
uint8_t modbus_wifi_buf[500];
uint16_t modbus_wifi_len;
uint8_t reg_num;
static bool isSocketCreated = false;
extern double ambient;
extern double object;
extern float mlx90614_ambient;
extern float mlx90614_object;

void start_fw_update(void)
{
	const esp_partition_t *factory = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
	esp_ota_set_boot_partition(factory);
	esp_restart();
}

uint16_t read_user_data_by_block(uint16_t addr)
{
	uint8_t index,item=0;
	uint16_t *block=NULL;
	/*if( addr >= MODBUS_OUTPUT_BLOCK_FIRST && addr <= MODBUS_OUTPUT_BLOCK_LAST )
	{
		index = (addr - MODBUS_OUTPUT_BLOCK_FIRST) / ( (sizeof(Str_out_point) + 1) / 2);
		block = (uint16_t *)&outputs[index];
		item = (addr - MODBUS_OUTPUT_BLOCK_FIRST) % ((sizeof(Str_out_point) + 1) / 2);
	}
	else */if( addr >= MODBUS_INPUT_BLOCK_FIRST && addr <= MODBUS_INPUT_BLOCK_LAST )
	{
		index = (addr - MODBUS_INPUT_BLOCK_FIRST) / ((sizeof(Str_in_point) + 1) / 2);
		block = (uint16_t *)&inputs[index];
		item = (addr - MODBUS_INPUT_BLOCK_FIRST) % ((sizeof(Str_in_point) + 1) / 2);
	}
	return block[item];
}

uint16_t read_wifi_data_by_block(uint16_t addr)
{
	uint8_t item;
	uint16_t *block;
	uint8_t *block1;
	uint8_t temp;
	if(addr == MODBUS_WIFI_SSID_MANUAL_EN)
	{
		return SSID_Info.MANUEL_EN;
	}
	else if(addr == MODBUS_WIFI_MODE)
	{
		return SSID_Info.IP_Auto_Manual;
	}
	else if(addr == MODBUS_WIFI_STATUS)
	{
		return SSID_Info.IP_Wifi_Status;
	}
	else if(addr == MODBUS_WIFI_MODBUS_PORT)
	{
		return SSID_Info.modbus_port;
	}
	else if(addr == MODBUS_WIFI_BACNET_PORT)
	{
		return SSID_Info.bacnet_port;
	}
	else if(addr == MODBUS_WIFI_REV)
	{
		return SSID_Info.rev;
	}
	else if(addr == MODBUS_WIFI_WRITE_MAC)
	{
		//temp = read_eeprom(EEP_WRITE_WIFI_MAC);
		temp = 0x55;
		return temp;
	}

	else if(addr >= MODBUS_WIFI_SSID_START && addr <= MODBUS_WIFI_SSID_END)
	{
		block = (uint16_t *)&SSID_Info.name;
		item = (addr - MODBUS_WIFI_SSID_START) % 32;  // size is 64
		return block[item];
	}
	else if(addr >= MODBUS_WIFI_PASS_START && addr <= MODBUS_WIFI_PASS_END)
	{
		block = (uint16_t *)&SSID_Info.password;
		item = (addr - MODBUS_WIFI_PASS_START) % 16;  // size is 32
		return block[item];
	}
	else if((addr >= MODBUS_WIFI_IP1) && (addr <= MODBUS_WIFI_IP1 + 3))
	{
		block1 = (uint8_t *)&SSID_Info.ip_addr;
		item = (addr - MODBUS_WIFI_IP1) % 4;
		return block1[item];
	}
	else if((addr >= MODBUS_WIFI_NETMASK) && (addr <= MODBUS_WIFI_NETMASK + 3))
	{
		block1 = (uint8_t *)&SSID_Info.net_mask;
		item = (addr - MODBUS_WIFI_NETMASK) % 4;
		return block1[item];
	}
	else if((addr >= MODBUS_WIFI_GETWAY) && (addr <= MODBUS_WIFI_GETWAY + 3))
	{
		block1 = (uint8_t *)&SSID_Info.getway;
		item = (addr - MODBUS_WIFI_GETWAY) % 4;
		return block1[item];
	}
	else if((addr >= MDOBUS_WIFI_MACADDR) && (addr <= MDOBUS_WIFI_MACADDR + 5))
	{
		block1 = (uint8_t *)&SSID_Info.mac_addr;
		item = (addr - MDOBUS_WIFI_MACADDR) % 6;
		return block1[item];
	}
	else
		return 0;

}


static void write_wifi_data_by_block(uint16_t StartAdd,uint8_t HeadLen,uint8_t *pData,uint8_t type)
{
	//uint8_t i,j;
	if(StartAdd == MODBUS_WIFI_SSID_MANUAL_EN)
	{
		SSID_Info.IP_Wifi_Status = WIFI_NO_CONNECT;
		SSID_Info.MANUEL_EN = pData[HeadLen + 5];
		//write_page_en[WIFI_TYPE] = 1;
		//Flash_Write_Mass();
		save_wifi_info();
		//re_init_wifi = true;
		esp_restart();
	}
	else if(StartAdd == MODBUS_WIFI_RESTORE)
	{
		//if(pData[HeadLen + 5] == 1)
		//	Restore_WIFI();
	}
	else if(StartAdd == MODBUS_WIFI_MODE)
	{
		SSID_Info.IP_Auto_Manual = pData[HeadLen + 5];
//		write_eeprom(WIFI_IP_AM,pData[HeadLen + 5]);
	}
	else if(StartAdd == MODBUS_WIFI_BACNET_PORT)
	{
		SSID_Info.bacnet_port = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}
	else if(StartAdd == MODBUS_WIFI_MODBUS_PORT)
	{
		SSID_Info.modbus_port = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
//		write_eeprom(WIFI_MODBUS_PORT,SSID_Info.modbus_port);
//		write_eeprom(WIFI_MODBUS_PORT + 1,SSID_Info.modbus_port >> 8);
	}
	else if(StartAdd == MODBUS_WIIF_START_SMART)
	{
		// write 1, start smart cofigure
		//if(pData[HeadLen + 5] == 1)
		//	Start_Smart_Config();
		//else
		//	Stop_Smart_Config();
	}
	else if(StartAdd == MODBUS_WIFI_WRITE_MAC)
	{
//		if(pData[HeadLen + 5] == 0)
//			write_eeprom(EEP_WRITE_WIFI_MAC,0);
	}
	else if(StartAdd >= MODBUS_WIFI_SSID_START && StartAdd <= MODBUS_WIFI_SSID_END)
	{
		if((StartAdd - MODBUS_WIFI_SSID_START) % 32 == 0)
		{
			memset(&SSID_Info.name,'\0',64);
			memcpy(&SSID_Info.name,&pData[HeadLen + 7],64);
		}
		//save_wifi_info();
	}
	else if(StartAdd >= MODBUS_WIFI_PASS_START && StartAdd <= MODBUS_WIFI_PASS_END)
	{
		if((StartAdd - MODBUS_WIFI_PASS_START) % 16 == 0)
		{
			memset(&SSID_Info.password,'\0',32);
			memcpy(&SSID_Info.password,&pData[HeadLen + 7],32);
		}
		//save_wifi_info();
	}
	else if(StartAdd == MODBUS_WIFI_IP1)
	{
		if((StartAdd - MODBUS_WIFI_IP1) % 12 == 0)
		{
			SSID_Info.ip_addr[0] = pData[HeadLen + 8];
			SSID_Info.ip_addr[1] = pData[HeadLen + 10];
			SSID_Info.ip_addr[2] = pData[HeadLen + 12];
			SSID_Info.ip_addr[3] = pData[HeadLen + 14];

			SSID_Info.net_mask[0] = pData[HeadLen + 16];
			SSID_Info.net_mask[1] = pData[HeadLen + 18];
			SSID_Info.net_mask[2] = pData[HeadLen + 20];
			SSID_Info.net_mask[3] = pData[HeadLen + 22];

			SSID_Info.getway[0] = pData[HeadLen + 24];
			SSID_Info.getway[1] = pData[HeadLen + 26];
			SSID_Info.getway[2] = pData[HeadLen + 28];
			SSID_Info.getway[3] = pData[HeadLen + 30];

		}
	}
	else if(StartAdd == MDOBUS_WIFI_MACADDR)
	{
		if((StartAdd - MDOBUS_WIFI_MACADDR) % 6 == 0)
		{
			SSID_Info.mac_addr[0] = pData[HeadLen + 8];
			SSID_Info.mac_addr[1] = pData[HeadLen + 10];
			SSID_Info.mac_addr[2] = pData[HeadLen + 12];
			SSID_Info.mac_addr[3] = pData[HeadLen + 14];
			SSID_Info.mac_addr[4] = pData[HeadLen + 16];
			SSID_Info.mac_addr[5] = pData[HeadLen + 18];

			//ESP8266_Set_MAC(SSID_Info.mac_addr);
			//ESP8266_Get_MAC(SSID_Info.mac_addr);
		}
	}

}

static void internalDeal(uint8_t  *bufadd,uint8_t type)
{
	uint16_t address;
	uint16_t  temp_i;
	uint8_t i;

	if(type == SERIAL || type == BAC_TO_MODBUS)  // modbus packet
	{
		//HeadLen = 0;
	}
	else    // TCP packet or wifi
	{
	//	HeadLen = UIP_HEAD;
		bufadd = bufadd + 6;
	}

	address = ((uint16_t)*(bufadd+2) <<8) + *(bufadd+3); //get modbus register number

	if(*(bufadd+1) == MULTIPLE_WRITE_VARIABLES)
	{
		temp_i = (uint16_t)(*(bufadd+2) << 8) + *(bufadd+3);
		if(temp_i >= MODBUS_WIFI_START && temp_i <= MODBUS_WIFI_END)
		{
			write_wifi_data_by_block(temp_i,0,bufadd,0);
		}
		else if(temp_i  >= MODBUS_INPUT_BLOCK_FIRST && temp_i  <= MODBUS_INPUT_BLOCK_LAST)
		{
			if((temp_i - MODBUS_INPUT_BLOCK_FIRST) % ((sizeof(Str_in_point) + 1   ) / 2) == 0)
			{
				i = (temp_i - MODBUS_INPUT_BLOCK_FIRST) / ((sizeof(Str_in_point) + 1) / 2);
				memcpy(&inputs[i],&bufadd[7],sizeof(Str_in_point));
				save_blob_info(FLASH_INPUT_INFO, (const void*)&inputs[0], INPUT_PAGE_LENTH);
			}
		}
		else if(temp_i == SERIALNUMBER_LOWORD )
		{
			holding_reg_params.serial_number_lo = (uint16_t)(*(bufadd+10) << 8) + *(bufadd+8);
			holding_reg_params.serial_number_hi = (uint16_t)(*(bufadd+14) << 8) + *(bufadd+12);//BUILD_UINT16(*(bufadd+12),*(bufadd+14));
			save_uint16_to_flash(FLASH_SERIAL_NUM_LO, holding_reg_params.serial_number_lo);
			save_uint16_to_flash(FLASH_SERIAL_NUM_HI, holding_reg_params.serial_number_hi);
		}
		else if(temp_i  >= TSTAT_NAME1 && temp_i <= TSTAT_NAME10)
		{
			if((*(bufadd +6)) <= 20)
			{
				for(i=0;i<*(bufadd +6);i++)			//	(data_buffer[6]*2)
				{
					holding_reg_params.panelname[i] = *(bufadd + 7+i);
				}
				save_blob_info(FLASH_PRODUCT_NAME,(const void*)holding_reg_params.panelname, 20);
			}
		}
	}
	if(*(bufadd+1) == WRITE_VARIABLES)
	{
		if(address >= MODBUS_WIFI_START && address <= MODBUS_WIFI_END)
		{
			write_wifi_data_by_block(address,0,bufadd,0);
		}
		else if(address == TSTAT_ADDRESS)
		{
			if((*(bufadd+5)!=0)&&(*(bufadd+5)!=0xff))
			{
				holding_reg_params.modbus_address = *(bufadd+5);
				save_uint8_to_flash( FLASH_MODBUS_ID, holding_reg_params.modbus_address);
			}
		}
		else if(address == BAUDRATE)
		{
			if(*(bufadd+5)<5)
			{
				holding_reg_params.baud_rate = *(bufadd+5);
				save_uint8_to_flash( FLASH_BAUD_RATE, holding_reg_params.baud_rate);
				modbus_init();
			}
		}
		else if(address == FAN_MODULE_PWM1)
		{
			holding_reg_params.fan_module_pwm1 = *(bufadd+5);
			if(holding_reg_params.which_project == PROJECT_FAN_MODULE){
				ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, holding_reg_params.fan_module_pwm1);
				ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
			}
		}
		else if(address == FAN_MODULE_PWM2)
		{
			holding_reg_params.fan_module_pwm2 = *(bufadd+5);
			if(holding_reg_params.which_project == PROJECT_FAN_MODULE){
				ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 255-holding_reg_params.fan_module_pwm2);
				ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
			}
		}
		else if(address == MODBUS_SOUND_TRIGGER)
		{
			sound_trigger.trigger = (((uint16_t)*(bufadd+4)<<8) + *(bufadd+5));
			save_uint16_to_flash(FLASH_SOUND_TRIGGER_VALUE, sound_trigger.trigger);
		}
		else if(address == MODBUS_SOUND_TIMER)
		{
			sound_trigger.timer = (((uint16_t)*(bufadd+4)<<8) + *(bufadd+5));
			sound_trigger.count_down = 0;
			save_uint16_to_flash(FLASH_SOUND_TRIGGER_TIMER, sound_trigger.timer);
		}
		else if(address == MODBUS_LIGHT_TRIGGER)
		{
			light_trigger.trigger = (((uint16_t)*(bufadd+4)<<8) + *(bufadd+5));
			save_uint16_to_flash(FLASH_LIGHT_TRIGGER_VALUE, light_trigger.trigger);
		}
		else if(address == MODBUS_LIGHT_TIMER)
		{
			light_trigger.timer = (((uint16_t)*(bufadd+4)<<8) + *(bufadd+5));
			light_trigger.count_down = 0;
			save_uint16_to_flash(FLASH_LIGHT_TRIGGER_TIMER, light_trigger.timer);
		}
		else if(address == MODBUS_CO2_TRIGGER)
		{
			co2_trigger.trigger = (((uint16_t)*(bufadd+4)<<8) + *(bufadd+5));
			save_uint16_to_flash(FLASH_CO2_TRIGGER_VALUE, co2_trigger.trigger);
		}
		else if(address == MODBUS_CO2_TIMER)
		{
			co2_trigger.timer = (((uint16_t)*(bufadd+4)<<8) + *(bufadd+5));
			co2_trigger.count_down = 0;
			save_uint16_to_flash(FLASH_CO2_TRIGGER_TIMER, co2_trigger.timer);
		}
		else if(address == MODBUS_OCC_TRIGGER)
		{
			occ_trigger.trigger = (((uint16_t)*(bufadd+4)<<8) + *(bufadd+5));
			save_uint16_to_flash(FLASH_OCC_TRIGGER_VALUE, occ_trigger.trigger);
		}
		else if(address == MODBUS_OCC_TRIGGER_TIMER)
		{
			occ_trigger.timer = (((uint16_t)*(bufadd+4)<<8) + *(bufadd+5));
			occ_trigger.count_down = 0;
			save_uint16_to_flash(FLASH_OCC_TRIGGER_TIMER, occ_trigger.timer);
		}
		else if(address == MODBUS_SHT31_TEMP_OFFSET)
		{
			holding_reg_params.sht31_temp_offset =  (((int16_t)*(bufadd+4)<<8) + *(bufadd+5));
			save_int16_to_flash(FLASH_SHT31_TEMP_OFFSET, holding_reg_params.sht31_temp_offset);
		}
		else if(address == MODBUS_TEMP_10K_OFFSET)
		{
			holding_reg_params.temp_10k_offset =  (((int16_t)*(bufadd+4)<<8) + *(bufadd+5));
			save_int16_to_flash(FLASH_10K_TEMP_OFFSET, holding_reg_params.temp_10k_offset);
		}
		else if(address == MODBUS_AMBIENT_TEMP_OFFSET)
		{
			holding_reg_params.ambient_temp_offset =  (((int16_t)*(bufadd+4)<<8) + *(bufadd+5));
			save_int16_to_flash(FLASH_AMBIENT_TEMP_OFFSET, holding_reg_params.ambient_temp_offset);
		}
		else if(address == MODBUS_OBJECT_TEMP_OFFSET)
		{
			holding_reg_params.object_temp_offset =  (((int16_t)*(bufadd+4)<<8) + *(bufadd+5));
			save_int16_to_flash(FLASH_OBJECT_TEMP_OFFSET, holding_reg_params.object_temp_offset);
		}
		else if(address == MODBUS_OUTPUT_BLOCK_FIRST)
		{
			save_uint16_to_flash(FLASH_INPUT_FLAG, 0);
		}
		//CO2 background calibration
		else if(address ==  MODBUS_CO2_BKCAL_ONOFF)
		{
				if((*(bufadd+5) == 0)||(*(bufadd+5)==1))
				{
					co2_bkcal_onoff = *(bufadd+5);
					save_uint8_to_flash(FLASH_CO2_BKCAL_ONOFF, co2_bkcal_onoff);
				}
		}
		else if(address == MODBUS_CO2_NATURE_LEVEL)
		{
				uint16_t itemp;
				itemp =((uint16_t)*(bufadd+4)<<8) + *(bufadd+5);
				if((itemp >= 390)&&(itemp<=500))
				{
					co2_level = itemp;
					save_uint16_to_flash(FLASH_CO2_NATURE_LEVEL, co2_level);
				}
		}
		else if(address == MODBUS_CO2_MIN_ADJ)
		{
				if((*(bufadd+5)>=1)&&(*(bufadd+5)<=10))
				{
					min_co2_adj = *(bufadd+5);
					save_uint8_to_flash(FLASH_CO2_MIN_ADJ, min_co2_adj);
				}
		}
		else if(address == MODBUS_CO2_CAL_DAYS)
		{
				if((*(bufadd+5)>=2)&&(*(bufadd+5)<=30))
				{
					co2_bkcal_day = *(bufadd+5);
					save_uint16_to_flash(FLASH_CO2_CAL_DAYS, co2_bkcal_day);
				}
		}
		else if(address == MODBUS_CO2_LOWVALUE_REMAIN_TIME)
		{
				value_keep_time = *(bufadd+5);
				save_uint8_to_flash(FLASH_CO2_LOWVALUE_REMAIN_TIME, value_keep_time);
		}
		else if(address == MODBUS_CO2_BKCAL_VALUE)
		{
				co2_bkcal_value = (int16_t)(((uint16_t)*(bufadd+4)<<8) + *(bufadd+5));
				save_int16_to_flash(FLASH_CO2_BKCAL_VALUE, co2_bkcal_value);

		}
		else if(address == CO2_FRC_VALUE)
		{
			uint16_t co2_frc_temp;
			co2_frc_temp = (((uint16_t)*(bufadd+4)<<8) + *(bufadd+5));
			holding_reg_params.co2_frc = co2_frc_temp;
		    scd4x_stop_periodic_measurement();
			scd4x_perform_forced_recalibration( co2_frc_temp, &holding_reg_params.register100);
			scd4x_start_periodic_measurement();
		}
		else if(address == CO2_ASC_ENABLE)
		{
			if(*(bufadd+5)<= 1)
			{
				holding_reg_params.co2_asc_enable =*(bufadd+5);
				scd4x_stop_periodic_measurement();
				scd4x_set_automatic_self_calibration(holding_reg_params.co2_asc_enable);
				scd4x_start_periodic_measurement();
			}
		}
		else if (address == UPDATE_STATUS)
		{
			if (*(bufadd+5) == 0x7f)
			{
				start_fw_update();
			}
		}
		
	}
}

static void responseData(uint8_t  *bufadd, uint8_t type, uint16_t rece_size)
{
	uint8_t num, i, temp1, temp2;
	uint16_t temp;
	uint16_t send_cout = 0 ;
	uint16_t address;
	uint8_t headlen = 0;
	uint16_t TransID;
	uint8_t *uart_sendB,*uart_send;
	//char test;

	if(type == WIFI)
	{
		headlen = UIP_HEAD;
		uart_sendB = malloc(512);
	}
	else{
		headlen = 0;
		uart_send = malloc(512);
		//uart_write_bytes(UART_NUM_0, "responseDataSIZE=", sizeof("responseDataSIZE=")-1);
		//test = (char)rece_size+0x30;
		//uart_write_bytes(UART_NUM_0, &test, 1);
	}
	if(*(bufadd + 1 + headlen) == WRITE_VARIABLES){
		if(type == WIFI){ // for wifi
			for(i = 0; i < 6; i++)
				uart_sendB[UIP_HEAD + send_cout++] = *(bufadd + i + headlen);

			uart_sendB[0] = *bufadd;//0;			//	TransID
			uart_sendB[1] = *(bufadd + 1);//TransID++;
			uart_sendB[2] = 0;			//	ProtoID
			uart_sendB[3] = 0;
			uart_sendB[4] = 0;	//	Len
			uart_sendB[5] = 6;

			memcpy(modbus_wifi_buf,uart_sendB,UIP_HEAD + send_cout);
			modbus_wifi_len = UIP_HEAD + send_cout;
		}else{
			//uart_write_bytes(UART_NUM_0, "WRITE_VARIABLES\r\n", sizeof("WRITE_VARIABLES\r\n"));
			for(i = 0; i < rece_size; i++)
				uart_send[send_cout++] = *(bufadd+i);
			if(type == BAC_TO_MODBUS){

			}
			else
				uart_write_bytes(UART_NUM_0, (const char *)uart_send, send_cout);
			return;
		}
	}else if(*(bufadd + 1 + headlen) == MULTIPLE_WRITE){
		//init_crc16();
		for(i = 0; i < 6; i++)
		{
			 if(type != WIFI)
					uart_send[send_cout++] = *(bufadd+i) ;
			 else//zigbee
			 {
					uart_sendB[UIP_HEAD + send_cout++] = *(bufadd + i + headlen);

			 }
			crc16_byte(*(bufadd+i));
		}

		if(type != WIFI){
			if(type == BAC_TO_MODBUS){
			//	memcpy(&bacnet_to_modbus,&uart_send[3],reg_num*2);
			}
			else{
				uart_send[6] = CRChi;
				uart_send[7] = CRClo;
				uart_write_bytes(UART_NUM_0, (const char *)uart_send, 8);
				return;
			}
		}else{
			uart_sendB[0] = *bufadd;//0;			//	TransID
			uart_sendB[1] = *(bufadd + 1);//TransID++;
			uart_sendB[2] = 0;			//	ProtoID
			uart_sendB[3] = 0;
			uart_sendB[4] = 0;	//	Len
			uart_sendB[5] = 6;
			memcpy(modbus_wifi_buf,uart_sendB,UIP_HEAD + send_cout);
			modbus_wifi_len = UIP_HEAD + send_cout;
		}
	}else if(*(bufadd + 1 + headlen) == READ_VARIABLES){
		num = *(bufadd+5 + headlen);
		if(type!=WIFI)
		{
			uart_send[send_cout++] = *(bufadd) ;
			uart_send[send_cout++] = *(bufadd+1) ;
			uart_send[send_cout++] = (*(bufadd+5)<<1);
		}
		else//WIFI
		{
			uart_sendB[UIP_HEAD + send_cout++] = *(bufadd + headlen);
			uart_sendB[UIP_HEAD + send_cout++] = *(bufadd+1 + headlen) ;
			uart_sendB[UIP_HEAD + send_cout++] = (*(bufadd +5 + headlen)<<1);
		}
		crc16_byte(*(bufadd + headlen));
		crc16_byte(*(bufadd +1 + headlen));
		crc16_byte((*(bufadd + 5 + headlen)<<1));
		for(i = 0; i < num; i++)
		{
			address = (uint16_t)((*(bufadd+2 + headlen))<<8) + (*(bufadd+3 + headlen)) + i;
			if(address == SERIALNUMBER_LOWORD )
			{
				temp1 = 0 ;
				temp2 = (uint8_t)(holding_reg_params.serial_number_lo);//BREAK_UINT32(holding_reg_params.serial_number_lo,0);
			}
			else if(address == (SERIALNUMBER_LOWORD +1))
			{
				temp1 = 0 ;
				temp2 = (uint8_t)(holding_reg_params.serial_number_lo>>8);
			}
			else if(address == SERIALNUMBER_HIWORD)
			{
				temp1 = 0 ;
				temp2 = LO_UINT16(holding_reg_params.serial_number_hi);
			}
			else if(address == (SERIALNUMBER_HIWORD +1))
			{
				temp1 = 0 ;
				temp2 = HI_UINT16(holding_reg_params.serial_number_hi);
			}
			else if(address == VERSION_NUMBER_LO)
			{
				temp1 = 0;
				temp2 = holding_reg_params.version_number_lo & 0xff;
			}

			else if(address == VERSION_NUMBER_HI)
			{
				temp1 = 0;//(EEPROM_VERSION >> 8) & 0xff;
				temp2 = 0;//FirmwareVersion_HI;
			}
			else if(address == TSTAT_ADDRESS)
			{
				temp1 = 0; ;
				temp2 = holding_reg_params.modbus_address & 0xff;
			}
			else if(address == PRODUCT_MODEL)
			{
				temp1 = 0 ;
				temp2 = holding_reg_params.product_model&0xff;
			}
			else if(address == HARDWARE_REV)
			{
				temp1 = 0 ;
				temp2 = holding_reg_params.hardware_version ;
			}
			else if(address == BAUDRATE)
			{
				temp1 = 0;
				temp2 = holding_reg_params.baud_rate;
			}
			else if(address == MODBUS_ETHERNET_STATUS)
			{
				temp1 = 0;
				temp2 = holding_reg_params.ethernet_status;
			}
			else if((address>=MODBUS_TEST_BUF_1)&&(address<MODBUS_TEST_BUF_20))
			{
				temp1 = (holding_reg_params.testBuf[address-MODBUS_TEST_BUF_1]>>8)&0xff;
				temp2 = holding_reg_params.testBuf[address-MODBUS_TEST_BUF_1]&0xff;
			}
			else if((address >= MAC_ADDR_1) && (address <= MAC_ADDR_6))
			{
				temp1 = 0;
				temp2 = holding_reg_params.mac_addr[address - MAC_ADDR_1];
			}
			else if((address >= IP_ADDR_1) && (address <= IP_ADDR_4))
			{
				temp1 = 0;
				temp2 = holding_reg_params.ip_addr[address - IP_ADDR_1];
			}
			else if((address >= IP_SUB_MASK_1)&& (address <= IP_SUB_MASK_4))
			{
				temp1 = 0;
				temp2 = holding_reg_params.ip_net_mask[address - IP_SUB_MASK_1];
			}

			else if((address >= IP_GATE_WAY_1)&& (address <= IP_GATE_WAY_4))
			{
				temp1 = 0;
				temp2 = holding_reg_params.ip_gateway[address - IP_GATE_WAY_1];
			}
			else if(address == WIFI_RSSI)
			{
				temp1 = 0xff;
				temp2 = SSID_Info.rssi;
			}
			else if(address == FAN_MODULE_PWM1)
			{
				temp1 = 0;
				temp2 = holding_reg_params.fan_module_pwm1;
			}
			else if(address == FAN_MODULE_PWM2)
			{
				temp1 = 0;
				temp2 = holding_reg_params.fan_module_pwm2;
			}
			else if(address == FAN_MODULE_PULSE)
			{
				temp = holding_reg_params.fan_module_pulse*6;
				//temp1 = ((uint8_t)(holding_reg_params.fan_module_pulse) >> 8) & 0xFF;;
				//temp2 = (uint8_t)(holding_reg_params.fan_module_pulse) & 0xFF;
				temp1 = (temp >> 8) & 0xFF;
				temp2 = temp & 0xFF;
			}
			else if(address == FAN_MODULE_INPUT_VOLTAGE)
			{
				temp = holding_reg_params.fan_module_input_voltage;
				temp1 = (temp >> 8) & 0xFF;
				temp2 = temp & 0xFF;
			}
			else if(address == FAN_MODULE_10K_TEMP)
			{
				temp = holding_reg_params.fan_module_10k_temp;
				temp1 = (temp >> 8) & 0xFF;
				temp2 = temp & 0xFF;
			}
			else if(address == MODBUS_SHT31_TEMP_OFFSET)
			{
				temp = holding_reg_params.sht31_temp_offset;
				temp1 = (temp >> 8) & 0xFF;
				temp2 = temp & 0xFF;
			}
			else if(address == MODBUS_TEMP_10K_OFFSET)
			{
				temp = holding_reg_params.temp_10k_offset;
				temp1 = (temp >> 8) & 0xFF;
				temp2 = temp & 0xFF;
			}
			else if(address == MODBUS_AMBIENT_TEMP_OFFSET)
			{
				temp = holding_reg_params.ambient_temp_offset;
				temp1 = (temp >> 8) & 0xFF;
				temp2 = temp & 0xFF;
			}
			else if(address == MODBUS_OBJECT_TEMP_OFFSET)
			{
				temp = holding_reg_params.object_temp_offset;
				temp1 = (temp >> 8) & 0xFF;
				temp2 = temp & 0xFF;
			}
			else if(address == TEMPRATURE_CHIP)
			{
				temp = g_sensors.temperature;
				temp1 = (temp >> 8) & 0xFF;
				temp2 = temp & 0xFF;
			}
			else if(address == ANALOG_INPUT8_VALUE)
			{
				temp = g_sensors.voc_value;
				temp1 = (temp >> 8) & 0xFF;
				temp2 = temp & 0xFF;
			}
			else if(address == EXTERNAL_SENSOR1)
			{
				//temp = g_sensors.co2;
				temp1 = (g_sensors.co2 >> 8) & 0xFF;
				temp2 =(uint8_t) g_sensors.co2 & 0xFF;
			}
			else if(address == EXTERNAL_SENSOR2)
			{
				temp = g_sensors.humidity;
				temp1 = (temp >> 8) & 0xFF;
				temp2 = temp & 0xFF;
			}
			else if( address == (EXTERNAL_SENSOR2+3))
			{
				//temp = 123;//g_sensors.infrared_temp1;
				if(holding_reg_params.which_project == PROJECT_FAN_MODULE)
				{
					temp1 = ((uint8_t)(mlx90614_ambient*10) >> 8) & 0xFF;;
					temp2 = (uint8_t)(mlx90614_ambient*10) & 0xFF;
				}
				else
				{
					temp1 = ((uint8_t)(ambient*10) >> 8) & 0xFF;;
					temp2 = (uint8_t)(ambient*10) & 0xFF;
				}
			}
			else if( address == (EXTERNAL_SENSOR2+4))
			{
				//temp = 123;//g_sensors.infrared_temp1;
				if(holding_reg_params.which_project == PROJECT_FAN_MODULE)
				{
					temp1 = ((uint8_t)(mlx90614_object*10) >> 8) & 0xFF;;
					temp2 = (uint8_t)(mlx90614_object*10) & 0xFF;
				}
				else
				{
					temp1 = ((uint8_t)(object*10) >> 8) & 0xFF;;
					temp2 = (uint8_t)(object*10) & 0xFF;
				}
			}
			else if( address == COOLHEATMODE)//(EXTERNAL_SENSOR2+3))
			{
				//temp = 123;//g_sensors.infrared_temp1;
				temp1 = (g_sensors.ambient >> 8) & 0xFF;;
				temp2 = (uint8_t)g_sensors.ambient & 0xFF;
			}
			else if( address == PID1_MODE_OPERATION)//(EXTERNAL_SENSOR2+4))
			{
				//temp = 123;//g_sensors.infrared_temp1;
				temp1 = (g_sensors.object >> 8) & 0xFF;;
				temp2 = (uint8_t)g_sensors.object & 0xFF;
			}
			else if(address == VOC_DATA)
			{
				//temp = g_sensors.voc_value;
				temp1 = (g_sensors.voc_value >> 8) & 0xFF;
				temp2 = (uint8_t)g_sensors.voc_value & 0xFF;
			}
			else if(address == PIR_SENSOR_VALUE)
			{
				temp1 = 0;
				temp2 = g_sensors.occ & 0xFF;
			}
			else if(address == MIC_SOUND)
			{
				temp1 = (uint8_t) (g_sensors.sound >> 8) & 0xFF;
				temp2 = (uint8_t)g_sensors.sound & 0xFF;
			}
			else if(address >= MODBUS_WIFI_START &&  address <= MODBUS_WIFI_END)
			{
				temp = read_wifi_data_by_block(address);

				temp1 = (temp >> 8) & 0xFF;
				temp2 = temp & 0xFF;
			}
			else if(address == LIGHT_SENSOR)
			{
				temp = g_sensors.light_value;
				temp1 = (temp >> 8) & 0xFF;
				temp2 = temp & 0xFF;
			}
			else if(address == MODBUS_SOUND_TRIGGER)
			{
				temp1 = (uint8_t)(sound_trigger.trigger>>8)&0xff;
				temp2 = (uint8_t)sound_trigger.trigger;
			}
			else if(address == MODBUS_SOUND_TIMER)
			{
				temp1 = (uint8_t)(sound_trigger.timer>>8)&0xff;
				temp2 = (uint8_t)sound_trigger.timer;
			}
			else if(address == MODBUS_SOUND_ALARM_ON)
			{
				temp1 = (uint8_t)(sound_trigger.alarmOn>>8)&0xff;
				temp2 = (uint8_t)sound_trigger.alarmOn;
			}
			else if(address == MODBUS_SOUND_COUNT_DOWN)
			{
				temp1 = (uint8_t)(sound_trigger.count_down>>8)&0xff;
				temp2 = (uint8_t)sound_trigger.count_down;
			}
			else if(address == MODBUS_LIGHT_TRIGGER)
			{
				temp1 = (uint8_t)(light_trigger.trigger>>8)&0xff;
				temp2 = (uint8_t)light_trigger.trigger;
			}
			else if(address == MODBUS_LIGHT_TIMER)
			{
				temp1 = (uint8_t)(light_trigger.timer>>8)&0xff;
				temp2 = (uint8_t)light_trigger.timer;
			}
			else if(address == MODBUS_LIGHT_ALARM_ON)
			{
				temp1 = (uint8_t)(light_trigger.alarmOn>>8)&0xff;
				temp2 = (uint8_t)light_trigger.alarmOn;
			}
			else if(address == MODBUS_LIGHT_COUNT_DOWN)
			{
				temp1 = (uint8_t)(light_trigger.count_down>>8)&0xff;
				temp2 = (uint8_t)light_trigger.count_down;
			}
			else if(address == MODBUS_CO2_TRIGGER)
			{
				temp1 = (uint8_t)(co2_trigger.trigger>>8)&0xff;
				temp2 = (uint8_t)co2_trigger.trigger;
			}
			else if(address == MODBUS_CO2_TIMER)
			{
				temp1 = (uint8_t)(co2_trigger.timer>>8)&0xff;
				temp2 = (uint8_t)co2_trigger.timer;
			}
			else if(address == MODBUS_CO2_ALARM_ON)
			{
				temp1 = (uint8_t)(co2_trigger.alarmOn>>8)&0xff;
				temp2 = (uint8_t)co2_trigger.alarmOn;
			}
			else if(address == MODBUS_CO2_COUNT_DOWN)
			{
				temp1 = (uint8_t)(co2_trigger.count_down>>8)&0xff;
				temp2 = (uint8_t)co2_trigger.count_down;
			}
			else if(address == MODBUS_OCC_TRIGGER)
			{
				temp1 = (uint8_t)(occ_trigger.trigger>>8)&0xff;
				temp2 = (uint8_t)occ_trigger.trigger;
			}
			else if(address == MODBUS_OCC_TRIGGER_TIMER)
			{
				temp1 = (uint8_t)(occ_trigger.timer>>8)&0xff;
				temp2 = (uint8_t)occ_trigger.timer;
			}
			else if(address == MODBUS_OCC_ALARM_ON)
			{
				temp1 = (uint8_t)(occ_trigger.alarmOn>>8)&0xff;
				temp2 = (uint8_t)occ_trigger.alarmOn;
			}
			else if(address == MODBUS_OCC_COUNT_DOWN)
			{
				temp1 = (uint8_t)(occ_trigger.count_down>>8)&0xff;
				temp2 = (uint8_t)occ_trigger.count_down;
			}
			//CO2 background calibration
			else if(address == MODBUS_CO2_BKCAL_ONOFF)
			{
				temp1 = 0;
				temp2 = co2_bkcal_onoff;
			}
			else if(address == MODBUS_CO2_NATURE_LEVEL)
			{
				temp1 = (uint8_t)(co2_level>>8)&0xff;;
				temp2 = (uint8_t)co2_level;
			}
			else if(address == MODBUS_CO2_MIN_ADJ)
			{
				temp1 = 0;
				temp2 = min_co2_adj;
			}
			else if(address == MODBUS_CO2_CAL_DAYS)
			{
				temp1 = 0;
				temp2 = co2_bkcal_day;
			}
			else if(address == MODBUS_CO2_LOWVALUE_REMAIN_TIME)
			{
				temp1 = 0;
				temp2 = value_keep_time;
			}
			else if(address == MODBUS_CO2_BKCAL_VALUE)
			{
				temp1 = (uint8_t)(co2_bkcal_value>>8)&0xff;
				temp2 = (uint8_t)co2_bkcal_value;
			}
			else if(address == MODBUS_CO2_LOWVALUE)
			{
				temp1 = (uint8_t)(co2_lowest_value>>8)&0xff;
				temp2 = (uint8_t)co2_lowest_value;
			}
			else if(address == CO2_FRC_VALUE)
			{
				temp1 = (uint8_t)(holding_reg_params.co2_frc>>8)&0xff;
				temp2 = (uint8_t)holding_reg_params.co2_frc;
			}
			else if(address == CO2_ASC_VALUE)
			{
				temp1 = (uint8_t)(holding_reg_params.co2_asc_enable>>8)&0xff;
				temp2 = (uint8_t)holding_reg_params.co2_asc_enable;
			}
			else if(address == TSTAT_NAME_ENABLE)
			{
				temp1 = 0;
				temp2 = 0x56;
			}
			else if((address >= TSTAT_NAME1) && (address <= TSTAT_NAME10))
			{
				uint16_t temp_modbus_name = address - TSTAT_NAME1;
				//temp1= Scan_Infor.panelname[temp_modbus_name * 2];
				//temp2= Scan_Infor.panelname[temp_modbus_name * 2 + 1];
				temp1= holding_reg_params.panelname[temp_modbus_name * 2];
				temp2= holding_reg_params.panelname[temp_modbus_name * 2 + 1];
			}
			/*else if((address>= MODBUS_OUTPUT_BLOCK_FIRST)&&(address<=MODBUS_INPUT_BLOCK_LAST))
			{
				temp = read_user_data_by_block(address);

				temp1 = (temp >> 8) & 0xFF;
				temp2 = temp & 0xFF;
			}*/
			else if((address >= MODBUS_INPUT_BLOCK_FIRST)&&(address<= MODBUS_INPUT_BLOCK_LAST))
			{
				uint8_t index,item;
				uint16_t *block;
				index = (address-MODBUS_INPUT_BLOCK_FIRST)/((sizeof(Str_in_point)+1)/2);
				block = (uint16_t *)&inputs[index];
				item = (address-MODBUS_INPUT_BLOCK_FIRST)%((sizeof(Str_in_point)+1)/2);
				temp1 = (block[item]>>8)&0xff;
				temp2 = block[item]&0xff;
			}
			else if(address == MODBUS_EX_MOUDLE_EN)
			{
				temp1 = 0;
				temp2 = 0x55;
			}
			else if(address == MODBUS_EX_MOUDLE_FLAG12)
			{
				if(holding_reg_params.which_project == PROJECT_FAN_MODULE)
				{
					temp1 = 0x10;
					temp2 = 0xc7;
				}
				else if(holding_reg_params.which_project == PROJECT_SAUTER)
				{
					temp1 = 0x13;
					temp2 = 0xff;
				}
				else
				{
					temp1 = 0;
					temp2 = 0;
				}
			}
			else
			{
				temp1 = 0 ;
				temp2 = 0;
			}
			if(type!=WIFI)//
			{
				uart_send[send_cout++] = temp1 ;
				uart_send[send_cout++] = temp2 ;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else
			{
				uart_sendB[UIP_HEAD + send_cout++] = temp1 ;
				uart_sendB[UIP_HEAD + send_cout++] = temp2 ;
				TransID =  (uint16_t)(*(bufadd) << 8) | (*(bufadd + 1));
				uart_sendB[0] = TransID >> 8;			//	TransID
				uart_sendB[1] = TransID;
				uart_sendB[2] = 0;			//	ProtoID
				uart_sendB[3] = 0;
				uart_sendB[4] = (3 + num * 2) >> 8;	//	Len
				uart_sendB[5] = (uint8_t)(3 + num * 2) ;
			}
		}
	}else if(*(bufadd+1 + headlen) == CHECKONLINE){
		if(type!=WIFI){
			uart_send[send_cout++] = 0xff;
			uart_send[send_cout++] = 0x19;
			uart_send[send_cout++] = holding_reg_params.modbus_address;
			uart_send[send_cout++] = LO_UINT16(holding_reg_params.serial_number_lo);
			uart_send[send_cout++] = HI_UINT16(holding_reg_params.serial_number_lo);
			uart_send[send_cout++] = LO_UINT16(holding_reg_params.serial_number_hi);
			uart_send[send_cout++] = HI_UINT16(holding_reg_params.serial_number_hi);
			for(i=0;i<send_cout;i++)
				crc16_byte(uart_send[i]);
		}
	}else
		return;

	temp1 = CRChi ;
	temp2 = CRClo;
	if(type == WIFI)
	{
		memcpy(modbus_wifi_buf,uart_sendB,UIP_HEAD + send_cout);
		modbus_wifi_len = UIP_HEAD + send_cout;
		free(uart_sendB);
	}else{
		uart_send[send_cout++] = temp1 ;
		uart_send[send_cout++] = temp2 ;
		holding_reg_params.led_rx485_tx = 2;
		holding_reg_params.stm32_uart_send[1] = 1;
		uart_write_bytes(UART_NUM_0, (const char *)uart_send, send_cout);

		free(uart_send);
	}
}



void responseCmd(uint8_t type, uint8_t *pData, uint16_t len)
{
	if(type == WIFI)
	{
		reg_num = pData[4]*256 + pData[5];
		responseData(pData,WIFI, len);
		internalDeal(pData,WIFI);
	}
	else if(type == BAC_TO_MODBUS)
	{

	}
	else
	{
		reg_num = pData[4]*256 + pData[5];
		//uart_write_bytes(UART_NUM_0, "responseCmd\r\n", sizeof("responseCmd\r\n"));
		responseData(pData,SERIAL,len);
		internalDeal(pData,SERIAL);
	}
}

void UdpData(unsigned char type)
{
	// header 2 bytes
	memset(&Scan_Infor,0,sizeof(STR_SCAN_CMD));
	if(type == 0)
		Scan_Infor.cmd = 0x0065;
	else if(type == 1)
		Scan_Infor.cmd = 0x0067;

	Scan_Infor.len = 0x001d;

	//serialnumber 4 bytes
	Scan_Infor.own_sn[0] = LO_UINT16(holding_reg_params.serial_number_lo);//BREAK_UINT32(holding_reg_params.serial_number_hi,1);//0;//SerialNumber(0);//(unsigned short int)Modbus.serialNum[0];
	Scan_Infor.own_sn[1] = HI_UINT16(holding_reg_params.serial_number_lo);//BREAK_UINT32(holding_reg_params.serial_number_hi,0);//1;//SerialNumber(1);//(unsigned short int)Modbus.serialNum[1];
	Scan_Infor.own_sn[2] = LO_UINT16(holding_reg_params.serial_number_hi);//BREAK_UINT32(holding_reg_params.serial_number_lo,1);//35;//SerialNumber(2);//(unsigned short int)Modbus.serialNum[2];
	Scan_Infor.own_sn[3] = HI_UINT16(holding_reg_params.serial_number_hi);//BREAK_UINT32(holding_reg_params.serial_number_lo,0);//255;//SerialNumber(3);//(unsigned short int)Modbus.serialNum[3];

	Scan_Infor.product = holding_reg_params.product_model&0xff;//PM_TSTAT_AQ;//PRODUCT_MINI_ARM;  // only for test now

	//modbus address
	Scan_Infor.address = holding_reg_params.modbus_address;//laddress;//(unsigned short int)Modbus.address;

	//Ip
	if(holding_reg_params.ethernet_status == 2)
	{
		Scan_Infor.ipaddr[0] = holding_reg_params.ip_addr[0];//(unsigned short int)SSID_Info.ip_addr[0];
		Scan_Infor.ipaddr[1] = holding_reg_params.ip_addr[1];//(unsigned short int)SSID_Info.ip_addr[1];
		Scan_Infor.ipaddr[2] = holding_reg_params.ip_addr[2];//(unsigned short int)SSID_Info.ip_addr[2];
		Scan_Infor.ipaddr[3] = holding_reg_params.ip_addr[3];//(unsigned short int)SSID_Info.ip_addr[3];
	}
	else
	{
		Scan_Infor.ipaddr[0] = SSID_Info.ip_addr[0];
		Scan_Infor.ipaddr[1] = SSID_Info.ip_addr[1];
		Scan_Infor.ipaddr[2] = SSID_Info.ip_addr[2];
		Scan_Infor.ipaddr[3] = SSID_Info.ip_addr[3];
	}

	//port
	Scan_Infor.modbus_port = 502;//SSID_Info.modbus_port;  //tbd :????????????????

	// software rev
	Scan_Infor.firmwarerev = holding_reg_params.version_number_lo;//EEPROM_VERSION;
	// hardware rev
	Scan_Infor.hardwarerev = holding_reg_params.hardware_version;//HardwareVersion;

	Scan_Infor.instance_low = htons(23456); // hight byte first
	Scan_Infor.panel_number = holding_reg_params.modbus_address;//laddress; //  36
	Scan_Infor.instance_hi = htons(23456 >> 16); // hight byte first

	Scan_Infor.bootloader = 0;  // 0 - app, 1 - bootloader, 2 - wrong bootloader

	Scan_Infor.BAC_port = 47808;//SSID_Info.bacnet_port;//((Modbus.Bip_port & 0x00ff) << 8) + (Modbus.Bip_port >> 8);  //
	Scan_Infor.zigbee_exist = 0; // 0 - inexsit, 1 - exist
	Scan_Infor.subnet_protocal = 0;
	Scan_Infor.master_sn[0] = 0;
	Scan_Infor.master_sn[1] = 0;
	Scan_Infor.master_sn[2] = 0;
	Scan_Infor.master_sn[3] = 0;
	memcpy(Scan_Infor.panelname, holding_reg_params.panelname,20);
/*	if(holding_reg_params.which_project == PROJECT_FAN_MODULE)
		memcpy(Scan_Infor.panelname,(char*)"Fan-Module ",12);
	else
		memcpy(Scan_Infor.panelname,(char*)"AirLab-esp32",12);
*/
//	state = 1;
//	scanstart = 0;

}

static void udp_server_task(void *pvParameters)
{
    char rx_buffer[600];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    while (1) {
    	//if(SSID_Info.IP_Wifi_Status == WIFI_CONNECTED)
    	{
	#ifdef CONFIG_EXAMPLE_IPV4
			struct sockaddr_in dest_addr;
			dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
			dest_addr.sin_family = AF_INET;
			dest_addr.sin_port = htons(1234);
			addr_family = AF_INET;
			ip_protocol = IPPROTO_IP;
			inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
	#else // IPV6
			struct sockaddr_in6 dest_addr;
			bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
			dest_addr.sin6_family = AF_INET6;
			dest_addr.sin6_port = htons(PORT);
			addr_family = AF_INET6;
			ip_protocol = IPPROTO_IPV6;
			inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
	#endif

			int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
			if (sock < 0) {
				ESP_LOGE(UDP_TASK_TAG, "Unable to create socket: errno %d", errno);
				break;
			}
			ESP_LOGI(UDP_TASK_TAG, "Socket created");

			int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
			if (err < 0) {
				ESP_LOGE(UDP_TASK_TAG, "Socket unable to bind: errno %d", errno);
			}
			ESP_LOGI(UDP_TASK_TAG, "Socket bound, port %d", PORT);

			while (1) {

				ESP_LOGI(UDP_TASK_TAG, "Waiting for data");
				struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
				socklen_t socklen = sizeof(source_addr);
				int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

				// Error occurred during receiving
				if (len < 0) {
					ESP_LOGE(UDP_TASK_TAG, "recvfrom failed: errno %d", errno);
					break;
				}
				// Data received
				else 
				{
					// Get the sender's ip address as string
					if (source_addr.sin6_family == PF_INET) {
						inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
						ESP_LOGI(UDP_TASK_TAG, "IPV4 receive data");
					} else if (source_addr.sin6_family == PF_INET6) {
						inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
						ESP_LOGI(UDP_TASK_TAG, "IPV6 receive data");
					}

					//rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
					ESP_LOGI(UDP_TASK_TAG, "Received %d bytes from %s:", len, addr_str);
					ESP_LOG_BUFFER_HEX(UDP_TASK_TAG, rx_buffer, len);

					if(rx_buffer[0] == 0x64)
					{
						UdpData(0);
						ESP_LOGI(UDP_TASK_TAG, "receive data buffer[0] = 0x64");
						int err = sendto(sock, (uint8_t *)&Scan_Infor, sizeof(STR_SCAN_CMD), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
						if (err < 0) {
							ESP_LOGE(UDP_TASK_TAG, "Error occurred during sending: errno %d", errno);
							break;
						}
					}
				}
			}

			if (sock != -1) {
				ESP_LOGE(UDP_TASK_TAG, "Shutting down socket and restarting...");
				shutdown(sock, 0);
				close(sock);
			}
		}
		vTaskDelete(NULL);
    }
}

int my_listen_sock;
static void tcp_server_task(void *pvParameters)
{
    char rx_buffer[512];
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    uint16_t testCount = 0;
    struct sockaddr_in dest_addr;
	dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(502);
	addr_family = AF_INET;
	ip_protocol = IPPROTO_IP;

	struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
	uint addr_len;
	int sock;
	int len;

	isSocketCreated = true;

	//if(SSID_Info.IP_Wifi_Status == WIFI_CONNECTED)
	{
		// ^^^^^^^^^^^^^^^^^^^TCP^^^^^^^^^
		my_listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
		if (my_listen_sock < 0) {
			//holding_reg_params.testBuf[12] = 1000;
			ESP_LOGE(TCP_TASK_TAG, "Unable to create TCP socket: errno %d", errno);
			//break;
		}
		//holding_reg_params.testBuf[12] ++;
		ESP_LOGI(TCP_TASK_TAG, "Socket created");

		int err = bind(my_listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
		if (err != 0) {
			//holding_reg_params.testBuf[13] = 2000;
			ESP_LOGE(TCP_TASK_TAG, "TCP Socket unable to bind: errno %d", errno);
			//break;
		}
		ESP_LOGI(TCP_TASK_TAG, "TCP Socket bound, port %d", 502);
		//holding_reg_params.testBuf[13] ++;

		err = listen(my_listen_sock, 1);
		if (err != 0) {
			//holding_reg_params.testBuf[14] = 3000;
			ESP_LOGE(TCP_TASK_TAG, "Error occurred during listen: errno %d", errno);
			//break;
		}
		//holding_reg_params.testBuf[14]++;
		ESP_LOGI(TCP_TASK_TAG, "Socket listening");
	}

    while (1) {

    	//if(SSID_Info.IP_Wifi_Status == WIFI_CONNECTED)
    	{

			addr_len = sizeof(source_addr);
			sock = accept(my_listen_sock, (struct sockaddr *)&source_addr, &addr_len);
			if (sock < 0) {
				//holding_reg_params.testBuf[15] = 4000;
				ESP_LOGE(TCP_TASK_TAG, "Unable to accept connection: errno %d", errno);
				break;
			}
			//holding_reg_params.testBuf[15]++;
			ESP_LOGI(TCP_TASK_TAG, "Socket accepted");

			while (1) {
				//if(SSID_Info.IP_Wifi_Status == WIFI_CONNECTED)
				{
					len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
					if(testCount++ < 60000)
						holding_reg_params.testBuf[9] = testCount;
					else
						testCount = 0;
					// Error occurred during receiving
					if (len < 0) {
						ESP_LOGE(TCP_TASK_TAG, "recv failed: errno %d", errno);
						break;
					}
					// Connection closed
					else if (len == 0) {
						ESP_LOGI(TCP_TASK_TAG, "Connection closed");
						//holding_reg_params.testBuf[10] = 100;
						break;
					}
					// Data received
					else 
					{
						// Get the sender's ip address as string
						if (source_addr.sin6_family == PF_INET) {
							inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
						} else if (source_addr.sin6_family == PF_INET6) {
							inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
						}

						//rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
						ESP_LOGI(TCP_TASK_TAG, "Received %d bytes from %s:", len, addr_str);
						// ESP_LOGI(TAG, "%s", rx_buffer);

						if (len == 5)
						{
							ESP_LOGI(TCP_TASK_TAG, "Receive: %02x %02x %02x %02x %02x.", rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3], rx_buffer[4]);
						}

						ESP_LOG_BUFFER_HEX(TCP_TASK_TAG, rx_buffer, len);

						{
							if( (rx_buffer[0] == 0xee) && (rx_buffer[1] == 0x10) &&
								(rx_buffer[2] == 0x00) && (rx_buffer[3] == 0x00) &&
								(rx_buffer[4] == 0x00) && (rx_buffer[5] == 0x00) &&
								(rx_buffer[6] == 0x00) && (rx_buffer[7] == 0x00) )
							{
								start_fw_update();
							}
						}

						if( (rx_buffer[6]==holding_reg_params.modbus_address) || ((rx_buffer[6]==255) && (rx_buffer[7]!=0x19)))
						{
							responseCmd(WIFI, (uint8_t *)rx_buffer, len);
							if(modbus_wifi_len > 0)
							{
								holding_reg_params.stm32_uart_send[STM32_LED_WIFI_ON] = 2;
								int err = send(sock, (uint8_t *)&modbus_wifi_buf, modbus_wifi_len, 0);
								if (err < 0) {
									ESP_LOGE(TCP_TASK_TAG, "Error occurred during sending: errno %d", errno);
									break;
								}
								break;
							}
						}
					}
				}
			}

			if (sock != -1) {
				//holding_reg_params.testBuf[11] = 215;
				ESP_LOGE(TCP_TASK_TAG, "Shutting down socket and restarting...");
				shutdown(sock, 0);
				close(sock);
			}
		}
    	isSocketCreated = false;
		// vTaskDelete(NULL);
		//xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
    }
}

// static void detect_tcp_task(void *pvParameters)
// {
// 	while (1) {
// 		if(isSocketCreated == false){
// 			if(SSID_Info.IP_Wifi_Status == WIFI_CONNECTED)
// 				xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
// 		}
// 		vTaskDelay(300 / portTICK_RATE_MS);
// 	}
// }

void app_main()
{

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    //ESP_ERROR_CHECK(example_connect());
	read_default_from_flash();
	mass_flash_init();
	modbus_init();
	debug_info("modbus init finished^^^^^^^^");

    ESP_ERROR_CHECK(i2c_master_init());

    ethernet_init();

    holding_reg_params.which_project = PROJECT_SAUTER;//PROJECT_FAN_MODULE;//
	//tcpip_socket_init();
	if(holding_reg_params.which_project == PROJECT_SAUTER)
	{
		stm32_uart_init();
	}
	else if(holding_reg_params.which_project == PROJECT_FAN_MODULE)
	{
		holding_reg_params.fan_module_pwm2 = 0;
		led_pwm_init();
		led_init();
		pcnt_init();
		adc_init();
	}
    //microphone_init();
    //SSID_Info.IP_Wifi_Status = WIFI_CONNECTED;
    //connect_wifi();
    xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 6, NULL);
    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
    xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);

    //if(holding_reg_params.which_project != PROJECT_FAN_MODULE)
    	xTaskCreate(i2c_task,"i2c_task", 2048*2, NULL, 10, NULL);
    xTaskCreate(modbus_task,"modbus_task",4096, NULL, 3, NULL);
    //if(holding_reg_params.which_project != PROJECT_FAN_MODULE)
    {
    	xTaskCreate(input_task,"input_task",1024, NULL, 4, NULL);
    }
    xTaskCreate(ble_mesh_task,"ble_mesh_task",4096*2, NULL, 7, NULL);
	// xTaskCreate(detect_tcp_task, "detect_tcp_task", 1024,NULL, 6,NULL);
	// xTaskCreate(pyq1548_task,"pyq1548_task",1024*2, NULL, 10, NULL);
	// xTaskCreate(&ota_task, "ota_example_task", 8192, NULL, 5, NULL);
}
