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

uint8_t ChangeFlash;
uint16_t count_write_Flash;
uint8_t count_reboot = 0;
extern S16_T timezone;
void Get_AVS(void);

const uint8 Var_Description[12][21];
const uint8 Var_label[12][9];

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

	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	  ESP_ERROR_CHECK(nvs_flash_erase());
	  ret = nvs_flash_init();

	}
	// Open

	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;
	debug_info("read_default_from_flash nvs_open\n");

	uint32_t len = sizeof(STR_SSID);
	err = nvs_get_blob(my_handle, FLASH_SSID_INFO, &SSID_Info, &len);

	if(err == ESP_ERR_NVS_NOT_FOUND)
	{
		//init_ssid_info();
		debug_info("The value is not initialized yet!\n");
	}
	//if (err != ESP_OK) return err;

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
	if(err ==ESP_ERR_NVS_NOT_FOUND)
	{
		Modbus.mini_type = MINI_NANO;
		nvs_set_u8(my_handle, FLASH_MINI_TYPE, Modbus.mini_type);
	}

	nvs_get_u16(my_handle, FLASH_NETWORK_NUMBER, &Modbus.network_number);
	nvs_get_u16(my_handle, FLASH_TIME_ZONE, (uint16 *)&timezone);
	nvs_get_u16(my_handle, FLASH_DSL, &Daylight_Saving_Time);
	if(Modbus.network_number == 0)
	{
		Modbus.network_number = 0xffff;
		nvs_set_u16(my_handle, FLASH_NETWORK_NUMBER, Modbus.network_number);
	}

	err = nvs_get_u8(my_handle, FLASH_EN_USERNAME, &Modbus.en_username);
	if(err ==ESP_ERR_NVS_NOT_FOUND)
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

	err = nvs_get_blob(my_handle, FLASH_SSID_INFO, &SSID_Info, &len);

	// panel name
	err = nvs_get_blob(my_handle, FLASH_PANEL_NAME, &panelname, &len);

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
		switch(loop)
		{
	  case OUT:
			baseAddr = 0;
			len = sizeof(Str_out_point) * MAX_OUTS;
			break;
		case IN:
			baseAddr += len;
			len = sizeof(Str_in_point) * MAX_INS;
			break;
		case VAR:
			baseAddr += len;
			len = sizeof(Str_variable_point) * MAX_VARS;
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
		/*case ID_ROUTION:
			baseAddr += len;
			len = STORE_ID_LEN * 254;
			break;*/
		default:
	//		len = 0;
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
/*
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
*/

void Set_Object_Name(char * name)
{
	// store it to flash memory
	memcpy(panelname,name,strlen(name));
	save_block(FLASH_BLOCK2_PN);
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
	//debug_info(" flash start");
	// step 2: ����flash

	err = esp_partition_erase_range(partition, 0, partition->size);
	if(err!=0)
   {
	 return err;//ESP_LOGI(TAG, "user  flash erase range ----%d",err);
   }
	//debug_info(" erase ok");
	for(loop = 0;loop < MAX_POINT_TYPE;loop++)
	{
		//if(loop == point_type)
		{
			ptr_flash.table = loop;
			ptr_flash.len = Flash_Position[loop].len;
			uint8_t *tempbuf = (uint8_t*)malloc(ptr_flash.len);

			switch(loop)
			{
			case OUT:
				memcpy(tempbuf,&outputs,sizeof(Str_out_point) * MAX_OUTS);
				break;
			case IN:
				memcpy(tempbuf,&inputs,sizeof(Str_in_point) * MAX_INS);
				break;

			case VAR:
				memcpy(tempbuf,&vars,sizeof(Str_variable_point) * MAX_VARS);
				break;

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
	#endif
			default:
				break;

			}

		// step 3�������Ҫ������û�����
			err = esp_partition_write(partition, Flash_Position[loop].addr,tempbuf,Flash_Position[loop].len);
			//debug_info("write ...");
			free(tempbuf);
		   if(err != 0)
		   {//debug_info("user  flash write error");
			// ESP_LOGI(TAG, "user  flash write header ----1111%d",err);
			   return err ;
		   }

		}
	   //debug_info("user  flash write success");
	}
	return ESP_OK;
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
	uint8_t  err =0xff;
	uint8_t *tempbuf;

#if 1
	for(loop = 0;loop < MAX_POINT_TYPE;loop++)
	{
		/*ptr_flash.table = loop;

		ptr_flash.len = Flash_Position[loop].len;
		base_addr = Flash_Position[loop].addr;*/
		//debug_info("read start");
		    // 1: read ������
		//err = esp_partition_read(partition, 0, tempbuf, 500);
		tempbuf = (uint8_t*)malloc(Flash_Position[loop].len);

		err = esp_partition_read(partition, Flash_Position[loop].addr, tempbuf, Flash_Position[loop].len);
		if(err == 0)
			;//debug_info("read ok");


		switch(loop)
		{
			case OUT:
				memcpy(&outputs,tempbuf,sizeof(Str_out_point) * MAX_OUTS);
				if((tempbuf[0] == 0x00 && tempbuf[1] == 0x00 && tempbuf[2] == 0x00) ||
					(tempbuf[0] == 0xff && tempbuf[1] == 0xff && tempbuf[2] == 0xff))
				{
					if(Modbus.mini_type == PROJECT_FAN_MODULE)
					{
						memcpy(outputs[0].description,"FAN AO",strlen("FAN AO"));
						memcpy(outputs[1].description,"FAN PWM",strlen("FAN PWM"));
						if(outputs[0].range == 0)
						{
							outputs[0].switch_status = 1;
							outputs[0].auto_manual = 1;
							outputs[0].digital_analog = 1;
							outputs[0].range = 4;
							memcpy(outputs[0].description,"FAN PWM 1",strlen("FAN PWM 1"));
							memcpy(outputs[0].label,"FANOUT1",strlen("FANOUT1"));
						}
						if(outputs[1].range == 0)
						{
							outputs[1].switch_status = 1;
							outputs[1].auto_manual = 1;
							outputs[1].digital_analog = 1;
							outputs[1].range = 4;
							memcpy(outputs[1].description,"FAN PWM 2",strlen("FAN PWM 2"));
							memcpy(outputs[1].label,"FANOUT2",strlen("FANOUT2"));
						}
					}
					if(Modbus.mini_type == PROJECT_TRANSDUCER)
					{
						memcpy(outputs[0].description,"TEMP OUTPUT",strlen("TEMP OUTPUT"));
						memcpy(outputs[1].description,"HUMI OUTPUT",strlen("HUMI OUTPUT"));
						memcpy(outputs[2].description,"CO2",strlen("CO2"));
						if(outputs[0].range == 0)
						{
							outputs[0].switch_status = 1;
							outputs[0].auto_manual = 1;
							outputs[0].digital_analog = 1;
							outputs[0].range = 4;
							memcpy(outputs[0].description,"TEMP OUTPUT",strlen("TEMP OUTPUT"));
							memcpy(outputs[0].label,"TEMPOUT",strlen("TEMPOUT"));
						}
						if(outputs[1].range == 0)
						{
							outputs[1].switch_status = 1;
							outputs[1].auto_manual = 1;
							outputs[1].digital_analog = 1;
							outputs[1].range = 4;
							memcpy(outputs[1].description,"HUMI OUTPUT",strlen("HUMI OUTPUT"));
							memcpy(outputs[1].label,"HUMIOUT",strlen("HUMIOUT"));
						}
						if(outputs[2].range == 0)
						{
							outputs[2].switch_status = 1;
							outputs[2].auto_manual = 1;
							outputs[2].digital_analog = 1;
							outputs[2].range = 4;
							memcpy(outputs[2].description,"CO2 OUTPUT",strlen("CO2 OUTPUT"));
							memcpy(outputs[2].label,"CO2OUT",strlen("CO2OUT"));
						}
					}
				}
				break;
			case IN:
				memcpy(&inputs,tempbuf,sizeof(Str_in_point) * MAX_INS);
				if((tempbuf[0] == 0x00 && tempbuf[1] == 0x00 && tempbuf[2] == 0x00) ||
					(tempbuf[0] == 0xff && tempbuf[1] == 0xff && tempbuf[2] == 0xff))
				{
					if(Modbus.mini_type == PROJECT_FAN_MODULE)
					{
						memcpy(inputs[0].description,"TEMP ON BOARD",strlen("TEMP ON BOARD"));
						memcpy(inputs[1].description,"HUMIDITY",strlen("HUMIDITY"));
						memcpy(inputs[2].description,"TEMP REMOTE",strlen("TEMP REMOTE"));
						memcpy(inputs[3].description,"FAN STATUS",strlen("FAN STATUS"));
						memcpy(inputs[4].description,"FAN SPEED",strlen("FAN SPEED"));
						memcpy(inputs[5].description,"THERMEL TEMP",strlen("THERMEL TEMP"));
						memcpy(inputs[0].label,"TEMP1",strlen("TEMP1"));
						memcpy(inputs[1].label,"HUM",strlen("HUM"));
						memcpy(inputs[2].label,"TEMP2",strlen("TEMP2"));
						memcpy(inputs[3].label,"FANSTAT",strlen("FANSTAT"));
						memcpy(inputs[4].label,"FANSPD",strlen("FANSPD"));
						memcpy(inputs[5].label,"TEMP3",strlen("TEMP3"));

						if(inputs[0].range == 0)
						{
							inputs[0].digital_analog = 1;
							inputs[0].range = 3;
							memcpy(inputs[0].description,"TEMP ON BOARD",strlen("TEMP ON BOARD"));
						}
						if(inputs[1].range == 0)
						{
							inputs[1].digital_analog = 1;
							inputs[1].range = 27;
							memcpy(inputs[1].description,"HUMIDITY",strlen("HUMIDITY"));
						}
						if(inputs[2].range == 0)
						{
							inputs[2].digital_analog = 1;
							inputs[2].range = 3;
							memcpy(inputs[2].description,"TEMP REMOTE",strlen("TEMP REMOTE"));
						}
						if(inputs[3].range == 0)
						{
							inputs[3].digital_analog = 1;
							inputs[3].range = 19;
							memcpy(inputs[3].description,"FAN STATUS",strlen("FAN STATUS"));
						}
						if(inputs[4].range == 0)
						{
							inputs[4].digital_analog = 1;
							inputs[4].range = 26;
							memcpy(inputs[4].description,"FAN SPEED",strlen("FAN SPEED"));
						}
						if(inputs[5].range == 0)
						{
							inputs[5].digital_analog = 1;
							inputs[5].range = 3;
							memcpy(inputs[5].description,"THERMEL TEMP",strlen("THERMEL TEMP"));
						}
					}

					if(Modbus.mini_type == PROJECT_TRANSDUCER)
					{
						memcpy(inputs[0].description,"TEMP ON BOARD",strlen("TEMP ON BOARD"));
						memcpy(inputs[1].description,"HUMIDITY",strlen("HUMIDITY"));
						memcpy(inputs[2].description,"CO2",strlen("CO2"));
						memcpy(inputs[0].label,"TEMP1",strlen("TEMP1"));
						memcpy(inputs[1].label,"HUM",strlen("HUM"));
						memcpy(inputs[2].label,"CO2",strlen("CO2"));

						if(inputs[0].range == 0)
						{
							inputs[0].digital_analog = 1;
							inputs[0].range = 3;
							memcpy(inputs[0].description,"TEMP ON BOARD",strlen("TEMP ON BOARD"));
						}
						if(inputs[1].range == 0)
						{
							inputs[1].digital_analog = 1;
							inputs[1].range = 27;
							memcpy(inputs[1].description,"HUMIDITY",strlen("HUMIDITY"));
						}
						if(inputs[2].range == 0)
						{
							inputs[2].digital_analog = 1;
							inputs[2].range = 28;
							memcpy(inputs[2].description,"CO2",strlen("CO2"));
						}
					}

					if(Modbus.mini_type == PROJECT_AIRLAB)
					{
						memcpy(inputs[0].description,"TEMPERATURE",strlen("TEMPERATURE"));
						memcpy(inputs[1].description,"HUMIDITY",strlen("HUMIDITY"));
						memcpy(inputs[2].description,"CO2 ",strlen("CO2 "));
						memcpy(inputs[3].description,"TVOC",strlen("TVOC"));
						memcpy(inputs[4].description,"PM1.0DEN",strlen("PM1.0DEN"));
						memcpy(inputs[5].description,"PM2.5DEN",strlen("PM2.5DEN"));
						memcpy(inputs[6].description,"PM4.0DEN",strlen("PM4.0DEN"));
						memcpy(inputs[7].description,"PM10DEN",strlen("PM10DEN"));
						memcpy(inputs[8].description,"PM0.5C",strlen("PM0.5C"));
						memcpy(inputs[9].description,"PM1.0C",strlen("PM1.0C"));
						memcpy(inputs[10].description,"PM2.5C",strlen("PM2.5C"));
						memcpy(inputs[11].description,"PM4.0C",strlen("PM4.0C"));
						memcpy(inputs[12].description,"PM10C",strlen("PM10C"));
						memcpy(inputs[13].description,"P_size",strlen("P_size"));
						memcpy(inputs[14].description,"SOUND",strlen("SOUND"));
						memcpy(inputs[15].description,"LIGHT",strlen("LIGHT"));
						memcpy(inputs[16].description,"OCCUPIED SENSOR",strlen("OCCUPIED SENSOR"));

						memcpy(inputs[0].label,"TEMP",strlen("TEMP"));
						memcpy(inputs[1].label,"HUM",strlen("HUM"));
						memcpy(inputs[2].label,"CO2",strlen("CO2"));
						memcpy(inputs[3].label,"TVOC",strlen("TVOC"));
						memcpy(inputs[4].label,"PM1.0DEN",strlen("PM1.0DEN"));
						memcpy(inputs[5].label,"PM2.5DEN",strlen("PM2.5DEN"));
						memcpy(inputs[6].label,"PM4.0DEN",strlen("PM1.0DEN"));
						memcpy(inputs[7].label,"PM10DEN",strlen("PM10DEN"));
						memcpy(inputs[8].label,"PM0.5C",strlen("PM0.5C"));
						memcpy(inputs[9].label,"PM1.0C",strlen("PM1.0C"));
						memcpy(inputs[10].label,"PM2.5C",strlen("PM2.5C"));
						memcpy(inputs[11].label,"PM4.0C",strlen("PM4.0C"));
						memcpy(inputs[12].label,"PM10C",strlen("PM10C"));
						memcpy(inputs[13].label,"P_size",strlen("P_size"));
						memcpy(inputs[14].label,"SOUND",strlen("SOUND"));
						memcpy(inputs[15].label,"LIGHT",strlen("LIGHT"));
						memcpy(inputs[16].label,"OCC",strlen("OCC"));

						for(i = 0;i < 18;i++)
							inputs[i].digital_analog = 1;

						inputs[0].range = R10K_40_120DegC;
						inputs[1].range = Humidty;
						inputs[2].range = CO2_PPM;
						inputs[3].range = TVOC_PPB;
						inputs[4].range = UG_M3;
						inputs[5].range = UG_M3;
						inputs[6].range = UG_M3;
						inputs[7].range = UG_M3;
						inputs[8].range = NUM_CM3;
						inputs[9].range = NUM_CM3;
						inputs[10].range = NUM_CM3;
						inputs[11].range = NUM_CM3;
						inputs[12].range = NUM_CM3;
						inputs[14].range = DB;
						inputs[15].range = LUX;
						inputs[16].range = 0;
					}
				}
				break;
			case VAR:
				memcpy(&vars,tempbuf,sizeof(Str_variable_point) * MAX_VARS);
				// if initial status
				if((tempbuf[0] == 0x00 && tempbuf[1] == 0x00 && tempbuf[2] == 0x00) ||
						(tempbuf[0] == 0xff && tempbuf[1] == 0xff && tempbuf[2] == 0xff))
				{
					if(Modbus.mini_type == PROJECT_AIRLAB)
					{
						for(i = 0; i < 13; i++ )
						{
							memcpy(vars[i].description,Var_Description[i],strlen((char *)Var_Description[i]));
							memcpy(vars[i].label,Var_label[i],strlen((char *)Var_label[i]));
							//vars[i].value = 0;
							vars[i].auto_manual = 0 ;
							vars[i].digital_analog = 1;
							vars[i].range = MAX_INPUT_RANGE;
						}
						Get_AVS();
					}
				}
				break;
#if 1
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
#endif
			default:
				break;

			}
		free(tempbuf);
	}

	/*for (i = 0; i < MAX_INS; i++)
	{
		if(inputs[i].range == 0)
			inputs[i].digital_analog = 1;
	}
	for (i = 0; i < MAX_OUTS; i++)
	{
		if(outputs[i].range == 0)
			outputs[i].digital_analog = 1;
	}
	for (i = 0; i < MAX_VARS; i++)
	{
		if(vars[i].range == 0)
			vars[i].digital_analog = 1;
	}*/
#endif

//	Flash_Read_Code();

//	Flash_Read_Other();

}


#if 0
esp_err_t save_blob_info(const char* key, const void* pValue, size_t length)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;

	err = nvs_set_blob(my_handle, key, pValue, length);
	if (err != ESP_OK) return err;

	err = nvs_commit(my_handle);
	if (err != ESP_OK) return err;

	nvs_close(my_handle);
	return ESP_OK;
}

esp_err_t read_blob_info(const char* key, const void* pValue, size_t length)
{
	nvs_handle_t my_handle;
	esp_err_t err;

	// Open
	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
	if (err != ESP_OK) return err;

	err = nvs_get_blob(my_handle, key, pValue, &length);
	if (err != ESP_OK) return err;

	nvs_close(my_handle);
	return ESP_OK;
}
#endif
