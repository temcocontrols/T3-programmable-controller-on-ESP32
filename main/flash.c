#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "flash.h"
#include "wifi.h"
#include "define.h"
#include "ud_str.h"
#include "user_data.h"
#include "bacnet.h"

#include "unity.h"
#include "esp_partition.h"
#include "esp_system.h"
#include <string.h>
#include "ud_str.h"
#include "user_data.h"
#include "driver/uart.h"
#include "scan.h"

uint8_t ChangeFlash;
uint16_t count_write_Flash;
uint8_t count_reboot = 0;
extern S16_T timezone;
extern U8_T lcddisplay[7];
void Get_AVS(void);

const uint8 Var_Description[12][21];
const uint8 Var_label[12][9];

extern uint16_t input_cal[16];


#define POINT_INFO_ADDR	0
#define POINT_INFO_LEN 	0x10000

#define TRENDLOG_ADDR	0x10000
#define TRENDLOG_LEN	0x10000


esp_err_t save_uint8_to_flash(const char* key, uint8_t value)
{
	nvs_handle_t my_handle;
	esp_err_t err;
	// Open

	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;

	err = nvs_set_u8(my_handle, key, value);
	if (err != ESP_OK) return err;

	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t save_uint16_to_flash(const char* key, uint16_t value)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;

	err = nvs_set_u16(my_handle, key, value);
	if (err != ESP_OK) return err;

	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t save_int16_to_flash(const char* key, int16_t value)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;

	err = nvs_set_i16(my_handle, key, value);
	if (err != ESP_OK) return err;

	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t read_uint8_from_falsh(const char* key, uint8_t* value)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;
	err = nvs_get_u8(my_handle, key, value);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		return ESP_ERR_NVS_NOT_FOUND;
	}
	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t read_uint16_from_falsh(const char* key, uint16_t* value)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;
	err = nvs_get_u16(my_handle, key, value);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		return ESP_ERR_NVS_NOT_FOUND;
	}
	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t read_default_from_flash(void)
{
	nvs_handle_t my_handle;
	esp_err_t err;
	uint8_t temp[4];
	uint32_t len;

	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	  ESP_ERROR_CHECK(nvs_flash_erase());
	  ret = nvs_flash_init();

	}
	// Open

	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;
	debug_info("read_default_from_flash nvs_open\n");

	err = nvs_get_u8(my_handle,FLASH_MODBUS_ID, &Modbus.address);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.address = 1;
		nvs_set_u8(my_handle, FLASH_MODBUS_ID, Modbus.address);
	}
	err = nvs_get_u8(my_handle, FLASH_BAUD_RATE, &Modbus.baudrate[0]);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.baudrate[0] = 9;
		nvs_set_u8(my_handle, FLASH_BAUD_RATE, Modbus.baudrate[0]);
	}
	err = nvs_get_u8(my_handle, FLASH_BAUD_RATE2, &Modbus.baudrate[2]);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.baudrate[2] = 9;
		nvs_set_u8(my_handle, FLASH_BAUD_RATE2, Modbus.baudrate[2]);
	}
	err = nvs_get_u8(my_handle, FLASH_SERIAL_NUM1, &Modbus.serialNum[0]);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.serialNum[0] = 4;
		nvs_set_u8(my_handle, FLASH_SERIAL_NUM1, Modbus.serialNum[0]);
	}

	err = nvs_get_u8(my_handle, FLASH_SERIAL_NUM2, &Modbus.serialNum[1]);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.serialNum[1] = 4;
		nvs_set_u8(my_handle, FLASH_SERIAL_NUM2, Modbus.serialNum[1]);
	}

	err = nvs_get_u8(my_handle, FLASH_SERIAL_NUM3, &Modbus.serialNum[2]);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.serialNum[2] = 0;
		nvs_set_u8(my_handle, FLASH_SERIAL_NUM3, Modbus.serialNum[2]);
	}

	err = nvs_get_u8(my_handle, FLASH_SERIAL_NUM4, &Modbus.serialNum[3]);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.serialNum[3] = 0;
		nvs_set_u8(my_handle, FLASH_SERIAL_NUM4, Modbus.serialNum[3]);
	}

	err = nvs_get_u8(my_handle, FLASH_INSTANCE1, &temp[0]);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		temp[0] = Modbus.serialNum[0];
		nvs_set_u8(my_handle, FLASH_INSTANCE1, temp[0]);
	}

	err = nvs_get_u8(my_handle, FLASH_INSTANCE2, &temp[1]);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		temp[1] = Modbus.serialNum[1];
		nvs_set_u8(my_handle, FLASH_INSTANCE2, temp[1]);
	}

	err = nvs_get_u8(my_handle, FLASH_INSTANCE3, &temp[2]);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		temp[2] = Modbus.serialNum[2];
		nvs_set_u8(my_handle, FLASH_INSTANCE3, temp[2]);
	}

	err = nvs_get_u8(my_handle, FLASH_INSTANCE4, &temp[3]);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		temp[3] = Modbus.serialNum[3];
		nvs_set_u8(my_handle, FLASH_INSTANCE4, temp[3]);
	}
	Instance = ((uint32)temp[3]<<24)+((uint32)temp[2]<<16)+((uint16)temp[1]<<8) + temp[0];
	if(Instance >= 0x3fffff)
	{
		Instance =  Modbus.serialNum[0] + (U16_T)(Modbus.serialNum[1] << 8) + ((U32_T)Modbus.serialNum[2] << 16) + ((U32_T)Modbus.serialNum[3] << 24);
		nvs_set_u8(my_handle, FLASH_INSTANCE1, temp[0]);
		nvs_set_u8(my_handle, FLASH_INSTANCE2, temp[1]);
		nvs_set_u8(my_handle, FLASH_INSTANCE3, temp[2]);
		nvs_set_u8(my_handle, FLASH_INSTANCE4, temp[3]);
	}

	err = nvs_get_u8(my_handle, FLASH_UART_CONFIG, &Modbus.com_config[0]);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.com_config[0] = MODBUS_SLAVE;
		nvs_set_u8(my_handle, FLASH_UART_CONFIG, Modbus.com_config[0]);
	}
	err = nvs_get_u8(my_handle, FLASH_UART2_CONFIG, &Modbus.com_config[2]);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.com_config[2] = MODBUS_SLAVE;
		nvs_set_u8(my_handle, FLASH_UART2_CONFIG, Modbus.com_config[2]);
	}
	err = nvs_get_u8(my_handle, FLASH_MINI_TYPE, &Modbus.mini_type);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.mini_type = MINI_NANO;
		nvs_set_u8(my_handle, FLASH_MINI_TYPE, Modbus.mini_type);
	}

	err = nvs_get_u16(my_handle, FLASH_NETWORK_NUMBER, &Modbus.network_number);

	if((Modbus.network_number == 0) || (err == ESP_ERR_NVS_NOT_FOUND))
	{
		Modbus.network_number = 0xffff;
		nvs_set_u16(my_handle, FLASH_NETWORK_NUMBER, Modbus.network_number);
	}

	err = nvs_get_u16(my_handle, FLASH_MSTP_NETWORK, &Modbus.mstp_network);
	if((Modbus.mstp_network == 0) ||((Modbus.mstp_network == 0xffff)) || (err == ESP_ERR_NVS_NOT_FOUND))
	{
		Modbus.mstp_network = 1;
		nvs_set_u16(my_handle, FLASH_MSTP_NETWORK, Modbus.mstp_network);
	}

	err = nvs_get_u16(my_handle, FLASH_TCP_PORT, &Modbus.tcp_port);
	if((Modbus.tcp_port == 0) ||((Modbus.tcp_port == 0xffff)) || (err == ESP_ERR_NVS_NOT_FOUND))
	{
		Modbus.tcp_port = 502;
		nvs_set_u16(my_handle, FLASH_TCP_PORT, Modbus.tcp_port);
	}

	err = nvs_get_u8(my_handle, FLASH_TCP_TYPE, &Modbus.tcp_type);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.tcp_type = 1;  //AUTO
		nvs_set_u8(my_handle, FLASH_TCP_TYPE, Modbus.tcp_type);
	}

	nvs_get_u16(my_handle, FLASH_TIME_ZONE, (uint16 *)&timezone);
	nvs_get_u16(my_handle, FLASH_DSL, &Daylight_Saving_Time);

	err = nvs_get_u8(my_handle, FLASH_MAX_VARS, &max_vars);
	if(err == ESP_ERR_NVS_NOT_FOUND || max_vars == 0)
	{
		max_vars = 128;
		nvs_set_u8(my_handle, FLASH_MAX_VARS, max_vars);
	}
	err = nvs_get_u8(my_handle, FLASH_MAX_OUTS, &max_outputs);
	if(err == ESP_ERR_NVS_NOT_FOUND || max_outputs == 0)
	{
		max_outputs = 64;
		nvs_set_u8(my_handle, FLASH_MAX_OUTS, max_outputs);
	}
	err = nvs_get_u8(my_handle, FLASH_MAX_INS, &max_inputs);
	if(err == ESP_ERR_NVS_NOT_FOUND || max_inputs == 0)
	{
		max_inputs = 64;
		nvs_set_u8(my_handle, FLASH_MAX_INS, max_inputs);
	}

	err = nvs_get_u8(my_handle, FLASH_EN_USERNAME, &Modbus.en_username);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.en_username = MINI_NANO;
		nvs_set_u8(my_handle, FLASH_EN_USERNAME, Modbus.en_username);
	}

	err = nvs_get_u8(my_handle, FLASH_BOOTLOADER, &Modbus.IspVer);
	err = nvs_get_u8(my_handle, FLASH_COUNT_REBOOT, &count_reboot);
	if(count_reboot >= 2)
	{ // reboot
		//start_fw_update();
	}
	else
	{
		nvs_set_u8(my_handle, FLASH_COUNT_REBOOT, ++count_reboot);
	}

	if(err == ESP_ERR_NVS_NOT_FOUND)
	{// old ISP, REV < 26
		//Modbus.mini_type = MINI_NANO;
		//nvs_set_u8(my_handle, FLASH_MINI_TYPE, Modbus.mini_type);

	}

	len = sizeof(STR_SSID);
	nvs_get_blob(my_handle, FLASH_SSID_INFO, &SSID_Info, &len);

	len = 12;
	nvs_get_blob(my_handle, FLASH_NET_INFO, &Modbus.ip_addr[0], &len);

	// panel name
	len = 20;
	err = nvs_get_blob(my_handle, FLASH_PANEL_NAME, &panelname, &len);

	len = 7;
	err = nvs_get_blob(my_handle, FLASH_LCD_CONFIG, &lcddisplay, &len);
	if(lcddisplay[0] == 0xff || lcddisplay[0] == 0)
	{
		memset(lcddisplay,0,sizeof(lcdconfig));
		lcddisplay[0] = 1;
		lcddisplay[5] = IN;
		lcddisplay[6] = 9;  // IN9 is internal termperature
	}
	// read input calibrate
	err = nvs_get_u16(my_handle, FLASH_IN1_CAL, &input_cal[0]);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		input_cal[0] = 4095;
		nvs_set_u16(my_handle, FLASH_IN1_CAL, input_cal[0]);
	}
	err = nvs_get_u16(my_handle, FLASH_IN2_CAL, &input_cal[1]);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		input_cal[1] = 4095;
		nvs_set_u16(my_handle, FLASH_IN2_CAL, input_cal[1]);
	}
	err = nvs_get_u16(my_handle, FLASH_IN3_CAL, &input_cal[2]);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		input_cal[2] = 4095;
		nvs_set_u16(my_handle, FLASH_IN3_CAL, input_cal[2]);
	}
	err = nvs_get_u16(my_handle, FLASH_IN4_CAL, &input_cal[3]);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		input_cal[3] = 4095;
		nvs_set_u16(my_handle, FLASH_IN4_CAL, input_cal[3]);
	}
	err = nvs_get_u16(my_handle, FLASH_IN5_CAL, &input_cal[4]);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		input_cal[4] = 4095;
		nvs_set_u16(my_handle, FLASH_IN5_CAL, input_cal[4]);
	}

	err = nvs_get_u16(my_handle, FLASH_IN6_CAL, &input_cal[5]);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		input_cal[5] = 4095;
		nvs_set_u16(my_handle, FLASH_IN6_CAL, input_cal[5]);
	}
	err = nvs_get_u16(my_handle, FLASH_IN7_CAL, &input_cal[6]);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		input_cal[6] = 4095;
		nvs_set_u16(my_handle, FLASH_IN7_CAL, input_cal[6]);
	}
	err = nvs_get_u16(my_handle, FLASH_IN8_CAL, &input_cal[7]);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		input_cal[7] = 4095;
		nvs_set_u16(my_handle, FLASH_IN8_CAL, input_cal[7]);
	}
	err = nvs_get_u16(my_handle, FLASH_IN9_CAL, &input_cal[8]);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		input_cal[8] = 4095;
		nvs_set_u16(my_handle, FLASH_IN9_CAL, input_cal[8]);
	}
	err = nvs_get_u16(my_handle, FLASH_IN10_CAL, &input_cal[9]);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		input_cal[9] = 4095;
		nvs_set_u16(my_handle, FLASH_IN10_CAL, input_cal[9]);
	}

	err = nvs_get_u16(my_handle, FLASH_IN11_CAL, &input_cal[10]);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		input_cal[10] = 4095;
		nvs_set_u16(my_handle, FLASH_IN11_CAL, input_cal[10]);
	}
	err = nvs_get_u16(my_handle, FLASH_IN12_CAL, &input_cal[11]);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		input_cal[11] = 4095;
		nvs_set_u16(my_handle, FLASH_IN12_CAL, input_cal[11]);
	}
	err = nvs_get_u16(my_handle, FLASH_IN13_CAL, &input_cal[12]);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		input_cal[12] = 4095;
		nvs_set_u16(my_handle, FLASH_IN13_CAL, input_cal[12]);
	}
	err = nvs_get_u16(my_handle, FLASH_IN14_CAL, &input_cal[13]);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		input_cal[13] = 4095;
		nvs_set_u16(my_handle, FLASH_IN14_CAL, input_cal[13]);
	}
	err = nvs_get_u16(my_handle, FLASH_IN15_CAL, &input_cal[14]);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		input_cal[14] = 4095;
		nvs_set_u16(my_handle, FLASH_IN15_CAL, input_cal[14]);
	}
	err = nvs_get_u16(my_handle, FLASH_IN16_CAL, &input_cal[15]);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		input_cal[15] = 4095;
		nvs_set_u16(my_handle, FLASH_IN16_CAL, input_cal[15]);
	}


	err = nvs_get_u8(my_handle, FLASH_ICON_CONFIG, &Modbus.icon_config);
	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.icon_config = 0;
		nvs_set_u8(my_handle, FLASH_ICON_CONFIG, Modbus.icon_config);
	}
	// Close
	nvs_close(my_handle);

	Flash_Inital();


	return ESP_OK;
}


void clear_count_reboot(void)
{
	save_uint8_to_flash(FLASH_COUNT_REBOOT,0);
}

typedef struct
{
	U16_T addr;
	U16_T len;
	U8_T valid;
}STR_Flash_POS;

STR_Flash_POS  Flash_Position[24];

void Flash_Inital(void)
{
	uint8_t loop;
	uint16_t baseAddr = 0;
	uint16_t  len = 0;
	ChangeFlash = 0;
	count_write_Flash = 0;
	for(loop = 0;loop < MAX_POINT_TYPE;loop++)
	{
		Flash_Position[loop].valid = 1;
		switch(loop)
		{
		case OUT:
			baseAddr = 0;
#if NEW_IO
			if(max_outputs <= MAX_OUTS)
				len = sizeof(Str_out_point) * MAX_OUTS;
			else
				len = sizeof(Str_out_point) * max_outputs;
#else
			len = sizeof(Str_out_point) * MAX_OUTS;
#endif
			break;
		case IN:
			baseAddr += len;
#if NEW_IO
			if(max_inputs <= MAX_INS)
				len = sizeof(Str_in_point) * MAX_INS;
			else
				len = sizeof(Str_in_point) * max_inputs;
#else
			len = sizeof(Str_in_point) * MAX_INS;
#endif
			break;
		case VAR:
			baseAddr += len;
#if NEW_IO
			if(max_vars <= MAX_VARS)
				len = sizeof(Str_variable_point) * max_vars;
			else
				len = sizeof(Str_variable_point) * max_vars;
#else
			len = sizeof(Str_variable_point) * MAX_VARS;
#endif
			break;
		case CON:
			baseAddr += len;
			len = sizeof(Str_controller_point) * MAX_CONS;
			break;
		case WRT:
			baseAddr += len;
			len = sizeof(Str_weekly_routine_point) * MAX_WR;
			break;
		case AR:
			baseAddr += len;
			len = sizeof(Str_annual_routine_point) * MAX_AR;
			break;
		case PRG:
			baseAddr += len;
			len = sizeof(Str_program_point) * MAX_PRGS;
			break;
	/*	case TBL:
			baseAddr += len;
			len = sizeof(Tbl_point) * MAX_TBLS * 16;
			break;
		case TZ:
			baseAddr += len;
			len = sizeof(Str_totalizer_point) * MAX_TOTALIZERS;
			break;*/
		case AMON:
			baseAddr += len;
			len = sizeof(Str_monitor_point) * MAX_MONITORS;
			break;
		case GRP:
			baseAddr += len;
			len = sizeof(Control_group_point) * MAX_GRPS;
			break;
	/*	case ARRAY:
			baseAddr += len;
			len = sizeof(Str_array_point) * MAX_ARRAYS;
			break;	*/
		case ALARMM:
			baseAddr += len;
			len = sizeof(Alarm_point) * MAX_ALARMS;
			break;
		/*case ALARM_SET:
			baseAddr += len;
			len = sizeof(Alarm_set_point) * MAX_ALARMS_SET;
			break;*/
		case PRG_CODE:
			baseAddr += len;
			len = MAX_PRGS * MAX_CODE * CODE_ELEMENT;
			break;
		case UNIT:
			baseAddr += len;
			len = sizeof(Units_element) * MAX_DIG_UNIT;
			break;
		case USER_NAME:
			baseAddr += len;
			len = sizeof(Password_point) * MAX_PASSW;
			break;
		case WR_TIME:
			baseAddr += len;
			len = sizeof(Wr_one_day) * MAX_WR * MAX_SCHEDULES_PER_WEEK;
			break;
		case AR_DATA:
			baseAddr += len;
			len = sizeof(S8_T) * MAX_AR * AR_DATES_SIZE;
			break;
		case GRP_POINT:
			baseAddr += len;
			len = sizeof(Str_grp_element) * 240;
			break;
		case TBL:
			baseAddr += len;
			len = sizeof(Str_table_point) * MAX_TBLS ;
			break;
		case SUB_DB:
			baseAddr += len;
			len = sizeof(SCAN_DB) * SUB_NO;
			break;
		/*case ID_ROUTION:
			baseAddr += len;
			len = STORE_ID_LEN * 254;
			break;*/
		default:
			//len = 0;
			Flash_Position[loop].valid = 0;
			break;
		}

		//if(len % 2048 != 0)
			//	len = (len / 2048 + 1) * 2048;

		Flash_Position[loop].addr = baseAddr;
		Flash_Position[loop].len = len;
		//write_page_en[loop] = 0;
	}
	for(loop = 0;loop < MAX_PRGS;loop++)
		programs[loop].real_byte = 0;
}

esp_err_t save_wifi_info(void)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	debug_info("save_wifi_info\n");
	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;


	err = nvs_set_blob(my_handle, FLASH_SSID_INFO, (const void*)(&SSID_Info), sizeof(STR_SSID));
	if (err != ESP_OK) return err;
	debug_info("nvs_set_blob\n");


	// Commit
	err = nvs_commit(my_handle);
	if (err != ESP_OK) return err;
	debug_info("nvs_commit\n");

	// Close
	nvs_close(my_handle);
	return ESP_OK;
}


void Set_Object_Name(char * name)
{
	// store it to flash memory
	memcpy(panelname,name,strlen(name));
	save_block(FLASH_BLOCK2_PN);
}

esp_err_t Save_Ethernet_Info(void)
{
	esp_err_t err = 0;
	err = save_block(FLASH_BLOCK3_NET);
	return err;
}

esp_err_t Save_Lcd_config(void)
{
	esp_err_t err = 0;
	err = save_block(FLASH_BLOCK_LCD_CONFIG);
	return err;
}

void Store_Instance_To_Eeprom(uint32_t Instance)
{
	save_uint8_to_flash(FLASH_INSTANCE1,Instance);
	save_uint8_to_flash(FLASH_INSTANCE2,(U8_T)(Instance >> 8));
	save_uint8_to_flash(FLASH_INSTANCE3,(U8_T)(Instance >> 16));
	save_uint8_to_flash(FLASH_INSTANCE4,(U8_T)(Instance >> 24));

}

esp_err_t save_block(uint8_t key)
{
	nvs_handle_t my_handle;
	esp_err_t err;
	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;

	switch(key)
	{
	case FLASH_BLOCK1_SSID:
		err = nvs_set_blob(my_handle, FLASH_SSID_INFO, (const void*)(&SSID_Info), sizeof(STR_SSID));
		if (err != ESP_OK) return err;
		break;
	case FLASH_BLOCK2_PN:
		err = nvs_set_blob(my_handle, FLASH_PANEL_NAME, (const void*)(&panelname[0]), 20);
		if (err != ESP_OK) return err;
		break;
	case FLASH_BLOCK3_NET:
		err = nvs_set_blob(my_handle, FLASH_NET_INFO, (const void*)(&Modbus.ip_addr[0]), 12);
		if (err != ESP_OK) return err;
		break;
	case FLASH_BLOCK_LCD_CONFIG:
		err = nvs_set_blob(my_handle, FLASH_LCD_CONFIG, (const void*)(&lcddisplay[0]), 7);
		if (err != ESP_OK) return err;
		break;
	default:
		break;
	}
	// Commit
	err = nvs_commit(my_handle);
	if (err != ESP_OK) return err;
	debug_info("nvs_commit\n");

	// Close
	nvs_close(my_handle);
	return ESP_OK;
}

typedef struct
{
	U8_T table;
	U16_T index;
	U8_T flag;
	U32_T len;
	//U8_T dat[500];
}STR_flag_flash;


esp_err_t save_point_info(uint8_t point_type)
{
	STR_flag_flash ptr_flash;
	uint8_t err=0xff;
	uint16_t loop;
	//  step 1: Ѱ���û�flash id

	const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_ANY, "storage");

	assert(partition != NULL);

	err = esp_partition_erase_range(partition, POINT_INFO_ADDR, POINT_INFO_LEN/*partition->size*/);
	if(err!=0)
	{
		return err;//ESP_LOGI(TAG, "user  flash erase range ----%d",err);
	}


	//debug_info(" erase ok");
	for(loop = 0;loop < MAX_POINT_TYPE;loop++)
	{
		//if(loop == point_type)
		{
			uint8_t *tempbuf = NULL;
			ptr_flash.table = loop;
			ptr_flash.len = Flash_Position[loop].len;

#if NEW_IO
			if(loop == OUT)
			{
				if(new_outputs != NULL)
				{
					tempbuf = new_outputs;
					Flash_Position[loop].len = max_outputs *sizeof(Str_out_point);
				}
			}
			else if(loop == IN)
			{
				if(new_inputs != NULL)
				{
					tempbuf = new_inputs;
					Flash_Position[loop].len = max_inputs *sizeof(Str_in_point);
				}
			}
			else if(loop == VAR)
			{
				if(new_vars != NULL)
				{
					tempbuf = new_vars;
					Flash_Position[loop].len = max_vars *sizeof(Str_variable_point);
				}
			}
			else
				tempbuf = (uint8_t*)malloc(ptr_flash.len);
#else
			tempbuf = (uint8_t*)malloc(ptr_flash.len);
#endif
			switch(loop)
			{
#if !NEW_IO
			case OUT:
				memcpy(tempbuf,&outputs,sizeof(Str_out_point) * MAX_OUTS);
				break;
			case IN:
				memcpy(tempbuf,&inputs,sizeof(Str_in_point) * MAX_INS);
				break;
			case VAR:
				memcpy(tempbuf,&vars,sizeof(Str_variable_point) * MAX_VARS);
				break;
#endif
			case CON:
				memcpy(tempbuf,&controllers,sizeof(Str_controller_point) * MAX_CONS);
				break;

			case WRT:
				memcpy(tempbuf,&weekly_routines,sizeof(Str_weekly_routine_point) * MAX_WR);
				break;
			case AR:
				memcpy(tempbuf,&annual_routines,sizeof(Str_annual_routine_point) * MAX_AR);
				break;
			case PRG:
				memcpy(tempbuf,&programs,sizeof(Str_program_point) * MAX_PRGS);
				break;
	#if 1
			case TBL:
				memcpy(tempbuf,&custom_tab,sizeof(Str_table_point) * MAX_TBLS);
				break;
		/*	case TZ:
				memcpy(&tempbuf,&totalizers,sizeof(Str_totalizer_point) * MAX_TOTALIZERS);
				break;	*/
			case AMON:
				memcpy(tempbuf,&monitors,sizeof(Str_monitor_point) * MAX_MONITORS);
				break;
		case GRP:
				memcpy(tempbuf,&control_groups,sizeof(Control_group_point) * MAX_GRPS);
				break;
	/*			case ARRAY:
				memcpy(&tempbuf,&arrays,sizeof(Str_array_point) * MAX_ARRAYS);
				break;
			case ALARMM:
	//				memcpy(&tempbuf,&alarms,sizeof(Alarm_point) * MAX_ALARMS);
				break;
				case ALARM_SET:
				memcpy(tempbuf,&alarms_set,sizeof(Alarm_set_point) * MAX_ALARMS_SET);
				break;*/
			case PRG_CODE: //prg_code[MAX_PRGS][MAX_CODE * CODE_ELEMENT];
				memcpy(tempbuf,&prg_code,MAX_CODE * CODE_ELEMENT * MAX_PRGS);
				break;
			case UNIT:
				memcpy(tempbuf,&digi_units,sizeof(Units_element) * MAX_DIG_UNIT);
				break;
			case USER_NAME:
				memcpy(tempbuf,&passwords,sizeof(Password_point) * MAX_PASSW);
				break;
			case WR_TIME:
				memcpy(tempbuf,&wr_times,sizeof(Wr_one_day) * 9 * MAX_WR);
				break;
			case AR_DATA:
				memcpy(tempbuf,&ar_dates,46 * sizeof(S8_T) * MAX_AR);
				break;

			case GRP_POINT:
				memcpy(tempbuf,&group_data,sizeof(Str_grp_element) * 240);
				break;
	/*		case ID_ROUTION:
				for(i = 0;i < 254;i++)
				memcpy(&tempbuf[i * STORE_ID_LEN],&ID_Config[i], STORE_ID_LEN);		// store 15 bytes
				break;*/
			case SUB_DB:
				memcpy(tempbuf,&scan_db,sizeof(SCAN_DB) * SUB_NO);
				break;
	#endif
			default:
				break;

			}

		// step 3�������Ҫ������û�����
			if(Flash_Position[loop].valid == 1)
			{
				err = esp_partition_write(partition, Flash_Position[loop].addr,tempbuf,Flash_Position[loop].len);
				//debug_info("write ...");

			}
#if NEW_IO
		if((loop != OUT) && (loop != IN) && (loop != VAR))
			free(tempbuf);
#else
		free(tempbuf);
#endif

		}
	   //debug_info("user  flash write success");
	}
	return ESP_OK;
}

void Initial_points(uint8_t point_type)
{
	Str_points_ptr ptr;
	uint8_t i;
	if(point_type == OUT)
	{
		if(Modbus.mini_type == PROJECT_FAN_MODULE)
		{
			ptr = put_io_buf(OUT,0);
			memcpy(ptr.pout->description,"FAN AO",strlen("FAN AO"));
			if(ptr.pout->range == 0)
			{
				ptr.pout->switch_status = 1;
				ptr.pout->auto_manual = 1;
				ptr.pout->digital_analog = 1;
				ptr.pout->range = 4;
				memcpy(ptr.pout->description,"FAN PWM 1",strlen("FAN PWM 1"));
				memcpy(ptr.pout->label,"FANOUT1",strlen("FANOUT1"));
			}
			ptr = put_io_buf(OUT,1);
			memcpy(ptr.pout->description,"FAN PWM",strlen("FAN PWM"));
			if(ptr.pout->range == 0)
			{
				ptr.pout->switch_status = 1;
				ptr.pout->auto_manual = 1;
				ptr.pout->digital_analog = 1;
				ptr.pout->range = 4;
				memcpy(ptr.pout->description,"FAN PWM 2",strlen("FAN PWM 2"));
				memcpy(ptr.pout->label,"FANOUT2",strlen("FANOUT2"));
			}
		}
		if(Modbus.mini_type == PROJECT_TRANSDUCER)
		{
			ptr = put_io_buf(OUT,1);
			memcpy(ptr.pout->description,"TEMP OUTPUT",strlen("TEMP OUTPUT"));
			if(ptr.pout->range == 0)
			{
				ptr.pout->switch_status = 1;
				ptr.pout->auto_manual = 1;
				ptr.pout->digital_analog = 1;
				ptr.pout->range = 4;
				memcpy(ptr.pout->description,"TEMP OUTPUT",strlen("TEMP OUTPUT"));
				memcpy(ptr.pout->label,"TEMPOUT",strlen("TEMPOUT"));
			}

			ptr = put_io_buf(OUT,1);
			memcpy(ptr.pout->description,"HUMI OUTPUT",strlen("HUMI OUTPUT"));
			if(ptr.pout->range == 0)
			{
				ptr.pout->switch_status = 1;
				ptr.pout->auto_manual = 1;
				ptr.pout->digital_analog = 1;
				ptr.pout->range = 4;
				memcpy(outputs[1].description,"HUMI OUTPUT",strlen("HUMI OUTPUT"));
				memcpy(outputs[1].label,"HUMIOUT",strlen("HUMIOUT"));
			}

			ptr = put_io_buf(OUT,2);
			memcpy(ptr.pout->description,"CO2",strlen("CO2"));
			if(ptr.pout->range == 0)
			{
				ptr.pout->switch_status = 1;
				ptr.pout->auto_manual = 1;
				ptr.pout->digital_analog = 1;
				ptr.pout->range = 4;
				memcpy(ptr.pout->description,"CO2 OUTPUT",strlen("CO2 OUTPUT"));
				memcpy(ptr.pout->label,"CO2OUT",strlen("CO2OUT"));
			}
		}
	}
	else if(point_type == IN)
	{
		if(Modbus.mini_type == PROJECT_FAN_MODULE)
		{
			ptr = put_io_buf(IN,0);
			memcpy(ptr.pin->description,"TEMP ON BOARD",strlen("TEMP ON BOARD"));
			memcpy(ptr.pin->label,"TEMP1",strlen("TEMP1"));
			if(ptr.pin->range == 0)
			{
				ptr.pin->digital_analog = 1;
				ptr.pin->range = 3;
				memcpy(ptr.pin->description,"TEMP ON BOARD",strlen("TEMP ON BOARD"));
			}

			ptr = put_io_buf(IN,1);
			memcpy(ptr.pin->description,"HUMIDITY",strlen("HUMIDITY"));
			memcpy(ptr.pin->label,"HUM",strlen("HUM"));
			if(ptr.pin->range == 0)
			{
				ptr.pin->digital_analog = 1;
				ptr.pin->range = 27;
				memcpy(ptr.pin->description,"HUMIDITY",strlen("HUMIDITY"));
			}

			ptr = put_io_buf(IN,2);
			memcpy(ptr.pin->description,"TEMP REMOTE",strlen("TEMP REMOTE"));
			memcpy(ptr.pin->label,"TEMP2",strlen("TEMP2"));
			if(ptr.pin->range == 0)
			{
				ptr.pin->digital_analog = 1;
				ptr.pin->range = 3;
				memcpy(ptr.pin->description,"TEMP REMOTE",strlen("TEMP REMOTE"));
			}

			ptr = put_io_buf(IN,3);
			memcpy(ptr.pin->description,"FAN STATUS",strlen("FAN STATUS"));
			memcpy(ptr.pin->label,"FANSTAT",strlen("FANSTAT"));
			if(ptr.pin->range == 0)
			{
				ptr.pin->digital_analog = 1;
				ptr.pin->range = 19;
				memcpy(ptr.pin->description,"FAN STATUS",strlen("FAN STATUS"));
			}

			ptr = put_io_buf(IN,4);
			memcpy(ptr.pin->description,"FAN SPEED",strlen("FAN SPEED"));
			memcpy(ptr.pin->label,"FANSPD",strlen("FANSPD"));
			if(ptr.pin->range == 0)
			{
				inputs[4].digital_analog = 1;
				inputs[4].range = 26;
				memcpy(inputs[4].description,"FAN SPEED",strlen("FAN SPEED"));
			}

			ptr = put_io_buf(IN,5);
			memcpy(ptr.pin->description,"THERMEL TEMP",strlen("THERMEL TEMP"));
			memcpy(ptr.pin->label,"TEMP3",strlen("TEMP3"));
			if(ptr.pin->range == 0)
			{
				ptr.pin->digital_analog = 1;
				ptr.pin->range = 3;
				memcpy(ptr.pin->description,"THERMEL TEMP",strlen("THERMEL TEMP"));
			}
		}

		if(Modbus.mini_type == PROJECT_TRANSDUCER)
		{
			ptr = put_io_buf(IN,0);
			memcpy(ptr.pin->description,"TEMP ON BOARD",strlen("TEMP ON BOARD"));
			memcpy(inputs[0].label,"TEMP1",strlen("TEMP1"));
			if(ptr.pin->range == 0)
			{
				ptr.pin->digital_analog = 1;
				ptr.pin->range = 3;
				memcpy(ptr.pin->description,"TEMP ON BOARD",strlen("TEMP ON BOARD"));
			}

			ptr = put_io_buf(IN,1);
			memcpy(ptr.pin->description,"HUMIDITY",strlen("HUMIDITY"));
			memcpy(ptr.pin->label,"HUM",strlen("HUM"));
			if(ptr.pin->range == 0)
			{
				ptr.pin->digital_analog = 1;
				ptr.pin->range = 27;
				memcpy(ptr.pin->description,"HUMIDITY",strlen("HUMIDITY"));
			}

			ptr = put_io_buf(IN,2);
			memcpy(ptr.pin->label,"CO2",strlen("CO2"));
			memcpy(ptr.pin->description,"CO2",strlen("CO2"));
			if(ptr.pin->range == 0)
			{
				ptr.pin->digital_analog = 1;
				ptr.pin->range = 28;
				memcpy(ptr.pin->description,"CO2",strlen("CO2"));
			}
		}

		if(Modbus.mini_type == PROJECT_AIRLAB)
		{
			ptr = put_io_buf(IN,0);
			memcpy(ptr.pin->description,"TEMPERATURE",strlen("TEMPERATURE"));
			memcpy(ptr.pin->label,"TEMP",strlen("TEMP"));
			for(i = 0;i < 18;i++)
				ptr.pin->digital_analog = 1;
			ptr.pin->range = R10K_40_120DegC;

			ptr = put_io_buf(IN,1);
			memcpy(ptr.pin->description,"HUMIDITY",strlen("HUMIDITY"));
			memcpy(ptr.pin->label,"HUM",strlen("HUM"));
			ptr.pin->range = Humidty;

			ptr = put_io_buf(IN,2);
			memcpy(ptr.pin->description,"CO2 ",strlen("CO2 "));
			memcpy(ptr.pin->label,"CO2",strlen("CO2"));
			ptr.pin->range = CO2_PPM;

			ptr = put_io_buf(IN,3);
			memcpy(ptr.pin->description,"TVOC",strlen("TVOC"));
			memcpy(ptr.pin->label,"TVOC",strlen("TVOC"));
			ptr.pin->range = TVOC_PPB;

			ptr = put_io_buf(IN,4);
			memcpy(ptr.pin->description,"PM1.0DEN",strlen("PM1.0DEN"));
			memcpy(ptr.pin->label,"PM1.0DEN",strlen("PM1.0DEN"));
			ptr.pin->range = UG_M3;

			ptr = put_io_buf(IN,5);
			memcpy(ptr.pin->description,"PM2.5DEN",strlen("PM2.5DEN"));
			memcpy(ptr.pin->label,"PM2.5DEN",strlen("PM2.5DEN"));
			ptr.pin->range = UG_M3;

			ptr = put_io_buf(IN,6);
			memcpy(ptr.pin->description,"PM4.0DEN",strlen("PM4.0DEN"));
			memcpy(ptr.pin->label,"PM4.0DEN",strlen("PM1.0DEN"));
			ptr.pin->range = UG_M3;

			ptr = put_io_buf(IN,7);
			memcpy(ptr.pin->description,"PM10DEN",strlen("PM10DEN"));
			memcpy(ptr.pin->label,"PM10DEN",strlen("PM10DEN"));
			ptr.pin->range = UG_M3;

			ptr = put_io_buf(IN,8);
			memcpy(ptr.pin->description,"PM0.5C",strlen("PM0.5C"));
			memcpy(ptr.pin->label,"PM0.5C",strlen("PM0.5C"));
			ptr.pin->range = NUM_CM3;

			ptr = put_io_buf(IN,9);
			memcpy(inputs[9].description,"PM1.0C",strlen("PM1.0C"));
			memcpy(inputs[9].label,"PM1.0C",strlen("PM1.0C"));
			ptr.pin->range = NUM_CM3;

			ptr = put_io_buf(IN,10);
			memcpy(ptr.pin->description,"PM2.5C",strlen("PM2.5C"));
			memcpy(ptr.pin->label,"PM2.5C",strlen("PM2.5C"));
			ptr.pin->range = NUM_CM3;

			ptr = put_io_buf(IN,11);
			memcpy(ptr.pin->description,"PM4.0C",strlen("PM4.0C"));
			memcpy(ptr.pin->label,"PM4.0C",strlen("PM4.0C"));
			ptr.pin->range = NUM_CM3;

			ptr = put_io_buf(IN,12);
			memcpy(ptr.pin->description,"PM10C",strlen("PM10C"));
			memcpy(ptr.pin->label,"PM10C",strlen("PM10C"));
			ptr.pin->range = NUM_CM3;

			ptr = put_io_buf(IN,13);
			memcpy(ptr.pin->description,"P_size",strlen("P_size"));
			memcpy(ptr.pin->label,"P_size",strlen("P_size"));

			ptr = put_io_buf(IN,14);
			memcpy(ptr.pin->description,"SOUND",strlen("SOUND"));
			memcpy(ptr.pin->label,"SOUND",strlen("SOUND"));
			ptr.pin->range = DB;

			ptr = put_io_buf(IN,15);
			memcpy(ptr.pin->description,"LIGHT",strlen("LIGHT"));
			memcpy(ptr.pin->label,"LIGHT",strlen("LIGHT"));
			ptr.pin->range = LUX;

			ptr = put_io_buf(IN,16);
			memcpy(ptr.pin->description,"OCCUPIED SENSOR",strlen("OCCUPIED SENSOR"));
			memcpy(ptr.pin->label,"OCC",strlen("OCC"));
			ptr.pin->range = 0;
		}
	}
	else if(point_type == VAR)
	{
		if(Modbus.mini_type == PROJECT_AIRLAB)
		{
			for(i = 0; i < 13; i++ )
			{
				ptr = put_io_buf(VAR,i);
				memcpy(ptr.pvar->description,Var_Description[i],strlen((char *)Var_Description[i]));
				memcpy(ptr.pvar->label,Var_label[i],strlen((char *)Var_label[i]));
				//vars[i].value = 0;
				ptr.pvar->auto_manual = 0 ;
				ptr.pvar->digital_analog = 1;
				ptr.pvar->range = MAX_INPUT_RANGE;
			}
			Get_AVS();
		}
	}
}

void read_point_info(void)
{
	// Find the partition map in the partition table
	const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "storage");
	assert(partition != NULL);
	//STR_flag_flash ptr_flash;
	//U16_T base_addr;
	U8_T loop,i;
	U8_T page;
	uint8_t  err = 0xff;
	uint8_t *tempbuf = NULL;

	for(loop = 0;loop < MAX_POINT_TYPE;loop++)
	{

		if(Flash_Position[loop].valid == 0)
			continue;

#if NEW_IO

		if(loop == OUT)
		{
			if(new_outputs != NULL)
			{
				tempbuf = new_outputs;
				Flash_Position[loop].len = max_outputs *sizeof(Str_out_point);
			}
		}
		else if(loop == IN)
		{
			if(new_inputs != NULL)
			{
				tempbuf = new_inputs;
				Flash_Position[loop].len = max_inputs *sizeof(Str_in_point);
			}
		}
		else if(loop == VAR)
		{
			if(new_vars != NULL)
			{
				tempbuf = new_vars;
				Flash_Position[loop].len = max_vars *sizeof(Str_variable_point);
			}
		}
		else
		{
			tempbuf = (uint8_t*)malloc(Flash_Position[loop].len);

		}
#else
		tempbuf = (uint8_t*)malloc(Flash_Position[loop].len);
#endif

		err = esp_partition_read(partition, Flash_Position[loop].addr, tempbuf, Flash_Position[loop].len);

		switch(loop)
		{

		case OUT:
#if !NEW_IO
			memcpy(&outputs,tempbuf,sizeof(Str_out_point) * MAX_OUTS);
#endif
			if((tempbuf[0] == 0x00 && tempbuf[1] == 0x00 && tempbuf[2] == 0x00) ||
				(tempbuf[0] == 0xff && tempbuf[1] == 0xff && tempbuf[2] == 0xff))
			{
				Initial_points(OUT);
			}

			break;
		case IN:
#if !NEW_IO
			memcpy(&inputs,tempbuf,sizeof(Str_in_point) * MAX_INS);
#endif
			if((tempbuf[0] == 0x00 && tempbuf[1] == 0x00 && tempbuf[2] == 0x00) ||
				(tempbuf[0] == 0xff && tempbuf[1] == 0xff && tempbuf[2] == 0xff))
			{
				Initial_points(IN);

			}

			break;
		case VAR:
#if !NEW_IO
			memcpy(&vars,tempbuf,sizeof(Str_variable_point) * MAX_VARS);
#endif
			// if initial status
			if((tempbuf[0] == 0x00 && tempbuf[1] == 0x00 && tempbuf[2] == 0x00) ||
					(tempbuf[0] == 0xff && tempbuf[1] == 0xff && tempbuf[2] == 0xff))
			{
				Initial_points(VAR);
			}
			break;

		case CON:
			memcpy(&controllers,tempbuf,sizeof(Str_controller_point) * MAX_CONS);
			break;
		case WRT:
			memcpy(&weekly_routines,tempbuf,sizeof(Str_weekly_routine_point) * MAX_WR);
			break;
		case AR:
			memcpy(&annual_routines,tempbuf,sizeof(Str_annual_routine_point) * MAX_AR);
			break;
		case PRG:
			memcpy(&programs,tempbuf,sizeof(Str_program_point) * MAX_PRGS);
			break;
		case TBL:
			memcpy(&custom_tab,tempbuf,sizeof(Str_table_point) * MAX_TBLS);
			break;
	/*	case TZ:
			memcpy(&totalizers,tempbuf,sizeof(Str_totalizer_point) * MAX_TOTALIZERS);
			break;	*/
		case AMON:
			memcpy(&monitors,tempbuf,sizeof(Str_monitor_point) * MAX_MONITORS);
			break;
		case GRP:
			memcpy(&control_groups,tempbuf,sizeof(Control_group_point) * MAX_GRPS);
			break;
/*			case ARRAY:
			memcpy(&arrays,&tempbuf,sizeof(Str_array_point) * MAX_ARRAYS);
			break; */
		case ALARMM:
			memcpy(&alarms,tempbuf,sizeof(Alarm_point) * MAX_ALARMS);
			break;
		/*case ALARM_SET:
			memcpy(&alarms_set,tempbuf,sizeof(Alarm_set_point) * MAX_ALARMS_SET);
			break;*/
		case PRG_CODE:
			memcpy(&prg_code,tempbuf,MAX_PRGS * MAX_CODE * CODE_ELEMENT);
			break;
		case UNIT:
			memcpy(&digi_units,tempbuf,sizeof(Units_element) * MAX_DIG_UNIT);
			break;
		case USER_NAME:
			memcpy(&passwords,tempbuf,sizeof(Password_point) * MAX_PASSW);
			break;
		case WR_TIME:
			memcpy(&wr_times,tempbuf,sizeof(Wr_one_day) * 9 * MAX_WR);

			break;
		case AR_DATA:
			memcpy(&ar_dates,tempbuf,46 * sizeof(S8_T) * MAX_AR);
			break;
		case SUB_DB:
			memcpy(&scan_db,tempbuf,sizeof(SCAN_DB) * SUB_NO);
			break;
	/*	case GRP_POINT:
			memcpy(&group_data,&tempbuf,sizeof(Str_grp_element) * 240);
			break;
		case ID_ROUTION:
			for(i = 0;i < 254;i++)
			{
				memcpy(&ID_Config[i],&tempbuf[i * STORE_ID_LEN],STORE_ID_LEN);
				ID_Config_Sche[i] = ID_Config[i].Str.schedule;
			}
			break;*/

		default:
			break;

			}


#if NEW_IO
		if((loop != OUT) && (loop != IN) && (loop != VAR)){

			free(tempbuf);
		}
#else
		free(tempbuf);
#endif

	}


}

#if 1//TRENDLOG

// SPI_FLASH_SEC_SIZE = 4k
#define 	MAX_TREND_PAGE 16	// max page is 16, 16 * 4k = 64k
uint16_t current_page;  //
uint16_t total_page;

esp_err_t save_trendlog(void)
{
	STR_flag_flash ptr_flash;
	uint8_t err=0xff;
	uint16_t loop;
	//  step 1: Ѱ���û�flash id
	const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_ANY, "storage");

	assert(partition != NULL);

//	sprintf(debug_array,"save_trendlog current_page = %d \r",current_page);
//	debug_info(debug_array);
	err = esp_partition_erase_range(partition, TRENDLOG_ADDR + current_page * SPI_FLASH_SEC_SIZE, SPI_FLASH_SEC_SIZE);
	if(err != 0)
	{//debug_info("erase error");
	 return err;//ESP_LOGI(TAG, "user  flash erase range ----%d",err);
	}
	//else
		//debug_info("erase ok");

	err = esp_partition_write(partition, TRENDLOG_ADDR + current_page * SPI_FLASH_SEC_SIZE,write_mon_point_buf_to_flash,SPI_FLASH_SEC_SIZE);

   if(err != 0)
   {//debug_info("flash write error");
	   return err ;
   }
   //else
	  // debug_info("flash write ok");
   current_page++;
   if(current_page >= MAX_TREND_PAGE)
	   current_page = 0;

	return ESP_OK;
}


esp_err_t read_trendlog(uint8_t page,uint8_t seg)
{
	// Find the partition map in the partition table
	const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "storage");
//	sprintf(debug_array,"read_trendlog page = %d, seg = %d\r",page,seg);
//	debug_info(debug_array);
	assert(partition != NULL);
	//STR_flag_flash ptr_flash;
	//U16_T base_addr;
	U8_T loop,i;
	uint8_t  err = 0xff;

	// 400(read packet length)
	// bacnet trasfer length is defined 400 by us
	// total 11 packets, 400 * 10 + 96 = 4096
	if(seg < 10)
		err = esp_partition_read(partition, TRENDLOG_ADDR + page * SPI_FLASH_SEC_SIZE + seg * 400, &read_mon_point_buf_from_flash, 400);
	else if(seg == 10)
		err = esp_partition_read(partition, TRENDLOG_ADDR + page * SPI_FLASH_SEC_SIZE + seg * 400, &read_mon_point_buf_from_flash, 96);
	else
		err = 1;

	//memcpy(&Test[0],&read_mon_point_buf_from_flash[0],sizeof(Str_mon_element));
	return err;

}


void TRENLOG_TEST(void)
{
	save_trendlog();
	Test[28] = read_trendlog(0,0);
	memcpy(&Test[0],&read_mon_point_buf_from_flash[Test[27]],sizeof(Str_mon_element));
}

#endif
