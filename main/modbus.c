#include <stdio.h>
#include "esp_err.h"

#include "mbcontroller.h"
#include "deviceparams.h"       // for device parameters structures
#include "esp_log.h"            // for log_write
#include "driver/gpio.h"
#include "modbus.h"
#include "i2c_task.h"
#include "store.h"

#define EEPROM_VERSION	  105

#define MB_PORT_NUM     (1)           // Number of UART port used for Modbus connection
#define MB_DEV_ADDR     (1)           // The address of device in Modbus network
#define MB_DEV_SPEED    (115200)      // The communication speed of the UART
// Defines below are used to define register start address for each type of Modbus registers
#define MB_REG_DISCRETE_INPUT_START         (0x0000)
#define MB_REG_INPUT_START                  (0x0000)
#define MB_REG_HOLDING_START                (0x0000)
#define MB_REG_COILS_START                  (0x0000)

#define MB_PAR_INFO_GET_TOUT                (10) // Timeout for get parameter info
#define MB_CHAN_DATA_MAX_VAL                (10)
#define MB_CHAN_DATA_OFFSET                 (0.01f)
#define MB_READ_MASK                        (MB_EVENT_INPUT_REG_RD \
                                                | MB_EVENT_HOLDING_REG_RD \
                                                | MB_EVENT_DISCRETE_RD \
                                                | MB_EVENT_COILS_RD)
#define MB_WRITE_MASK                       (MB_EVENT_HOLDING_REG_WR \
                                                | MB_EVENT_COILS_WR)
#define MB_READ_WRITE_MASK                  (MB_READ_MASK | MB_WRITE_MASK)

#define MB_BUF_SIZE (512)

#define GPIO_MODBUS_EN_PIN		GPIO_NUM_13
#define MODBUS_EN_PIN_SEL	(1ULL<<GPIO_MODBUS_EN_PIN)
//static const char *TAG = "MODBUS_SLAVE_APP";
//uint8_t uart_sendB[512];
const int uart_num = UART_NUM_0;

// Set register values into known state
static void setup_reg_data()
{
    // Define initial state of parameters
    discrete_reg_params.discrete_input1 = 1;
    discrete_reg_params.discrete_input3 = 1;
    discrete_reg_params.discrete_input5 = 1;
    discrete_reg_params.discrete_input7 = 1;

    //holding_reg_params.serial_number_lo = 25;
    //holding_reg_params.serial_number_hi = 0;
    holding_reg_params.version_number_lo = 26;
    holding_reg_params.version_number_hi = 0;
    //holding_reg_params.modbus_address = MB_DEV_ADDR;
    if(holding_reg_params.which_project == PROJECT_FAN_MODULE)
    	holding_reg_params.product_model = 97;//62;//
    else
    	holding_reg_params.product_model = 62;//74;//74;//
    holding_reg_params.hardware_version = 2;
    holding_reg_params.readyToUpdate = 0;

    coil_reg_params.coil0 = 1;
    coil_reg_params.coil2 = 1;
    coil_reg_params.coil4 = 1;
    coil_reg_params.coil6 = 1;
    coil_reg_params.coil7 = 1;

    input_reg_params.data_chan0 = 1.34;
    input_reg_params.data_chan1 = 2.56;
    input_reg_params.data_chan2 = 3.78;
    input_reg_params.data_chan3 = 4.90;
}


//void* mbc_slave_handler = NULL;
//mb_communication_info_t comm_info;
//mb_register_area_descriptor_t reg_area; // Modbus register area descriptor structure
//mb_communication_info_t tcp_info;

void modbus_init(void)
{
	int baud;
	switch(holding_reg_params.baud_rate)
	{
	case 0:
		baud = 9600;
		break;
	case 1:
		baud = 19200;
		break;
	case 2:
		baud = 38400;
		break;
	case 3:
		baud = 57600;
		break;
	case 4:
		baud = 115200;
		break;
	default:
		baud = 115200;
		break;
	}
//	mb_communication_info_t tcp_info; // Modbus communication parameters

	    uart_config_t uart_config = {
	        .baud_rate = baud,//115200,//
	        .data_bits = UART_DATA_8_BITS,
	        .parity = UART_PARITY_DISABLE,
	        .stop_bits = UART_STOP_BITS_1,
	        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	       // .rx_flow_ctrl_thresh = 122,
	    };
// Configure UART parameters
	uart_param_config(uart_num, &uart_config);
	uart_set_pin(uart_num, GPIO_NUM_1, GPIO_NUM_3, GPIO_MODBUS_EN_PIN, UART_PIN_NO_CHANGE);
	//uart_set_pin(uart_num, GPIO_NUM_1, GPIO_NUM_3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	uart_driver_install(uart_num, MB_BUF_SIZE * 2, 0, 0, NULL, 0);

	/*gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask = MODBUS_EN_PIN_SEL;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);
	gpio_set_level(GPIO_MODBUS_EN_PIN,0);*/
// Set RS485 half duplex mode
	uart_set_mode(uart_num, UART_MODE_RS485_HALF_DUPLEX);
}

void stm32_uart_init(void)
{
	//if(holding_reg_params.which_project != PROJECT_FAN_MODULE)
	{
		uart_config_t uart_config = {
					.baud_rate = 19200,
					.data_bits = UART_DATA_8_BITS,
					.parity = UART_PARITY_DISABLE,
					.stop_bits = UART_STOP_BITS_1,
					.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
				};
		// Configure UART parameters
		uart_param_config(UART_NUM_1, &uart_config);
		uart_set_pin(UART_NUM_1, GPIO_NUM_33, GPIO_NUM_32, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
		uart_driver_install(UART_NUM_1, MB_BUF_SIZE * 2, 0, 0, NULL, 0);
		//uart_set_mode(UART_NUM_1, UART_MODE_UART);
	}
}

bool checkdata(uint8_t* data)
{
	uint16_t crc_val ;

	if((data[0] != 255) && (data[0] != holding_reg_params.modbus_address) && (data[0] != 0))
		return false;

	if(data[1] != READ_VARIABLES && data[1] != WRITE_VARIABLES && data[1] != MULTIPLE_WRITE_VARIABLES && data[1] != CHECKONLINE)
		return false;

	if(data[1] == CHECKONLINE)
	{
		crc_val = crc16(data,4);
		if(crc_val != data[4]*256 + data[5])
		{
			return false;
		}
	}
	return true;
}

void modbus_task(void *arg)
{
	uint8_t testCmd[8] = {0xff,0x03,0x00,0x00,0x00,0x64,0x51,0xff};
	uint8_t i;
	//mb_param_info_t reg_info; // keeps the Modbus registers access information
	printf("MODBUS_TASK&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\r\n");
	// Configure a temporary buffer for the incoming data
	//uint8_t *data = (uint8_t *) malloc(MB_BUF_SIZE);
	//char* test_str = "This is a test string.\n";
//	char prefix[] = "RS485 Received: [";
	//modbus_init();
	setup_reg_data();
	uint8_t *uart_rsv = (uint8_t*)malloc(MB_BUF_SIZE);
	uint8_t *stm32_uart_rsv = (uint8_t*)malloc(64);
	if(holding_reg_params.which_project != PROJECT_FAN_MODULE)
	{
		//holding_reg_params.testBuf[6] = 0x55;
		memset(stm32_uart_rsv, 030,64);
	}
	for(i=0;i<5;i++)
	{
		init_crc16();
		responseCmd(SERIAL, testCmd, 8);
		vTaskDelay(300 / portTICK_RATE_MS);
	}

	while (1) {
		int len = uart_read_bytes(uart_num, uart_rsv, MB_BUF_SIZE, 20 / portTICK_RATE_MS);

		if(len>0)
		{
			if(checkdata(uart_rsv))
			{
				holding_reg_params.led_rx485_rx = 2;
				init_crc16();
			//gpio_set_level(GPIO_MODBUS_EN_PIN,1);
			//		vTaskDelay(5 / portTICK_RATE_MS);
				responseCmd(SERIAL, uart_rsv, len);
			}
			//vTaskDelay(20 / portTICK_RATE_MS);
			//gpio_set_level(GPIO_MODBUS_EN_PIN,0);
			//uart_write_bytes(uart_num, "\r\n", 2);
			//uart_write_bytes(uart_num, prefix, (sizeof(prefix) - 1));
		}

		if(holding_reg_params.which_project != PROJECT_FAN_MODULE)
		{
			uart_read_bytes(UART_NUM_1, stm32_uart_rsv, 64, 20 / portTICK_RATE_MS);
			//holding_reg_params.testBuf[7] = uart_write_bytes(UART_NUM_1, (const char *)holding_reg_params.testBuf, 20);
			//for(i=0;i<5;i++){
			//	holding_reg_params.testBuf[i] = stm32_uart_rsv[i];
			//}
			g_sensors.occ = stm32_uart_rsv[0];
			g_sensors.sound = BUILD_UINT16(stm32_uart_rsv[2],stm32_uart_rsv[1]);//BUILD_UINT32(stm32_uart_rsv[1],stm32_uart_rsv[2],stm32_uart_rsv[3],stm32_uart_rsv[4]);
			if(!inputs[14].calibration_sign)
				g_sensors.sound += (inputs[14].calibration_hi * 256 + inputs[14].calibration_lo)/10;
			else
				g_sensors.sound += -(inputs[14].calibration_hi * 256 + inputs[14].calibration_lo)/10;
		}
		//uart_write_bytes(uart_num, (const char *)uart_rsv, len);
		//vTaskDelay(1000 / portTICK_RATE_MS);
#if 0
		// The cycle below will be terminated when parameter holdingRegParams.dataChan0
		// incremented each access cycle reaches the CHAN_DATA_MAX_VAL value.
		//for(;holding_reg_params.data_chan0 < MB_CHAN_DATA_MAX_VAL;) {
			// Check for read/write events of Modbus master for certain events
		mb_event_group_t event = mbc_slave_check_event(MB_READ_WRITE_MASK);
		const char* rw_str = (event & MB_READ_MASK) ? "READ" : "WRITE";
		// Filter events and process them accordingly
		if(event & (MB_EVENT_HOLDING_REG_WR | MB_EVENT_HOLDING_REG_RD)) {
			// Get parameter information from parameter queue
			ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
			printf("HOLDING %s: time_stamp(us):%u, mb_addr:%u, type:%u, st_address:0x%.4x, size:%u\r\n",
					rw_str,
					(uint32_t)reg_info.time_stamp,
					(uint32_t)reg_info.mb_offset,
					(uint32_t)reg_info.type,
					(uint32_t)reg_info.address,
					(uint32_t)reg_info.size);
			//if (reg_info.address == (uint8_t*)&holding_reg_params.data_chan0)
			{
			//	holding_reg_params.data_chan0 += MB_CHAN_DATA_OFFSET;
			}
		} else if (event & MB_EVENT_INPUT_REG_RD) {
			ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
			printf("INPUT READ: time_stamp(us):%u, mb_addr:%u, type:%u, st_address:0x%.4x, size:%u\r\n",
					(uint32_t)reg_info.time_stamp,
					(uint32_t)reg_info.mb_offset,
					(uint32_t)reg_info.type,
					(uint32_t)reg_info.address,
					(uint32_t)reg_info.size);
		} else if (event & MB_EVENT_DISCRETE_RD) {
			ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
			printf("DISCRETE READ: time_stamp(us):%u, mb_addr:%u, type:%u, st_address:0x%.4x, size:%u\r\n",
								(uint32_t)reg_info.time_stamp,
								(uint32_t)reg_info.mb_offset,
								(uint32_t)reg_info.type,
								(uint32_t)reg_info.address,
								(uint32_t)reg_info.size);
		} else if (event & (MB_EVENT_COILS_RD | MB_EVENT_COILS_WR)) {
			ESP_ERROR_CHECK(mbc_slave_get_param_info(&reg_info, MB_PAR_INFO_GET_TOUT));
			printf("COILS %s: time_stamp(us):%u, mb_addr:%u, type:%u, st_address:0x%.4x, size:%u\r\n",
								rw_str,
								(uint32_t)reg_info.time_stamp,
								(uint32_t)reg_info.mb_offset,
								(uint32_t)reg_info.type,
								(uint32_t)reg_info.address,
								(uint32_t)reg_info.size);
		}

		// Read data from the UART
		//int len = uart_read_bytes(MB_PORT_NUM, data, MB_BUF_SIZE, 20 / portTICK_RATE_MS);
		// Write data back to the UART
		//uart_write_bytes(MB_PORT_NUM, (const char *) data, len);
		/*if(len>0)
		{
			printf("get data from UART\r\n");
		}
		else
			printf("nothing\r\n");*/
		//printf("test uart........\r\n");
		//uart_write_bytes(MB_PORT_NUM, (const char*)test_str, strlen(test_str));
		//vTaskDelay(5000 / portTICK_RATE_MS);
#endif
	}
}
/*
void InitCRC16(void)
{
	CRClo = 0xFF;
	CRChi = 0xFF;
}

void serial_restart(void)
{
	TXEN = RECEIVE;
	rece_count = 0;
	dealwithTag = 0;

}


uint8_t modbus_wifi_buf[500];
uint16_t modbus_wifi_len;
uint16_t rece_size = 0 ;
uint8_t reg_num;
uint8 bacnet_to_modbus[300];

static void responseData(uint8_t  *bufadd, uint8_t uartsel, uint8_t type)
{
	uint8_t num, i, temp1, temp2;
	uint16_t send_cout = 0 ;
	uint16_t address;
	uint8_t headlen = 0;
	uint16_t TransID;
//	u16 address_temp ;
	//if(USART_RX_BUFB[1] == WRITE_VARIABLES)
	if(type == WIFI)
	{
		headlen = UIP_HEAD;
	}
	else
		headlen = 0;

	if(*(bufadd + 1 + headlen) == WRITE_VARIABLES)
	{
		if(type == WIFI)
		{ // for wifi

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

		}
		else
		{
			for(i = 0; i < rece_size; i++)
				uart_send[send_cout++] = *(bufadd+i);

			if(type == BAC_TO_MODBUS)
			{
				//memcpy(&bacnet_to_modbus,&uart_send[3],send_cout);
//				Test[3]++;
				memcpy(&bacnet_to_modbus,&uart_send[3],reg_num*2);
			}
			else
				USART_SendDataString(send_cout);
		}

		return;

	}
	else if(*(bufadd + 1 + headlen) == MULTIPLE_WRITE_VARIABLES)
	{
		for(i = 0; i < 6; i++)
		{
			 if(uartsel == 0)
					uart_send[send_cout++] = *(bufadd+i) ;
			 else//zigbee
			 {
				//for(i = 0; i < rece_sizeB; i++)
					uart_sendB[UIP_HEAD + send_cout++] = *(bufadd + i + headlen);

			 }
			crc16_byte(*(bufadd+i));
		}

		uart_sendB[0] = *bufadd;//0;			//	TransID
		uart_sendB[1] = *(bufadd + 1);//TransID++;
		uart_sendB[2] = 0;			//	ProtoID
		uart_sendB[3] = 0;
		uart_sendB[4] = 0;	//	Len
		uart_sendB[5] = 6;
		if(uartsel == 0)
    {
			if(type == BAC_TO_MODBUS)
			{
//				memcpy(&bacnet_to_modbus,&uart_send,send_cout);
//				Test[2]++;
				memcpy(&bacnet_to_modbus,&uart_send[3],reg_num*2);
			}
			else
				USART_SendDataString(send_cout);
		}
		else
		{
			//USART_SendDataStringB(send_cout);
			//ESP8266_SendString (DISABLE, (uint8_t *)&uart_sendB, send_cout+UIP_HEAD,cStr[7] - '0' );
				memcpy(modbus_wifi_buf,uart_sendB,UIP_HEAD + send_cout);
				modbus_wifi_len = UIP_HEAD + send_cout;
		}
	}

	else if(*(bufadd + 1 + headlen) == READ_VARIABLES)
	{
			num = *(bufadd+5 + headlen);
			if(uartsel == 0)
			{
			uart_send[send_cout++] = *(bufadd) ;
			uart_send[send_cout++] = *(bufadd+1) ;
			uart_send[send_cout++] = (*(bufadd+5)<<1);
			}
			else//zigbee
			{
			//#ifdef TSTAT_WIFI
			uart_sendB[UIP_HEAD + send_cout++] = *(bufadd + headlen);
			uart_sendB[UIP_HEAD + send_cout++] = *(bufadd+1 + headlen) ;
			uart_sendB[UIP_HEAD + send_cout++] = (*(bufadd +5 + headlen)<<1);
			//#endif //TSTAT_WIFI
			}
			crc16_byte(*(bufadd + headlen));
			crc16_byte(*(bufadd +1 + headlen));
			crc16_byte((*(bufadd + 5 + headlen)<<1));


			for(i = 0; i < num; i++)
				{
					address = (uint16_t)((*(bufadd+2 + headlen))<<8) + (*(bufadd+3 + headlen)) + i;


				if(address <= SERIALNUMBER_HIWORD + 1)
					{
					temp1 = 0 ;
					temp2 = SerialNumber(address) ;
					}


				else if(address == VERSION_NUMBER_LO)
					{
					temp1 = (EEPROM_VERSION >> 8) & 0xff;
					temp2 = EEPROM_VERSION & 0xff;
					}

				else if(address == VERSION_NUMBER_HI)
					{
					temp1 = 0;//(EEPROM_VERSION >> 8) & 0xff;
					temp2 = 0;//FirmwareVersion_HI;
					}

				else if(address == TSTAT_ADDRESS)
					{
					temp1 = 0; //;
					temp2 = 1;//laddress;
					}
				else if(address == PRODUCT_MODEL)
					{
					temp1 = 0 ;
					temp2 = 62;//ProductModel;

//					#ifndef TSTAT7_ARM
//					temp2 = PM_TSTAT8;
//					#else
//					temp2 = PM_TSTAT7_ARM;
//					#endif
					}
				else if(address == HARDWARE_REV)
					{
					temp1 = 0 ;
					temp2 = 1;//HardwareVersion ;

					}
	//			else if(address == PIC882VERSION)
	//				{
	//				temp1 = (co2_dataB >> 8) & 0xff ;
	//				temp2 = co2_dataB & 0xff;

	//				}
				else if(address == HARDWARE_INFORMATION)
					{
					#ifdef TSTAT_CO2
					temp1 = 0;//Hardware_Info_HI;
					temp2 = 7;//Hardware_Info_LO;
					#else
					temp1 = Hardware_Info_HI;
					temp2 = Hardware_Info_LO;
					#endif
					}
				else if(address == MODBUS_BACNET_SWITCH)
				{
					temp1 = 0;
					temp2 = protocol_select;
//				  if(modbus.com_config[0] == MODBUS)
//						temp2 = 10;
//					else if(modbus.com_config[0] == BAC_MSTP)
//						temp2 = 11;
				}

				else if(address == ISP_MODE_INDICATION)
					{
					temp1 = 0;
					temp2 = 0;
					}

#ifdef TSTAT_ZIGBEE

				else if(address == MODBUS_ZIGBEE_ID)
				{
					temp1 = (zigbee_id >> 8);
					temp2 =  zigbee_id & 0xff;
				}

				else if(address == MODBUS_ZIGBEE_INDEX)
					{
					temp1 = 0;
					temp2 = zigbee_index;
					}

//				else if((address >= MODBUS_ZIGBEE_INFO1) && (address <= MODBUS_ZIGBEE_INFO48))
//					{
//					temp1 = 0;
//					temp2 = zigbee_point_info[address - MODBUS_ZIGBEE_INFO1];
//					}
#endif


				else if((address >= IP_ADDR_1) && (address <= IP_ADDR_4))
				{
					temp1 = 0;
					temp2 = SSID_Info.ip_addr[address - IP_ADDR_1];
				}


        else if(address == MODBUS_DELTA_TEM1)
				{
					temp1 = 0 ;
					temp2 = Delta_tem_source1;
				}

				else if(address == IS_WIFI_EXIST)
				{
					temp1 = 0;
					temp2 = isWifiExist;
				}
        else if(address == MODBUS_DELTA_TEM2)
				{
					temp1 = 0 ;
					temp2 = Delta_tem_source2;
				}

        else if(address == MODBUS_DELTA_TEMPERATURE)
				{
					temp1 = (delta_temperature >> 8) & 0xff;
					temp2 = delta_temperature & 0xff;
				}

				else if(address == VOC_MINIPID2)
					{
						temp1 = (minipid2_value >> 8)& 0xff;
						temp2 =  minipid2_value & 0xff;
					}
				else if(address == VOC_MINIPID2_OFFSET)
					{
						temp1 = (minipid2_offset >> 8)& 0xff;
						temp2 =  minipid2_offset & 0xff;
					}


				else if(address == VOC_MINIPID2_ADC)
					{
						temp1 = (minipid2_adc >> 8)& 0xff;
						temp2 =  minipid2_adc & 0xff;
					}
				else if(address == COOLHEATMODE)
					{
					temp1 = 0 ;

					temp2 = get_bit(COOLING_MODE,0);
					if(EEP_COOL_TABLE1 == 0)
						temp2 = 0;
					if(get_bit(HEATING_MODE,0))
						{
						temp2 += 2;
						if(EEP_HEAT_TABLE1 == 0)
							temp2 = 0;
						}

					}
				else if(address == PID1_MODE_OPERATION)
					{
					temp1 = 0 ;
					temp2 = 0;
					if(current_mode_of_operation[0] >=4 && current_mode_of_operation[0] <=6)
						temp2 = current_mode_of_operation[0] + 10;
				// if cooling stages and heating satges exceed 3,we will use these values 14:  cooling4  17:heating4
					else if(current_mode_of_operation[0] >=7 && current_mode_of_operation[0] <=9)
						temp2 = current_mode_of_operation[0] - 3;

					else if(current_mode_of_operation[0] >=10 && current_mode_of_operation[0] <=12)
						temp2 = current_mode_of_operation[0] + 7;

					else
						temp2 = current_mode_of_operation[0];


					}

				else if(address == SEQUENCE)
					{
					temp1 = 0 ;
					temp2 = Sys_Sequence;

					}

				else if(address == DEGC_OR_F)
					{
					temp1 = 0 ;
					temp2 = EEP_DEGCorF;

					}

				else if(address == FAN_MODE)
					{
					temp1 = 0 ;
					temp2 = EEP_FanMode;

					}

				else if(address == POWERUP_MODE)
					{
					temp1 = 0 ;
					temp2 = EEP_PowerupOnOff;

					}
				else if(address == AUTO_ONLY)
					{
					temp1 = 0 ;
					temp2 = EEP_AutoOnly;
					}

	//			else if(address == FACTORY_DEFAULTS)
	//				{
	//				temp1 = 0 ;
	//				temp2 = 0;

	//				}

				else if(address == INFO_BYTE)
					{
					temp1 = 0 ;
					temp2 = info_byte;

					}

				else if(address == BAUDRATE)
					{
					temp1 = 0 ;
					temp2 = EEP_Baudrate;

					}

				else if(address == TSTAT_OVERRIDE_TIMER)
					{
					temp1 = 0 ;
					temp2 = EEP_OverrideTimer;

					}

				else if(address == OVERRIDE_TIMER_LEFT)
					{
					temp1 = 0 ;

					if(override_timer_time > 0)
						{
						temp2 = override_timer_time/61;
						temp2 += 1;
						}
					else
						temp2 = 0;
					//temp2 = EEP_OverrideTimer;

					}

				else if(address == HEAT_COOL_CONFIG)
					{
					temp1 = 0 ;
					temp2 = EEP_HeatCoolConfig;

					}


				else if(address == TIMER_ON)
					{
					temp1 = EEP_TimerOnHi;
					temp2 = EEP_TimerOn;

					}
				else if(address == TIMER_OFF)
					{
					temp1 = EEP_TimerOffHi;
					temp2 = EEP_TimerOff;

					}

				else if(address == TIMER_UNITS)
					{
					temp1 = 0;
					temp2 = EEP_TimerUnits;

					}

				else if(address == DEAD_MASTER)
					{
					temp1 = 0;
					temp2 = EEP_DeadMaster;

					}

	//			else if(address == SYSTEM_TIME_FORMAT)
	//				{
	//				temp1 = 0;
	//				temp2 = TimeFormat;

	//				}

				else if(address == FREE_COOL_CONFIG)
					{
					temp1 = 0;
					temp2 = FreeCoolConfig;

					}

				else if(address == RS485_MODE)
					{
					temp1 = 0;
	//				if(rs485_zigbee == RS485_ENABLE)
	//					temp2 = 0;
	//				else
					temp2 = RS485_Mode;

					}

				else if(address == TEMPRATURE_CHIP)
					{
					temp1 = (temperature >> 8) & 0xff;
					temp2 = temperature;

					}

				else if((address >= ANALOG1_RANGE) && (address <= ANALOG8_RANGE))
					{
					temp1 = 0;//
					temp2 = AI_Range(address - ANALOG1_RANGE);

					}

				else if(address == INTERNAL_THERMISTOR)
					{
					temp1 = (temperature_internal >> 8) & 0xff;
					temp2 = temperature_internal;

					}

				else if((address >= ANALOG_INPUT1_VALUE) && (address <= ANALOG_INPUT8_VALUE))
					{
	//				if(GetByteBit(&EEP_InputManuEnable,(address - ANALOG_INPUT1_VALUE)))
	//					{
	//					temp1 = ManualAI_HI(address - ANALOG_INPUT1_VALUE);
	//
	//					temp2 = ManualAI_LO(address - ANALOG_INPUT1_VALUE);
	//					}
	//				else
	//					{
						temp1 = (mul_analog_input[address - ANALOG_INPUT1_VALUE] >> 8) & 0xFF;

						temp2 = mul_analog_input[address - ANALOG_INPUT1_VALUE] & 0xFF;
	//					}


					}

				else if(address == EXTERNAL_SENSOR1)//co2 sensor
					{
					temp1 = (co2_data >> 8) & 0xff;//
					temp2 = co2_data & 0xff;

					}

				else if((address == EXTERNAL_SENSOR2)||(address == TSTAT_HUMIDITY_RH))//hum sensor
					{
					temp1 = (humidity >> 8) & 0xff;//
					temp2 = humidity & 0xff;


					}

				else if(address == INPUT_MANU_ENABLE)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_InputManuEnable;

					}

				else if(address == FILTER)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_Filter;

					}

				else if((address >= INPUT1_FILTER) && (address <= HUM_FILTER))//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = InputFilter(address - INPUT1_FILTER);

					}

//				else if(address == CO2_FILTER)//
//					{
//					temp1 = 0;//(>> 8) & 0xff;//
//					temp2 = CO2_Filter;

//					}

//				else if(address == HUM_FILTER)//
//					{
//					temp1 = 0;//(>> 8) & 0xff;//
//					temp2 = HUM_Filter;

//					}

				else if(address == CALIBRATION)//
					{
					temp1 = ((int16)Hum_T_calibration>> 8) & 0xff;//
					temp2 = (int16)Hum_T_calibration & 0xff;

					}

				else if(address == CALIBRATION_INTERNAL_THERMISTOR)//
					{
					temp1 = Calibration_Internal_HI;//(>> 8) & 0xff;//
					temp2 = Calibration_Internal_LO;

					}

				else if(address == CALIBRATION_ANALOG1)//
					{
					temp1 = (mul_analog_cal[0]>>8)&0xff;//
					temp2 = mul_analog_cal[0]&0xff;

					}

				else if(address == CALIBRATION_ANALOG2)//
					{
					temp1 = (mul_analog_cal[1]>>8)&0xff;//Calibration_AI1_HI;//(>> 8) & 0xff;//
					temp2 = mul_analog_cal[1]&0xff;//Calibration_AI1_LO;

					}

				else if(address == CALIBRATION_ANALOG3)//
					{
					temp1 = (mul_analog_cal[2]>>8)&0xff;//Calibration_AI1_HI;//(>> 8) & 0xff;//
					temp2 = mul_analog_cal[2]&0xff;//Calibration_AI1_LO;

					}


				else if(address == CALIBRATION_ANALOG4)//
					{
					temp1 = (mul_analog_cal[3]>>8)&0xff;//Calibration_AI1_HI;//(>> 8) & 0xff;//
					temp2 = mul_analog_cal[3]&0xff;//Calibration_AI1_LO;

					}
				else if(address == CALIBRATION_ANALOG5)//
					{
					temp1 = (mul_analog_cal[4]>>8)&0xff;//Calibration_AI1_HI;//(>> 8) & 0xff;//
					temp2 = mul_analog_cal[4]&0xff;//Calibration_AI1_LO;

					}
				else if(address == CALIBRATION_ANALOG6)//
					{
					temp1 = (mul_analog_cal[5]>>8)&0xff;//Calibration_AI1_HI;//(>> 8) & 0xff;//
					temp2 = mul_analog_cal[5]&0xff;//Calibration_AI1_LO;

					}
				else if(address == CALIBRATION_ANALOG7)//
					{
					temp1 = (mul_analog_cal[6]>>8)&0xff;//Calibration_AI1_HI;//(>> 8) & 0xff;//
					temp2 = mul_analog_cal[6]&0xff;//Calibration_AI1_LO;

					}
				else if(address == CALIBRATION_ANALOG8)//
					{
					temp1 = (mul_analog_cal[7]>>8)&0xff;//Calibration_AI1_HI;//(>> 8) & 0xff;//
					temp2 = mul_analog_cal[7]&0xff;//Calibration_AI1_LO;

					}

				else if(address == ANALOG1_FUNCTION)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = AI_Function1;

					}
				else if(address == ANALOG2_FUNCTION)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = AI_Function2;

					}
				else if(address == ANALOG3_FUNCTION)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = AI_Function3;

					}
				else if(address == ANALOG4_FUNCTION)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = AI_Function4;

					}
				else if(address == ANALOG5_FUNCTION)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = AI_Function5;

					}
				else if(address == ANALOG6_FUNCTION)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = AI_Function6;

					}
				else if(address == ANALOG7_FUNCTION)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = AI_Function7;

					}
				else if(address == ANALOG8_FUNCTION)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = AI_Function8;

					}

				else if(address == TABLE1_ZERO)//
					{
					temp1 = Table1_ZERO_HI;//(>> 8) & 0xff;//
					temp2 = Table1_ZERO_LO;

					}

				else if(address == TABLE1_HALFONE)//
					{
					temp1 = Table1_HalfOne_HI;//(>> 8) & 0xff;//
					temp2 = Table1_HalfOne_LO;

					}


				else if(address == TABLE1_ONE)//
					{
					temp1 = Table1_One_HI;//(>> 8) & 0xff;//
					temp2 = Table1_One_LO;

					}

				else if(address == TABLE1_HALFTWO)//
					{
					temp1 = Table1_HalfTwo_HI;//(>> 8) & 0xff;//
					temp2 = Table1_HalfTwo_LO;

					}

				else if(address == TABLE1_TWO)//
					{
					temp1 = Table1_Two_HI;//(>> 8) & 0xff;//
					temp2 = Table1_Two_LO;

					}

				else if(address == TABLE1_HALFTHREE)//
					{
					temp1 = Table1_HalfThree_HI;//(>> 8) & 0xff;//
					temp2 = Table1_HalfThree_LO;

					}

				else if(address == TABLE1_THREE)//
					{
					temp1 = Table1_Three_HI;//(>> 8) & 0xff;//
					temp2 = Table1_Three_LO;

					}

				else if(address == TABLE1_HALFFOUR)//
					{
					temp1 = Table1_HalfFour_HI;//(>> 8) & 0xff;//
					temp2 = Table1_HalfFour_LO;

					}

				else if(address == TABLE1_FOUR)//
					{
					temp1 = Table1_Four_HI;//(>> 8) & 0xff;//
					temp2 = Table1_Four_LO;

					}

				else if(address == TABLE1_HALFFIVE)//
					{
					temp1 = Table1_HalfFive_HI;//(>> 8) & 0xff;//
					temp2 = Table1_HalfFive_LO;

					}

				else if(address == TABLE1_FIVE)//
					{
					temp1 = Table1_Five_HI;//(>> 8) & 0xff;//
					temp2 = Table1_Five_LO;

					}

				else if(address == TABLE2_ZERO)//
					{
					temp1 = Table2_ZERO_HI;//(>> 8) & 0xff;//
					temp2 = Table2_ZERO_LO;

					}

				else if(address == TABLE2_HALFONE)//
					{
					temp1 = Table2_HalfOne_HI;//(>> 8) & 0xff;//
					temp2 = Table2_HalfOne_LO;

					}


				else if(address == TABLE2_ONE)//
					{
					temp1 = Table2_One_HI;//(>> 8) & 0xff;//
					temp2 = Table2_One_LO;

					}

				else if(address == TABLE2_HALFTWO)//
					{
					temp1 = Table2_HalfTwo_HI;//(>> 8) & 0xff;//
					temp2 = Table2_HalfTwo_LO;

					}

				else if(address == TABLE2_TWO)//
					{
					temp1 = Table2_Two_HI;//(>> 8) & 0xff;//
					temp2 = Table2_Two_LO;

					}

				else if(address == TABLE2_HALFTHREE)//
					{
					temp1 = Table2_HalfThree_HI;//(>> 8) & 0xff;//
					temp2 = Table2_HalfThree_LO;

					}

				else if(address == TABLE2_THREE)//
					{
					temp1 = Table2_Three_HI;//(>> 8) & 0xff;//
					temp2 = Table2_Three_LO;

					}

				else if(address == TABLE2_HALFFOUR)//
					{
					temp1 = Table2_HalfFour_HI;//(>> 8) & 0xff;//
					temp2 = Table2_HalfFour_LO;

					}

				else if(address == TABLE2_FOUR)//
					{
					temp1 = Table2_Four_HI;//(>> 8) & 0xff;//
					temp2 = Table2_Four_LO;

					}

				else if(address == TABLE2_HALFFIVE)//
					{
					temp1 = Table2_HalfFive_HI;//(>> 8) & 0xff;//
					temp2 = Table2_HalfFive_LO;

					}

				else if(address == TABLE2_FIVE)//
					{
					temp1 = Table2_Five_HI;//(>> 8) & 0xff;//
					temp2 = Table2_Five_LO;

					}


				else if(address == TSTAT_HUM_PIC_UPDATE)//TBD: modify this function as before // write current calibration table to PIC, which table decided by register 427
					{
					temp1 = 0;
					temp2 = 0;//Table2_Five_LO;

					}

				else if(address == TSTAT_HUM_CAL_NUM)//TBD: modify this function as before 	// calibration data number
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = 0;//Table2_Five_LO;

					}

				else if(address == TSTAT_HUM_CURRENT_DEFAULT)//TBD: modify this function as before // decide which table will run, default tabel or customer table   current=1 default=0
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = 0;//Table2_Five_LO;

					}

				else if((address >= MODE_OUTPUT1) && (address <= MODE_OUTPUT5))//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = OutputRange(address - MODE_OUTPUT1);

					}

				else if(address == OUTPUT1_SCALE)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_Output1Scale;

					}

				else if(address == OUTPUT2_SCALE)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_Output2Scale;

					}

				else if(address == TSTAT_DIGITAL_OUTPUT_STATUS)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = ~RELAY_1TO5.BYTE;

					}

				else if(address == TSTAT_COOLING_VALVE)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = 0;

					if(GetByteBit(&EEP_OutputManuEnable,5))
						{
						temp1 = ManualAO1_HI;//read_eeprom(MANUAL_COOL_VALVE_HI);
						temp2 = ManualAO1_LO;//read_eeprom(MANUAL_COOL_VALVE_LO);
						}
					else
						{
						temp1 = (valve[0] >> 8)&0xff;//((uint16)output_priority[MAX_BOS][7]>> 8) & 0xFF;
						temp2 = valve[0] & 0xff;//(uint16)output_priority[MAX_BOS][7] & 0xFF;
						}


					}

				else if(address == TSTAT_HEATING_VALVE)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = 0;

					if(GetByteBit(&EEP_OutputManuEnable,6))
						{
						temp1 = ManualAO2_HI;//read_eeprom(MANUAL_HEAT_VALVE_HI);
						temp2 = ManualAO2_LO;//read_eeprom(MANUAL_HEAT_VALVE_LO);
						}
					else
						{
						temp1 = (valve[1] >> 8)&0xff;//((uint16)output_priority[MAX_BOS][7]>> 8) & 0xFF;
						temp2 = valve[1] & 0xff;//(uint16)output_priority[MAX_BOS][7] & 0xFF;
						}


					}


	//	DAC_OFFSET,	           //					 // 0	255	DAC voltage offset

				else if((address >= OUTPUT1_DELAY_OFF_TO_ON) && (address <= OUTPUT7_DELAY_OFF_TO_ON))//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = Output_Delay_OfftoOn(address - OUTPUT1_DELAY_OFF_TO_ON);

					}

	//	DELAY1_OFF_TO_ON_TIME_CURRENT,//
	//	DELAY2_OFF_TO_ON_TIME_CURRENT,//
	//	DELAY3_OFF_TO_ON_TIME_CURRENT,//
	//	DELAY4_OFF_TO_ON_TIME_CURRENT,//
	//	DELAY5_OFF_TO_ON_TIME_CURRENT,//
	//	DELAY6_OFF_TO_ON_TIME_CURRENT,//future
	//	DELAY7_OFF_TO_ON_TIME_CURRENT,//future
				else if((address >= OUTPUT1_DELAY_ON_TO_OFF) && (address <= OUTPUT7_DELAY_ON_TO_OFF))//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = Output_Delay_OntoOff(address - OUTPUT1_DELAY_ON_TO_OFF);

					}

	//	DELAY1_ON_TO_OFF_TIME_CURRENT,//
	//	DELAY2_ON_TO_OFF_TIME_CURRENT,//
	//	DELAY3_ON_TO_OFF_TIME_CURRENT,//
	//	DELAY4_ON_TO_OFF_TIME_CURRENT,//
	//	DELAY5_ON_TO_OFF_TIME_CURRENT,//
	//	DELAY6_ON_TO_OFF_TIME_CURRENT,//future
	//	DELAY7_ON_TO_OFF_TIME_CURRENT,//future

				else if(address == CYCLING_DELAY)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = CyclingDelay;

					}

				else if(address == CHANGOVER_DELAY)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = ChangeOverDelay;

					}


				else if(address == VALVE_TRAVEL_TIME)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_ValveTravelTime;

					}


				else if(address == VALVE_PERCENT)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					if(GetByteBit(&EEP_OutputManuEnable,7))
							temp2 = ManualValvePercent;
					else
							temp2 = valve_target_percent;

					}

				else if(address == INTERLOCK_OUTPUT1)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_Interlock1;

					}

				else if(address == INTERLOCK_OUTPUT2)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_Interlock2;

					}
				else if(address == INTERLOCK_OUTPUT3)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_Interlock3;

					}
				else if(address == INTERLOCK_OUTPUT4)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_Interlock4;

					}
				else if(address == INTERLOCK_OUTPUT5)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_Interlock5;

					}
				else if(address == INTERLOCK_OUTPUT6)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_Interlock6;

					}
				else if(address == INTERLOCK_OUTPUT7)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_Interlock7;

					}

				else if(address == FREEZE_DELAY_ON)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_FreezeDelayOn;

					}
				else if(address == FREEZE_DELAY_OFF)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_FreezeDelayOff;

					}

				else if(address == OUTPUT_MANU_ENABLE)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_OutputManuEnable;

					}

				else if((address >= MANU_RELAY1) && (address <= MANU_RELAY5))//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					if((b.eeprom[EEP_RANGE_OUTPUT1 + address - MANU_RELAY1]) == OUTPUT_RANGE_PWM)
						temp2 = ManualRelay(address - MANU_RELAY1);
					else
						temp2 = GetByteBit(&ManualRelayAll,address - MANU_RELAY1);//ManualRelay(address - MANU_RELAY1);//GetByteBit(&ManualRelay, (address - MANU_RELAY1));
					}

				else if(address == MANUAL_AO1)//
					{
					temp1 = ManualAO1_HI;//(>> 8) & 0xff;//
					temp2 = ManualAO1_LO;

					}

				else if(address == MANUAL_AO2)//
					{
					temp1 = ManualAO2_HI;//(>> 8) & 0xff;//
					temp2 = ManualAO2_LO;

					}

				else if(address == DEADMASTER_AUTO_MANUAL)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = DeadMsater_AutoManual;

					}

				else if(address == DEADMASTER_OUTPUT_STATE)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = DeadMsater_OutputState;

					}

				else if(address == DEADMASTER_COOL_OUTPUT)//
					{
					temp1 = DeadMsater_CoolOutput_HI;//(>> 8) & 0xff;//
					temp2 = DeadMsater_CoolOutput_LO;

					}

				else if(address == DEADMASTER_HEAT_OUTPUT)//
					{
					temp1 = DeadMsater_HeatOutput_HI;//(>> 8) & 0xff;//
					temp2 = DeadMsater_HeatOutput_LO;

					}

				else if(address == OUTPUT1_FUNCTION)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_Output1Function;

					}

				else if(address == OUTPUT2_FUNCTION)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_Output2Function;

					}

				else if(address == OUTPUT3_FUNCTION)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_Output3Function;

					}

				else if(address == OUTPUT4_FUNCTION)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_Output4Function;

					}

				else if(address == OUTPUT5_FUNCTION)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_Output5Function;

					}

				else if(address == OUTPUT6_FUNCTION)
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = Out6_Function;
					}

				else if(address == OUTPUT7_FUNCTION)
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = Out7_Function;
					}


				else if(address == FAN_SPEED)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = fan_speed_user;//EEP_FanSpeed;

					}

				else if(address == PID_OUTPUT1)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = pid_ctrl_bit[0];//EEP_PidOutput1;

					}
				else if(address == PID_OUTPUT2)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = pid_ctrl_bit[1];//EEP_PidOutput2;

					}
				else if(address == PID_OUTPUT3)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = pid_ctrl_bit[2];
					//temp2 = EEP_PidOutput3;

					}
				else if(address == PID_OUTPUT4)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = pid_ctrl_bit[3];//EEP_PidOutput4;

					}
				else if(address == PID_OUTPUT5)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = pid_ctrl_bit[4];//EEP_PidOutput5;

					}
				else if(address == PID_OUTPUT6)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = pid_ctrl_bit[5];//EEP_PidOutput6;

					}
				else if(address == PID_OUTPUT7)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = pid_ctrl_bit[6];//EEP_PidOutput7;

					}

				else if((address >= UNIVERSAL_OUTPUT_BEGIN) && (address <= UNIVERSAL_OUTPUT_HEAT3))//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = PID2_Output_Table(address - UNIVERSAL_OUTPUT_BEGIN);

					}

				else if((address >= FAN0_OPER_TABLE_COAST) && (address <= FAN0_OPER_TABLE_HEAT3))//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = FanTable0(address - FAN0_OPER_TABLE_COAST);

					}

				else if((address >= FAN1_OPER_TABLE_COAST) && (address <= FAN1_OPER_TABLE_HEAT3))//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = FanTable1(address - FAN1_OPER_TABLE_COAST);

					}

				else if((address >= FAN2_OPER_TABLE_COAST) && (address <= FAN2_OPER_TABLE_HEAT3))//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = FanTable2(address - FAN2_OPER_TABLE_COAST);

					}

				else if((address >= FAN3_OPER_TABLE_COAST) && (address <= FAN3_OPER_TABLE_HEAT3))//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = FanTable3(address - FAN3_OPER_TABLE_COAST);
					}

				else if((address >= FANAUT_OPER_TABLE_COAST) && (address <= FANAUT_OPER_TABLE_HEAT3))//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = FanTableAuto(address - FANAUT_OPER_TABLE_COAST);

					}

				else if((address >= VALVE_OPER_TABLE_BEGIN) && (address <= VALVE_OPER_TABLE_HEAT3))//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = PID1_Valve_Table(address - VALVE_OPER_TABLE_BEGIN);

					}

				else if(address == HEAT_UNIVERSAL_TABLE)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_HEAT_TABLE2;

					}

				else if(address == COOL_UNIVERSAL_TABLE)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_COOL_TABLE2;

					}

				else if(address == HEAT_ORIGINAL_TABLE)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_HEAT_TABLE1;

					}

				else if(address == COOL_ORIGINAL_TABLE)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_COOL_TABLE1;

					}

				else if((address >= VALVE_OFF_TABLE_COAST) && (address <= VALVE_OFF_TABLE_HEAT3))//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = PID1_Valve_Off_Table(address - VALVE_OFF_TABLE_COAST);

					}

				else if(address == DEFAULT_SETPOINT)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_DefaultSetpoint;

					}

				else if(address == SETPOINT_CONTROL)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_SetpointControl;

					}
	//DAYSETPOINT_OPTION
	//MIDDLE_SETPOINT
				else if(address == DAY_SETPOINT)//
					{
				temp1 = EEP_DaySpHi;
				temp2 = EEP_DaySpLo;


					}

				else if(address == DAY_COOLING_DEADBAND)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_DayCoolingDeadband;

					}

				else if(address == DAY_HEATING_DEADBAND)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_DayHeatingDeadband;

					}

				else if(address == DAY_COOLING_SETPOINT)//
					{
					temp1 = EEP_DayCoolingSpHi;//(>> 8) & 0xff;//
					temp2 = EEP_DayCoolingSpLo;

					}

				else if(address == DAY_HEATING_SETPOINT)//
					{
					temp1 = EEP_DayHeatingSpHi;//(>> 8) & 0xff;//
					temp2 = EEP_DayHeatingSpLo;

					}

				else if(address == NIGHT_SETPOINT)//
					{
					temp1 = EEP_NightSpHi;//(>> 8) & 0xff;//
					temp2 = EEP_NightSpLo;

					}

				else if(address == APPLICATION)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_Application;

					}

				else if(address == NIGHT_HEATING_DEADBAND)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_NightHeatingDeadband;

					}

				else if(address == NIGHT_COOLING_DEADBAND)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_NightCoolingDeadband;

					}

				else if(address == NIGHT_HEATING_SETPOINT)//
					{
					temp1 = EEP_NightHeatingSpHi;//(>> 8) & 0xff;//
					temp2 = EEP_NightHeatingSpLo;

					}

				else if(address == NIGHT_COOLING_SETPOINT)//
					{
					temp1 = EEP_NightCoolingSpHi;//(>> 8) & 0xff;//
					temp2 = EEP_NightCoolingSpLo;

					}


		//WINDOW_INTERLOCK_COOLING_SETPOINT,   //TBD
		//WINDOW_INTERLOCK_HEATING_SETPOINT,	//TBD

				else if(address == UNIVERSAL_NIGHTSET)//
					{
					temp1 = EEP_UniversalNightSetpointHi;//(>> 8) & 0xff;//
					temp2 = EEP_UniversalNightSetpointLo;

					}

				else if(address == UNIVERSAL_SET)//
					{
					temp1 = EEP_UniversalSetpointHi;//(>> 8) & 0xff;//
					temp2 = EEP_UniversalSetpointLo;

					}

				else if(address == UNIVERSAL_HEAT_DB)//
					{
					temp1 = UniversalHeatDB_HI;//(>> 8) & 0xff;//
					temp2 = UniversalHeatDB_LO;

					}

				else if(address == UNIVERSAL_COOL_DB)//
					{
					temp1 = UniversalCoolDB_HI;//(>> 8) & 0xff;//
					temp2 = UniversalCoolDB_LO;

					}


		//ECOMONY_COOLING_SETPOINT,
		//ECOMONY_HEATING_SETPOINT,

				else if(address == POWERUP_SETPOINT)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_PowerupSetpoint;

					}

				else if(address == MAX_SETPOINT)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_MaxSetpoint;

					}

				else if(address == MIN_SETPOINT)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_MinSetpoint;

					}

	//	MAX_SETPOINT2,//			// max and min setpoint for celling setpoint
	//	MIN_SETPOINT2,//
	//	MAX_SETPOINT3,//			// max and min setpoint for average setpoint
	//	MIN_SETPOINT3,//
	//	MAX_SETPOINT4,//
	//	MIN_SETPOINT4,//

				else if(address == SETPOINT_INCREASE)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_SetpointIncrease;

					}


				else if(address == FREEZE_TEMP_SETPOINT)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_FreezeSetpoint;

					}
				else if(address == SETPOINT_UNLIMIT)
				{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = SetpointUNLimit;
				}

				else if(address == TEMP_SELECT)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_TempSelect;

					}

				else if(address == INPUT1_SELECT)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_Input1Select;

					}

				else if(address == COOLING_PID)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = (100 - pid[0]) & 0xFF;

					}

				else if(address == COOLING_PTERM)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_PTERM1;

					}

				else if(address == COOLING_ITERM)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_ITERM1;

					}

				else if(address == UNIVERSAL_PTERM)//
					{
					temp1 = EEP_PTERM2_HI;//(>> 8) & 0xff;//
					temp2 = EEP_PTERM2_LO;

					}

				else if(address == UNIVERSAL_ITERM)//
					{
					temp1 = EEP_ITERM2_HI;//(>> 8) & 0xff;//
					temp2 = EEP_ITERM2_LO;

					}

				else if(address == TSTAT_PID_UNIVERSAL)//
					{
					temp1 = 0;//(>> 8) & 0xff;//

					temp2 = (100 - pid[1]) & 0xFF;


					}

				else if(address == UNITS1_HIGH)//
					{
					temp1 = UnitS1_HI_HI;//(>> 8) & 0xff;//
					temp2 = UnitS1_HI_LO;

					}

				else if(address == UNITS1_LOW)//
					{
					temp1 = UnitS1_LO_HI;//(>> 8) & 0xff;//
					temp2 = UnitS1_LO_LO;

					}

				else if(address == UNITS2_HIGH)//
					{
					temp1 = UnitS2_HI_HI;//(>> 8) & 0xff;//
					temp2 = UnitS2_HI_LO;

					}

				else if(address == UNITS2_LOW)//
					{
					temp1 = UnitS2_LO_HI;//(>> 8) & 0xff;//
					temp2 = UnitS2_LO_LO;

					}

				else if(address == TSTAT_PID2_MODE_OPERATION)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					if(current_mode_of_operation[1] >= 4 && current_mode_of_operation[1] <= 6)
						temp2 = current_mode_of_operation[1] + 10;

					else if(current_mode_of_operation[1] >= 7 && current_mode_of_operation[1] <= 9)
						temp2 = current_mode_of_operation[1] - 3;

					else if(current_mode_of_operation[1] >= 10 && current_mode_of_operation[1] <= 12)
						temp2 = current_mode_of_operation[1] + 7;

					else
						temp2 = current_mode_of_operation[1];


					}

				else if(address == KEYPAD_SELECT)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_KeypadSelect;

					}

				else if(address == SPECIAL_MENU_LOCK)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_SpecialMenuLock;

					}

	//			else if(address == DISPLAY)//
	//				{
	//				temp1 = 0;//(>> 8) & 0xff;//
	//				temp2 = EEP_Display;
	//				uart_send[send_cout++] = temp1 ;
	//				uart_send[send_cout++] = temp2 ;
	//				crc16_byte(temp1);
	//				crc16_byte(temp2);
	//				}

	//	ICON,
				else if(address == LAST_KEY_TIMER)//
					{
					temp1 = (lastkey_counter>> 8) & 0xff;//
					temp2 = lastkey_counter & 0xff;

					}

				else if(address == TSTAT_KEYPAD_VALUE)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = keypad_value;

					keypad_value = 0;//reset once the register has been read

					}


	//	DISPLAY_HUNDRED,//
	//	DISPLAY_TEN,//
	//	DISPLAY_DIGITAL,//
	//	DISPLAY_STATUS,//
				else if(address == ROUND_DISPLAY)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_RoundDisplay;

					}

				else if(address == MIN_ADDRESS)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_MinAddress;

					}

				else if(address == MAX_ADDRESS)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_MaxAddress;

					}

	//	EEPROM_SIZE,//
				else if(address == TIMER_SELECT)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = EEP_TimerSelect;

					}

				else if(address == RTC_YEAR)//
					{
					temp1 = (calendar.w_year >> 8) & 0xff;//
					temp2 = calendar.w_year;

					}

				else if(address == RTC_MONTH)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = calendar.w_month;

					}

				else if(address == RTC_DAY)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = calendar.w_date;

					}

				else if(address == RTC_HOUR)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = calendar.hour;

					}

				else if(address == RTC_MINUTE)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = calendar.min;

					}

				else if(address == RTC_SECOND)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = calendar.sec;

					}

				else if(address == RTC_WEEK)//
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = calendar.week;

					}





	// 	MONTH,//
	// 	WEEK,//
	// 	DAY,//
	//	HOUR,//
	// 	MINUTE,//
	// 	SECOND,//
	//	DIAGNOSTIC_ALARM,//

	//			else if(address == SCHEDULE_DHOME_HOUR)//
	//				{
	//				temp1 = 0;//(>> 8) & 0xff;//
	//				temp2 = ScheduleDHomeHour;

	//				}
	//			else if(address == SCHEDULE_DHOME_MIN)//
	//				{
	//				temp1 = 0;//(>> 8) & 0xff;//
	//				temp2 = ScheduleDHomeMin;

	//				}
	//			else if(address == SCHEDULE_WORK_HOUR)//
	//				{
	//				temp1 = 0;//(>> 8) & 0xff;//
	//				temp2 = ScheduleWorkHour;

	//				}
	//			else if(address == SCHEDULE_WORK_MIN)//
	//				{
	//				temp1 = 0;//(>> 8) & 0xff;//
	//				temp2 = ScheduleWorkMin;

	//				}
	//			else if(address == SCHEDULE_NHOME_HOUR)//
	//				{
	//				temp1 = 0;//(>> 8) & 0xff;//
	//				temp2 = ScheduleNHomeHour;

	//				}
	//			else if(address == SCHEDULE_NHOME_MIN)//
	//				{
	//				temp1 = 0;//(>> 8) & 0xff;//
	//				temp2 = ScheduleNHomeMin;

	//				}
	//			else if(address == SCHEDULE_SLEEP_HOUR)//
	//				{
	//				temp1 = 0;//(>> 8) & 0xff;//
	//				temp2 = ScheduleSleepHour;

	//				}
	//			else if(address == SCHEDULE_SLEEP_MIN)//
	//				{
	//				temp1 = 0;//(>> 8) & 0xff;//
	//				temp2 = ScheduleSleepMin;

	//				}
	//			else if(address == SCHEDULE_HOLIDAY_BYEAR)//
	//				{
	//				temp1 = 0;//(>> 8) & 0xff;//
	//				temp2 = ScheduleHolidayByear;

	//				}
	//			else if(address == SCHEDULE_HOLIDAY_BMONTH)//
	//				{
	//				temp1 = 0;//(>> 8) & 0xff;//
	//				temp2 = ScheduleHolidayBmon;

	//				}
	//			else if(address == SCHEDULE_HOLIDAY_BDAY)//
	//				{
	//				temp1 = 0;//(>> 8) & 0xff;//
	//				temp2 = ScheduleHolidayBday;

	//				}
	//			else if(address == SCHEDULE_HOLIDAY_EYEAR)//
	//				{
	//				temp1 = 0;//
	//				temp2 = ScheduleHolidayEyear;

	//				}
	//			else if(address == SCHEDULE_HOLIDAY_EMONTH)//
	//				{
	//				temp1 = 0;//(>> 8) & 0xff;//
	//				temp2 = ScheduleHolidayEmon;

	//				}
	//			else if(address == SCHEDULE_HOLIDAY_EDAY)//
	//				{
	//				temp1 = 0;//(>> 8) & 0xff;//
	//				temp2 = ScheduleHolidayEday;

	//				}

	//
	//			else if(address == DAY2_EVENT4_HI)//
	//				{
	//				temp1 = 0;//(>> 8) & 0xff;//
	//				temp2 = Day2_Event4_HI;

	//				}
	//			else if(address == DAY2_EVENT4_LO)//
	//				{
	//				temp1 = 0;//(>> 8) & 0xff;//
	//				temp2 = Day2_Event4_LO;

	//				}

				else if(address == LCD_TURN_OFF)
					{
					temp1 = 0;//(>> 8) & 0xff;//
					temp2 = LCDTurnOff;

					}

				else if((address >= LINE1_CHAR1) && (address <= LINE2_CHAR4))
					{
					temp1 = User_Info((address - LINE1_CHAR1) * 2);
					temp2 = User_Info((address - LINE1_CHAR1)*2 + 1);

					}

				else if((address >= INTERNAL_CHAR1) && (address <= INTERNAL_CHAR4))
					{
					temp1 = Disp_internal_sensor((address - INTERNAL_CHAR1) * 2);
					temp2 = Disp_internal_sensor((address - INTERNAL_CHAR1)*2 + 1);

					}

				else if((address >= AI1_CHAR1) && (address <= AI1_CHAR4))
					{
					temp1 = Disp_AI1((address - AI1_CHAR1) * 2);
					temp2 = Disp_AI1((address - AI1_CHAR1)*2 + 1);

					}
				else if((address >= AI2_CHAR1) && (address <= AI2_CHAR4))
					{
					temp1 = Disp_AI2((address - AI2_CHAR1) * 2);
					temp2 = Disp_AI2((address - AI2_CHAR1)*2 + 1);

					}
				else if((address >= AI3_CHAR1) && (address <= AI3_CHAR4))
					{
					temp1 = Disp_AI3((address - AI3_CHAR1) * 2);
					temp2 = Disp_AI3((address - AI3_CHAR1)*2 + 1);

					}
				else if((address >= AI4_CHAR1) && (address <= AI4_CHAR4))
					{
					temp1 = Disp_AI4((address - AI4_CHAR1) * 2);
					temp2 = Disp_AI4((address - AI4_CHAR1)*2 + 1);

					}
				else if((address >= AI5_CHAR1) && (address <= AI5_CHAR4))
					{
					temp1 = Disp_AI5((address - AI5_CHAR1) * 2);
					temp2 = Disp_AI5((address - AI5_CHAR1)*2 + 1);

					}
				else if((address >= AI6_CHAR1) && (address <= AI6_CHAR4))
					{
					temp1 = Disp_AI6((address - AI6_CHAR1) * 2);
					temp2 = Disp_AI6((address - AI6_CHAR1)*2 + 1);

					}
				else if((address >= AI7_CHAR1) && (address <= AI7_CHAR4))
					{
					temp1 = Disp_AI7((address - AI7_CHAR1) * 2);
					temp2 = Disp_AI7((address - AI7_CHAR1)*2 + 1);

					}
				else if((address >= AI8_CHAR1) && (address <= AI8_CHAR4))
					{
					temp1 = Disp_AI8((address - AI8_CHAR1) * 2);
					temp2 = Disp_AI8((address - AI8_CHAR1)*2 + 1);

					}

				else if((address >= OUTPUT1_CHAR1) && (address <= OUTPUT1_CHAR4))
					{
					temp1 = Disp_OUT1((address - OUTPUT1_CHAR1) * 2);
					temp2 = Disp_OUT1((address - OUTPUT1_CHAR1)*2 + 1);

					}
				else if((address >= OUTPUT2_CHAR1) && (address <= OUTPUT2_CHAR4))
					{
					temp1 = Disp_OUT2((address - OUTPUT2_CHAR1) * 2);
					temp2 = Disp_OUT2((address - OUTPUT2_CHAR1)*2 + 1);

					}
				else if((address >= OUTPUT3_CHAR1) && (address <= OUTPUT3_CHAR4))
					{
					temp1 = Disp_OUT3((address - OUTPUT3_CHAR1) * 2);
					temp2 = Disp_OUT3((address - OUTPUT3_CHAR1)*2 + 1);

					}
				else if((address >= OUTPUT4_CHAR1) && (address <= OUTPUT4_CHAR4))
					{
					temp1 = Disp_OUT4((address - OUTPUT4_CHAR1) * 2);
					temp2 = Disp_OUT4((address - OUTPUT4_CHAR1)*2 + 1);

					}
				else if((address >= OUTPUT5_CHAR1) && (address <= OUTPUT5_CHAR4))
					{
					temp1 = Disp_OUT5((address - OUTPUT5_CHAR1) * 2);
					temp2 = Disp_OUT5((address - OUTPUT5_CHAR1)*2 + 1);

					}
				else if((address >= OUTPUT6_CHAR1) && (address <= OUTPUT6_CHAR4))
					{
					temp1 = Disp_OUT6((address - OUTPUT6_CHAR1) * 2);
					temp2 = Disp_OUT6((address - OUTPUT6_CHAR1)*2 + 1);

					}
				else if((address >= OUTPUT7_CHAR1) && (address <= OUTPUT7_CHAR4))
					{
					temp1 = Disp_OUT7((address - OUTPUT7_CHAR1) * 2);
					temp2 = Disp_OUT7((address - OUTPUT7_CHAR1)*2 + 1);

					}


	//	SCHEDULEA_CHAR1,	//						//
	//	SCHEDULEA_CHAR2,
	//	SCHEDULEA_CHAR3,
	//	SCHEDULEA_CHAR4,
	//	SCHEDULEB_CHAR1,							//
	//	SCHEDULEB_CHAR2,
	//	SCHEDULEB_CHAR3,
	//	SCHEDULEB_CHAR4,
	//	SCHEDULEC_CHAR1,							//
	//	SCHEDULEC_CHAR2,
	//	SCHEDULEC_CHAR3,//
	//	SCHEDULEC_CHAR4,
	//	SCHEDULED_CHAR1,							//
	//	SCHEDULED_CHAR2,
	//	SCHEDULED_CHAR3,
	//	SCHEDULED_CHAR4,
	//	WALL_CHAR1,							//
	//	WALL_CHAR2,
	//	WALL_CHAR3,
	//	WALL_CHAR4,
	//	CEILING_CHAR1,//						//
	//	CEILING_CHAR2,
	//	CEILING_CHAR3,
	//	CEILING_CHAR4,
	//	OUTDOOR_CHAR1,//
	//	OUTDOOR_CHAR2,
	//	OUTDOOR_CHAR3,
	//	OUTDOOR_CHAR4,
	//	AVERAGE_CHAR1,//
	//	AVERAGE_CHAR2,
	//	AVERAGE_CHAR3,//
	//	AVERAGE_CHAR4,
	//	LCD_SCREEN1,//
	//	LCD_SCREEN2,//

				else if(address == DEMAND_RESPONSE)
					{
					temp1 = 0;
					temp2 = DemandResponse;

					}

				else if(address == LOCK_REGISTER)
					{
					temp1 = 0;
					temp2 = LockRegister;

					}

				else if(address == PIR_STAGE)
					{
					temp1 = 0;
					temp2 = EEP_PirStage;

					}

				else if(address == PIR_STAGE)
					{
					temp1 = 0;
					temp2 = EEP_PirStage;

					}

	//	FIRST_CAL_FLAG, //	        // picdataok
	//	HUM_CAL_EREASE,//			// erease current PIC calibration data table
	//	HUMCOUNT3_H ,//
	//	HUMRH3_H	,	//
	//	HUMCOUNT4_H ,//
	//	HUMRH4_H	,	//
	//	HUMCOUNT5_H ,//
	//	HUMRH5_H	,	//
	//	HUMCOUNT6_H ,//
	//	HUMRH6_H	,	//
	//	HUMCOUNT7_H ,//
	//	HUMRH7_H	,	//
	//	HUMCOUNT8_H ,//
	//	HUMRH8_H	,	//
	//	HUMCOUNT9_H ,//
	//	HUMRH9_H	,	//
	//	HUMCOUNT10_H,//
	//	HUMRH10_H,   //
	//	HUM_LOCK_A,
	//	HUM_LOCK_B,

				else if(address == LCD_ROTATE_ENABLE)
					{
					temp1 = 0;
					temp2 = lcd_rotate_max;

					}

				else if(address == SCHEDULE_ON_OFF)
					{
					temp1 = 0;
					temp2 = schedule_on_off;

					}

				else if((address >= DISP_ITEM_TEMPERATURE) && (address <= DISP_ITEM_OUT7))
					{
					temp1 = 0;
					temp2 = disp_item_queue(address - DISP_ITEM_TEMPERATURE);

					}

				else if((address >= OUTPUT_PWM_AUTO_COAST) && (address <= OUTPUT_PWM_AUTO_HEAT3))
					{
					temp1 = 0;
					temp2 = Output_PWM_Table(address - OUTPUT_PWM_AUTO_COAST);

					}

				else if(address == PWM_OUT1)
				{
					temp1 = 0;
					temp2 = pwm1_percent[0];
				}


				else if(address == PWM_OUT2)
				{
					temp1 = 0;
					temp2 = pwm1_percent[1];
				}
				else if(address == EXT_SENSOR_TEMP_CAL) //temperature sensor on hum board calibration
					{
					temp1 = Calibration_External_Tem_HI;
					temp2 = Calibration_External_Tem_LO;

					}

	//	PWM_OUT4,
	//	PWM_OUT5,
	//	SUN_ICON_CONTROL, //
	//	MOON_ICON_CONTROL,//

	//			else if(address == BUTTON_LEFT1) //
	//				{
	//				temp1 = 0;
	//				temp2 = LeftButton1;
	//
	//				}
	//
	//			else if(address == BUTTON_LEFT2) //
	//				{
	//				temp1 = 0;
	//				temp2 = LeftButton2;
	//
	//				}

	//			else if(address == BUTTON_LEFT3) //
	//				{
	//				temp1 = 0;
	//				temp2 = LeftButton3;
	//
	//				}

	//			else if(address == BUTTON_LEFT4) //
	//				{
	//				temp1 = 0;
	//				temp2 = LeftButton4;
	//
	//				}

	//			else if(address == HUM_HEATING_CONTROL) //
	//				{
	//				temp1 = 0;
	//				temp2 = HumHeatControl;
	//
	//				}

				else if(address == HUM_INPUT_MANUAL_ENABLE) //
					{
					temp1 = 0;
					temp2 = hum_manual_enable;

					}

				else if(address == HUM_INPUT_MANUAL_VALUE) //
					{
					temp1 = HumInputManualValue_HI;
					temp2 = HumInputManualValue_LO;

					}

				else if(address == CO2_INPUT_MANUAL_ENABLE) //
					{
					temp1 = 0;
					temp2 = co2_manual_enable;

					}

				else if(address == CO2_INPUT_MANUAL_VALUE) //
					{
					temp1 = CO2InputManualValue_HI;
					temp2 = CO2InputManualValue_LO;

					}

				else if(address == CO2_CALIBRATION) //
					{
					temp1 = (co2_calibration_data >> 8)& 0xff;//Calibration_CO2_HI;
					temp2 = co2_calibration_data & 0xff;//Calibration_CO2_LO;

					}

				else if((address >= UNIVERSAL_OFF_TABLE_BEGIN) && (address <= UNIVERSAL_OFF_TABLE_HEAT3))
					{
					temp1 = 0;
					temp2 = PID2_Output_OFF_Table(address - UNIVERSAL_OFF_TABLE_BEGIN);

					}

				else if((address >= UNIVERSAL_OFF_VALVE_BEGIN) && (address <= UNIVERSAL_OFF_VALVE_HEAT3))
					{
					temp1 = 0;
					temp2 = PID2_Valve_OFF_Table(address - UNIVERSAL_OFF_VALVE_BEGIN);

					}

				else if(address == KEYPAD_REVERSE) //
					{
					temp1 = 0;
					temp2 = KeypadReverse;

					}

				else if(address == LIGHT_SENSOR) //
					{
					temp1 = (light_sensor >> 8) & 0xff;
					temp2 = light_sensor & 0xff;

					}

				else if(address == PIR_SENSOR_SELECT) //
					{
					temp1 = 0;
					temp2 = PirSensorSelect;

					}

				else if(address == PIR_SENSOR_VALUE) //
					{
					temp1 = (pir_value >> 8) & 0xff;
					temp2 = pir_value & 0xff;

					}

				else if(address == PIR_SENSOR_ZERO) //
					{
					temp1 = 0;
					temp2 = PirSensorZero;
					}

				else if(address == MIC_SOUND) //
					{
					temp1 = (sound_level >> 8)& 0xff;
					temp2 = sound_level & 0xff;
					}

				else if((address >= UNIVERSAL_VALVE_BEGIN) && (address <= UNIVERSAL_VALVE_HEAT3))
					{
					temp1 = 0;
					temp2 = PID2_Valve_Table(address - UNIVERSAL_VALVE_BEGIN);

					}

				else if(address == TSTAT_ID_WRITE_ENABLE) //
					{
					temp1 = 0;
					temp2 = ID_Lock;
					}
				else if(address == PWM_ENABLE)
				{
					temp1 = 0;
					temp2 = pwm_duty/10;
				}
        else if(address == SCHEDULE_MODE_NUM)
				{
					temp1 = 0;
					temp2 = ScheduleModeNum;
				}


				else if(address == PIR_TIMER) //
					{
					temp1 = (override_timer_time >> 8) & 0xff;
					temp2 = override_timer_time & 0xff;

					}

				else if(address == SUPPLY_SETPOINT) //
					{
					temp1 = Supply_SP_HI;
					temp2 = Supply_SP_LO;

					}

				else if(address == MAX_SUPPLY_SETPOINT) //
					{
					temp1 = Supply_Max_SP_HI;
					temp2 = Supply_Max_SP_LO;

					}

				else if(address == MIN_SUPPLY_SETPOINT) //
					{
					temp1 = Supply_Min_SP_HI;
					temp2 = Supply_Min_SP_LO;

					}

				else if(address == MAX_AIRFLOW_COOLING) //
					{
					temp1 = Max_AirflowSP_Cool_HI;
					temp2 = Max_AirflowSP_Cool_LO;

					}

				else if(address == MAX_AIRFLOW_HEATING) //
					{
					temp1 = Max_AirflowSP_Heat_HI;
					temp2 = Max_AirflowSP_Heat_LO;

					}


				else if(address == OCC_MIN_AIRFLOW) //
					{
					temp1 = Occ_Min_Airflow_HI;
					temp2 = Occ_Min_Airflow_LO;

					}

				else if(address == AIRFLOW_SETPOINT) //
					{
					temp1 = Airflow_SP_HI;
					temp2 = Airflow_SP_LO;

					}

				else if(address == VAV_CONTROL) //
					{
					temp1 = 0;
					temp2 = VAV_Control;

					}

				else if(address == PID3_INPUT_SELECT) //
					{
					temp1 = 0;
					temp2 = PID3_InputSelect;

					}

				else if((address >= PID3_VALVE_OPER_TABLE_BEGIN) && (address <= PID3_VALVE_OPER_TABLE_HEAT3))
					{
					temp1 = 0;
					temp2 = PID3_Valve_Table(address - PID3_VALVE_OPER_TABLE_BEGIN);

					}

				else if(address == PID3_COOLING_DB) //
					{
					temp1 = 0;
					temp2 = pid3_cool_db;

					}


				else if(address == PID3_HEATING_DB) //
					{
					temp1 = 0;
					temp2 = pid3_heat_db;

					}

				else if(address == PID3_PTERM) //
					{
					temp1 = 0;
					temp2 = pid3_pterm;

					}

				else if(address == PID3_ITERM) //
					{
					temp1 = 0;
					temp2 = pid3_iterm;

					}

				else if(address == PID3_HEAT_STAGE) //
					{
					temp1 = 0;
					temp2 = EEP_HEAT_TABLE3;

					}

				else if(address == PID3_COOL_STAGE) //
					{
					temp1 = 0;
					temp2 = EEP_COOL_TABLE3;

					}

				else if(address == PID3_OUTPUT) //
					{
					temp1 = 0;
					temp2 = pid[2];

					}

				else if((address >= PID3_OUTPUT_BEGIN) && (address <= PID3_OUTPUT_HEAT3))
					{
					temp1 = 0;
					temp2 = PID3_Output_Table(address - PID3_OUTPUT_BEGIN);

					}

				else if((address >= PID3_VALVE_OFF_TABLE_BEGIN) && (address <= PID3_VALVE_OFF_TABLE_HEAT3))
					{
					temp1 = 0;
					temp2 = PID3_Valve_Off_Table(address - PID3_VALVE_OFF_TABLE_BEGIN);

					}

				else if((address >= PID3_OFF_OUTPUT_BEGIN) && (address <= PID3_OFF_OUTPUT_HEAT3))
					{
					temp1 = 0;
					temp2 = PID3_Output_OFF_Table(address - PID3_OFF_OUTPUT_BEGIN);

					}

	//	WIRELESS_PIR_RESPONSE1,
	//	WIRELESS_PIR_RESPONSE2,
	//	WIRELESS_PIR_RESPONSE3,
	//	WIRELESS_PIR_RESPONSE4,
	//	WIRELESS_PIR_RESPONSE5,
				else if(address == TSTAT_HEAT_COOL) //
					{
					temp1 = 0;
					temp2 = heat_cool_user;

					}

	//	SPARE1,
	//	SPARE2,
				else if(address == TSTAT_HUM_PIC_VERSION) //
					{
					temp1 = 0;
					temp2 = Hum_PIC_Version;

					}

				else if(address == INTERNAL_SENSOR_MANUAL) //
					{
					temp1 = 0;
					temp2 = EEP_InterThermistorManual;

					}

				else if(address == PRESSURE_VALUE) //
					{
					temp1 = 0;
					temp2 = 0;

					}

//				else if(address == PRESSURE_MANUAL_VALUE) //
//					{
//					temp1 = PressureManualValue_HI;
//					temp2 = PressureManualValue_LO;
//
//					}

//				else if(address == PRESSURE_MANUAL_ENABLE) //
//					{
//					temp1 = 0;
//					temp2 = PressureManualEnable;
//
//					}

				else if(address == AQ_VALUE) //
					{
					temp1 = (aq_value>>8) & 0xff;
					temp2 = aq_value & 0xff;
					}

				else if(address == AQ_MANUAL_VALUE) //
					{
					temp1 = AQManualValue_HI;
					temp2 = AQManualValue_LO;

					}

				else if(address == AQ_MANUAL_ENABLE) //
					{
					temp1 = 0;
					temp2 = AQManualEnable;

					}
	//	TEMPERATURE_OF_PRESSURE_SENSOR,
	//	TEMPERATURE_OF_HUM_SENSOR,

				else if(address == SP_DISPLAY_SELECT) //
					{
					temp1 = 0;
					temp2 = SP_DisplaySelect;

					}

				else if(address == PID3_DAY_SP) //
					{
					temp1 = PID3_DaySP_HI;
					temp2 = PID3_DaySP_LO;

					}

				else if(address == PID3_NIGHT_SP) //
					{
					temp1 = PID3_NightSP_HI;
					temp2 = PID3_NightSP_LO;

					}
				else if(address == MODBUS_EX_MOUDLE_EN)
				{
					temp1 = 0;
					temp2 = ex_moudle.enable;
				}
				else if(address == MODBUS_EX_MOUDLE_FLAG12)
				{
					temp1 = (ex_moudle.flag >> 8) & 0XFF;
					temp2 = ex_moudle.flag & 0xff;
				}
				else if(address == MODBUS_EX_MOUDLE_FLAG34)
				{
					temp1 = (ex_moudle.flag >> 24) & 0XFF;
					temp2 = (ex_moudle.flag>>16) & 0xff;
				}
	//	BK_SN,
	//	BK_SNLOLO,
	//	BK_SNLOHI,
	//	BK_SNHILO,
	//	BK_SNHIHI,
	//	BK_PN,
	//	BK_HWN,

				else if(address == TSTAT_NAME_ENABLE) //
					{
					temp1 = 0;
					temp2 = 0x56;

					}

				else if((address >= TSTAT_NAME1) && (address <= TSTAT_NAME10)) //
					{
					temp1 = UserInfo_Name((address - TSTAT_NAME1)*2);
					temp2 = UserInfo_Name((address - TSTAT_NAME1)*2 + 1);

					}

				else if(address == SHOW_ID) //
					{
					temp1 = 0;
					temp2 = Show_ID_Enable;

					}

				else if(address == CO2_SELF_CAL)
				{
					temp1 = 0;
					temp2 = CO2_AutoCal;

				}

				else if(address == TRANSDUCER_RANGE_MIN) //
					{
					temp1 = 0;
					temp2 = MinTransducerRange;

					}

				else if(address == TRANSDUCER_RANGE_MAX) //
					{
					temp1 = 0;
					temp2 = MaxTransducerRange;

					}

				else if(address == ICON_MANUAL_MODE) //
					{
					temp1 = 0;
					temp2 = ICON_ManualMode;

					}

				else if(address == ICON_MANUAL_VALUE) //
					{
					temp1 = 0;
					temp2 = ICON_ManualValue;

					}

				else if(address == MODBUS_PM25_PARTICLE_UINT)
				{
					temp1= 0 ;
					temp2= pm25_unit;
				}

				else if(address == MODBUS_PM25_WEIGHT_1_0)
				{
					temp1 = pm25_weight_10 >> 8 ;
					temp2 = pm25_weight_10;
				}
				else if(address == MODBUS_PM25_WEIGHT_2_5)
				{
					temp1 = pm25_weight_25 >> 8 ;
					temp2 = pm25_weight_25;
				}

				else if(address == MODBUS_PM25_WEIGHT_4_0)
				{
					temp1 = pm25_weight_40 >> 8 ;
					temp2 = pm25_weight_40;
				}

			  else if(address == MODBUS_PM25_WEIGHT_10)
				{
					temp1 = pm25_weight_100 >> 8 ;
					temp2 = pm25_weight_100;
				}
				else if(address == MODBUS_PM25_NUMBER_0_5)
				{
					temp1 = pm25_number_05 >> 8 ;
					temp2 = pm25_number_05;
				}

				else if(address == MODBUS_PM25_NUMBER_1_0)
				{
					temp1 = pm25_number_10 >> 8 ;
					temp2 = pm25_number_10;
				}

				else if(address == MODBUS_PM25_NUMBER_2_5)
				{
					temp1 = pm25_number_25 >> 8 ;
					temp2 = pm25_number_25;
				}

				else if(address == MODBUS_PM25_NUMBER_4_0)
				{
					temp1 = pm25_number_40 >> 8 ;
					temp2 = pm25_number_40;
				}

				else if(address == MODBUS_PM25_NUMBER_10)
				{
					temp1 = pm25_number_100 >> 8 ;
					temp2 = pm25_number_100;
				}
				else if(address == MODBUS_PATICAL_SIZE)
				{
					temp1 = typical_partical_size>>8;
					temp2 = typical_partical_size;
				}




				else if(address == HUM_CALIBRATION) //
					{
					temp1 = (humidity_calibration>>8) & 0XFF;
					temp2 = humidity_calibration & 0XFF;

					}
				else if(address == HUM_TEM)
				{
					temp1 = htu_temp >> 8;
					temp2 = htu_temp & 0xff;
				}
				else if(address == DTERM) //
					{
					temp1 = 0;//dew_data >> 8;
					temp2 = EEP_DTERM1 & 0xff;
					}

				else if(address == PID_SAMPLE_TIME)
					{
						temp1 = 0;
						temp2 = PidSampleTime;
					}
				else if(address == CO2_ASC_ENABLE)
					{
						temp1 = co2_asc>>8;
						temp2 = co2_asc & 0xff;
					}
					else if(address == CO2_FRC_VALUE)
					{
						temp1 = co2_frc>>8;
						temp2 = co2_frc & 0xff;
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

//
//				else if(address == HUM_C_TEMP) //
//					{
//					temp1 = 0;//(sensor_hum_temp >> 8) & 0xff;
//					temp2 = 0;//sensor_hum_temp & 0xff;
//
//					}
//				else if(address == HUM_C_VALUE) //
//					{
//					temp1 = 0;//(sensor_hum_value >> 8) & 0xff;
//					temp2 = 0;//sensor_hum_value & 0xff;
//
//					}
//				else if(address == HUM_C_FRE) //
//					{
//					temp1 = 0;//(sensor_hum_fre >> 8) & 0xff;
//					temp2 = 0;//sensor_hum_fre & 0xff;
//
//					}

//				else if(address == HUM_C_POINTS) //
//					{
//					temp1 = 0;
//					temp2 = 0;//hum_size_copy;
//
//					}

//				else if((address >= HUM_C_RH1) && (address <= HUM_C_RH10) && ((address-HUM_C_RH1)%2 == 0))//
//					{
//					temp1 = Hum_C_RH_HI((address - HUM_C_RH1)/2);
//					temp2 = Hum_C_RH_LO((address - HUM_C_RH1)/2);
//
//					}
//				else if((address >= HUM_C_RH1) && (address <= HUM_C_RH10) && ((address-HUM_C_RH1)%2 == 1))//
//					{
//					temp1 = Hum_C_FRE_HI((address - HUM_C_FRE1)/2);
//					temp2 = Hum_C_FRE_LO((address - HUM_C_FRE1)/2);
//
//					}

//				else if(address == HUM_C_REFRESH_TABLE) //
//					{
//					temp1 = 0;
//					temp2 = HumRefreshTable;
//
//					}
//
//				else if(address == HUM_C_ERASE) //
//					{
//					temp1 = 0;
//					temp2 = HumErase;
//
//					}
//				else if(address == HUM_C_LOCK) //
//					{
//					temp1 = 0;
//					temp2 = HumLock;
//					}

				else if(address >= TSTAT_TEST1 && address <= TSTAT_TEST50) //
				{
					temp1 = (Test[address - TSTAT_TEST1]  >> 8) & 0xff;
					temp2 = Test[address - TSTAT_TEST1]  & 0xff;
				}

				else if((address >= MODE1_NAME1) && (address <= MODE5_NAME4))
					{
					temp1 = Modename((address - MODE1_NAME1)*2);
					temp2 = Modename((address - MODE1_NAME1)*2 + 1);

					}

				else if(address == OUTSIDETEM)
				{
					temp1 = (outside_tem >> 8) & 0xff;
					temp2 = outside_tem & 0xff;

				}

				else if(address == OUTSIDETEM_SLAVE)
				{
					temp1 = 0;//
					temp2 = outside_tem & 0xff;

				}

				else if(address == SLEEP_SP)
				{
					temp1 = Sleep_sp_hi;//
					temp2 = Sleep_sp_lo;

				}

				else if(address == SLEEP_COOLING_SP)
				{
					temp1 = SleepCoolSp_h;//
					temp2 = SleepCoolSp_l;

				}

				else if(address == SLEEP_HEATING_SP)
				{
					temp1 = SleepHeatSp_h;//
					temp2 = SleepHeatSp_l;

				}

				else if(address == SLEEP_COOLING_DB)
				{
					temp1 = 0;//
					temp2 = SleepCooldb;

				}

				else if(address == SLEEP_HEATING_DB)
				{
					temp1 = 0;//
					temp2 = SleepHeatdb;

				}

				else if(address == MAX_WORK_SP)
				{
					temp1 = 0;//
					temp2 = Max_work_sp;

				}

				else if(address == MIN_WORK_SP)
				{
					temp1 = 0;//
					temp2 = Min_work_sp;

				}

				else if(address == MAX_SLEEP_SP)
				{
					temp1 = 0;//
					temp2 = Max_sleep_sp;

				}

				else if(address == MIN_SLEEP_SP)
				{
					temp1 = 0;//
					temp2 = Min_sleep_sp;

				}
				else if(address == MAX_HOLIDAY_SP)
				{
					temp1 = 0;//
					temp2 = Max_holiday_sp;
				}

				else if(address == INPUT_VOLTAGE_TERM)
				{
					temp1 = 0;
					temp2 = InputVoltageTerm;
				}

				else if((address >= ICON_DAY_OUTPUT_CONTROL) && (address <= ICON_FAN3_OUTPUT_CONTROL))
				{
					temp1 = 0;
					temp2 = IconOutputControl(address - ICON_DAY_OUTPUT_CONTROL);
				}

			  else if((address >= OUTPUT_PWM_OFF_COAST) && (address <= OUTPUT_PWM_OFF_HEAT3))
				{
					temp1 = 0;
					temp2 = Output_PWM_Off_Table(address - OUTPUT_PWM_OFF_COAST);
				}

				else if(address == MIN_HOLIDAY_SP)
				{
					temp1 = 0;//
					temp2 = Min_holiday_sp;
				}

//				else if(address == BOARD_SELECT)
//				{
//					temp1 = 0;//
//					temp2 = BoardSelect;
//				}

				else if(address == SET_PIR_SENSETIVITY)
				{
					temp1 = 0;//(Pir_Sensetivity >> 8) & 0xff;//
					temp2 = Pir_Sensetivity & 0xff;

				}

				else if((address >= SCHEDULE_MONDAY_EVENT1_H)&&(address <= SCHEDULE_MONDAY_EVENT6_M))
				{
					temp1 = 0;
					temp2 =  ScheduleMondayEvent1(address - SCHEDULE_MONDAY_EVENT1_H);

				}

				else if((address >= SCHEDULE_TUESDAY_EVENT1_H)&&(address <= SCHEDULE_TUESDAY_EVENT6_M))
				{
					temp1 = 0;
					temp2 =  ScheduleTuesdayEvent1(address - SCHEDULE_TUESDAY_EVENT1_H);

				}

				else if((address >= SCHEDULE_WENSDAY_EVENT1_H)&&(address <= SCHEDULE_WENSDAY_EVENT6_M))
				{
					temp1 = 0;
					temp2 =  ScheduleWensdayEvent1(address - SCHEDULE_WENSDAY_EVENT1_H);

				}

				else if((address >= SCHEDULE_THURSDAY_EVENT1_H)&&(address <= SCHEDULE_THURSDAY_EVENT6_M))
				{
					temp1 = 0;
					temp2 =  ScheduleThursdayEvent1(address - SCHEDULE_THURSDAY_EVENT1_H);

				}

				else if((address >= SCHEDULE_FRIDAY_EVENT1_H)&&(address <= SCHEDULE_FRIDAY_EVENT6_M))
				{
					temp1 = 0;
					temp2 =  ScheduleFridayEvent1(address - SCHEDULE_FRIDAY_EVENT1_H);

				}

				else if((address >= SCHEDULE_SATDAY_EVENT1_H)&&(address <= SCHEDULE_SATDAY_EVENT6_M))
				{
					temp1 = 0;
					temp2 =  ScheduleSatdayEvent1(address - SCHEDULE_SATDAY_EVENT1_H);

				}

				else if((address >= SCHEDULE_SUNDAY_EVENT1_H)&&(address <= SCHEDULE_SUNDAY_EVENT6_M))
				{
					temp1 = 0;
					temp2 =  ScheduleSundayEvent1(address - SCHEDULE_SUNDAY_EVENT1_H);

				}

				else if((address >= SCHEDULE_HOLIDAY_EVENT1_H)&&(address <= SCHEDULE_HOLIDAY_EVENT6_M))
				{
					temp1 = 0;
					temp2 =  ScheduleHolidayEvent1(address - SCHEDULE_HOLIDAY_EVENT1_H);

				}
//				else if((address >= SCHEDULE_MONDAY_FLAG)&&(address <= SCHEDULE_HOLIDAY_FLAG))
//				{
//					temp1 = ScheduleMondayFlag((address - SCHEDULE_MONDAY_FLAG)*2 + 1);
//					temp2 = ScheduleMondayFlag((address - SCHEDULE_MONDAY_FLAG)*2);
//
//				}

				else if((address >= SCHEDULE_MONDAY_FLAG_1)&&(address <= SCHEDULE_HOLIDAY_FLAG_2))
				{
					if((address - SCHEDULE_MONDAY_FLAG_1)%2 == 0)//first flag reg
					{
						temp1 = ScheduleMondayFlag((address - SCHEDULE_MONDAY_FLAG_1)/2*3 + 1);
						temp2 = ScheduleMondayFlag((address - SCHEDULE_MONDAY_FLAG_1)/2*3);
					}
					else//second flag reg
					{
						temp1 = 0;
						temp2 = ScheduleMondayFlag((address - SCHEDULE_MONDAY_FLAG_1 -1)/2 *3 + 2);
					}
				}


				else if((address >= SCHEDULE_DAY_BEGAIN)&&(address <= SCHEDULE_DAY_END))
				{
					temp1 = 0;
					temp2 = ScheduleDay(address - SCHEDULE_DAY_BEGAIN);
				}

				else if(address == AWAY_SP)
				{
					temp1 = Away_sp_hi;
					temp2 = Away_sp_lo;
				}


			else if(address == MODBUS_CELBRA_AIR1)
		 {
				temp1 = air_cal_point[0] >> 8;
				temp2 = air_cal_point[0] & 0xff;
		 }

		 else if(address == MODBUS_CELBRA_AIR2)
		 {
				temp1 = air_cal_point[1] >> 8;
				temp2 = air_cal_point[1] & 0xff;
		 }

		 else if(address == MODBUS_CELBRA_AIR3)
		 {
				temp1 = air_cal_point[2] >> 8;
				temp2 = air_cal_point[2] & 0xff;
		 }

		 else if(address == MODBUS_CELBRA_AIR4)
		 {
				temp1 = air_cal_point[3] >> 8;
				temp2 = air_cal_point[3] & 0xff;
		 }

		 else if(address == MODBUS_AQ_LEVEL0)
		 {
				temp1 = (aq_level_value[0] >> 8)&0xff;
				temp2 = aq_level_value[0]&0xff;
		 }

		 else if(address == MODBUS_AQ_LEVEL1)
		 {
				temp1 = (aq_level_value[1] >> 8)&0xff;
				temp2 = aq_level_value[1]&0xff;
		 }

		 else if(address == MODBUS_AQ_LEVEL2)
		 {
				temp1 = (aq_level_value[2] >> 8)&0xff;
				temp2 = aq_level_value[2]&0xff;
		 }

		 else if(address == MODBUS_AQI_AREA)
		 {
				temp1 = 0;
				temp2 = AQI_area;
		 }
		 else if((address >= MODBUS_AQI_FIRST_LINE)&&(address <= MODBUS_AQI_FIFTH_LINE))
		 {
				temp1 = (aqi_table_customer[address-MODBUS_AQI_FIRST_LINE] >> 8)&0xff;
				temp2 = aqi_table_customer[address-MODBUS_AQI_FIRST_LINE]&0xff;
		 }
		 			else if(address == MODBUS_CO2_ON_TIME)
			{
					temp1 = (co2_on_time  >> 8)& 0xff;;
					temp2 = co2_on_time & 0xff ;
			}

			else if(address == MODBUS_CO2_OFF_TIME)
			{
					temp1 = (co2_off_time  >> 8)& 0xff;;
					temp2 = co2_off_time & 0xff ;
			}

			else if(address == MODBUS_PM25_ON_TIME)
			{
					temp1 = (pm25_on_time  >> 8)& 0xff;;
					temp2 = pm25_on_time & 0xff ;
			}

			else if(address == MODBUS_PM25_OFF_TIME)
			{
					temp1 = (pm25_off_time  >> 8)& 0xff;;
					temp2 = pm25_off_time & 0xff ;
			}
		 else if(address == MODBUS_MAX_AQ_VAL)
		 {
				temp1 = (aq_level_value[3] >> 8)&0xff;
				temp2 = aq_level_value[3]&0xff;
		 }

		 else if(address == MODBUS_CALIBRATION_AQ)
		 {
				temp1 = (aq_calibration >> 8)&0xff;
				temp2 = aq_calibration & 0xff;
		 }
		 else if(address == MODBUS_AQI)
		 {
			 temp1 = (AQI_value >> 8)&0xff;
			 temp2 = AQI_value&0xff;
		 }
		 else if( address == MODBUS_AQI_LEVEL)
		 {
			 temp1 = 0;
			 temp2 = AQI_level;
		 }
// FOR WIFI START
		 else if(address >= MODBUS_WIFI_START &&  address <= MODBUS_WIFI_END)
		 {
				U16_T far temp;
				temp = read_wifi_data_by_block(address);

				temp1 = (temp >> 8) & 0xFF;
				temp2 = temp & 0xFF;

		 }

// WIFI END

			else if(address == FRC_ENABLE)//free cooling  function enable/disable 0:DIABLE  1:ENABLE
			{
				temp1 = 0;
				temp2 =  FrcEnable;

			}
			else if(address == FRC_BASE_SELECT)//1001FREE COOLING BASE SELECT 0: TEMPERATUERE BASE 1: ENTHALPY BASE
			{
				temp1 = 0;
				temp2 = FrcBaseSelect;
			}
			//----------------------------------------
			else if(address == FRC_SPACETEM_SOURCE)//1002FREE COOLING SPACE TEMPERATURE SOURCE SELECT, INTERNAL OR NETWORK
			{
				temp1 = 0;
				temp2 = spacetem.source;
			}
			else if(address == FRC_SPACETEM_VALUE)//1002FREE COOLING SPACE TEMPERATURE SOURCE SELECT, INTERNAL OR NETWORK
			{
				temp1 = (spacetem.value >> 8)& 0xff;
				temp2 = spacetem.value & 0xff;
			}

				else if(address == FRC_SPACETEM_UNIT)//1002FREE COOLING SPACE TEMPERATURE SOURCE SELECT, INTERNAL OR NETWORK
				{
					temp1 = 0;
					if(spacetem.source == FRC_LOCAL_SENSOR)
						temp2 = EEP_DEGCorF;
					else
						temp2 = FrcSpaceTemUnit;
				}

				else if(address == FRC_SPACETEM_ID)//1003 ??? COULD BE INTERNAL SENSOR, AI1 TO AI8 OR GET FROM NETWORK
				{
					temp1 = 0;//FrcSpaceTemIDHi;
					temp2 =  spacetem.id;
				}

				else if(address == FRC_SPACETEM_LASTUPDATE)//1004 FREE COOLING SPACE TEMPERATURE LAST UPDATE TIME, UNIT: MINUTS
				{
					temp1 = 0;
					temp2 = 60 - spacetem.update ;
				}
				else if(address == FRC_SPACETEM_STATUE)//1005 CHECK IF SPACE TEMPERATURE SOURCE IS WORK WELL
					{
					temp1 = 0;
					temp2 =  spacetem.status;

				}
				else if(address == FRC_SPACETEM_CONFIG)//1006 FREE COOLING SPACE TEMPERATURE STATUS, CHECK RANGE,UNIT
				{
					temp1 = 0;
					temp2 = spacetem.config;

				}
  			//---------------------------------------------
				else if(address == FRC_SUPPLYTEM_SOURCE)//1007 FREE COOLING SUPPLY TEMPERATURE SOURCE SELECT, INTERNAL OR NETWORK
				{
					temp1 = 0;
					temp2 = supplytem.source;

				}
				else if(address == FRC_SUPPLYTEM_VALUE)//1002FREE COOLING SPACE TEMPERATURE SOURCE SELECT, INTERNAL OR NETWORK
				{
					temp1 = (supplytem.value >> 8)& 0xff;
					temp2 = supplytem.value & 0xff;
				}

				else if(address == FRC_SUPPLYTEM_UNIT)//1002FREE COOLING SPACE TEMPERATURE SOURCE SELECT, INTERNAL OR NETWORK
				{
					temp1 = 0;
					if(supplytem.source == FRC_LOCAL_SENSOR)
						temp2 = EEP_DEGCorF;
					else
						temp2 = FrcSupplyTemUnit;
				}

				else if(address == FRC_SUPPLYTEM_ID)//1008 ??? COULD BE INTERNAL SENSOR, AI1 TO AI8 OR GET FROM NETWORK
				{
					temp1 = 0;//FrcSupplyTemIDHi;
					temp2 =  supplytem.id;

				}
				else if(address == FRC_SUPPLYTEM_LASTUPDATE)//1009 FREE COOLING SUPPLY TEMPERATURE LAST UPDATE TIME, UNIT: MINUTS
					{
					temp1 = 0;
					temp2 = 60 - supplytem.update ;

				}
				else if(address == FRC_SUPPLYTEM_STATUE)//1010 CHECK IF SUPPLY TEMPERATURE SOURCE IS WORK WELL
					{
					temp1 = 0;
					temp2 =  supplytem.status;

				}
				else if(address == FRC_SUPPLYTEM_CONFIG)//1011 FREE COOLING SUPPLY TEMPERATURE STATUS, CHECK RANGE,UNIT
					{
					temp1 = 0;
					temp2 =  supplytem.config;

				}
	     //----------------------------------------
				else if(address == FRC_OUTDOORTEM_SOURCE)//1012 FREE COOLING OUTDOOR TEMPERATURE SOURCE SELECT, INTERNAL OR NETWORK
					{
					temp1 = 0;
					temp2 =  outdoortem.source;

				}
				else if(address == FRC_OUTDOORTEM_VALUE)//1002FREE COOLING SPACE TEMPERATURE SOURCE SELECT, INTERNAL OR NETWORK
				{
					temp1 = (outdoortem.value >> 8)& 0xff;
					temp2 = outdoortem.value & 0xff;
				}
				else if(address == FRC_OUTDOORTEM_UNIT)//1002FREE COOLING SPACE TEMPERATURE SOURCE SELECT, INTERNAL OR NETWORK
				{
					temp1 = 0;
					if(outdoortem.source == FRC_LOCAL_SENSOR)
						temp2 = EEP_DEGCorF;
					else
						temp2 = FrcOutdoorTemUnit;
				}

				else if(address == FRC_OUTDOORTEM_ID)//1013 ??? COULD BE INTERNAL SENSOR, AI1 TO AI8 OR GET FROM NETWORK
					{
					temp1 = 0;//FrcOutdoorTemIDHi;
					temp2 =  outdoortem.id;

				}
				else if(address == FRC_OUTDOORTEM_LASTUPDATE)//1014 FREE COOLING OUTDOOR TEMPERATURE LAST UPDATE TIME, UNIT: MINUTS
					{
					temp1 = 0;
					temp2 = 60 - outdoortem.update;

				}
				else if(address == FRC_OUTDOORTEM_STATUE)//1015 CHECK IF OUTDOOR TEMPERATURE SOURCE IS WORK WELL
					{
					temp1 = 0;
					temp2 =  outdoortem.status;

				}
				else if(address == FRC_OUTDOORTEM_CONFIG)//1016 FREE COOLING OUTDOOR TEMPERATURE STATUS, CHECK RANGE,UNIT
				{
					temp1 = 0;
					temp2 = outdoortem.config ;

				}
				//---------------------------------------
				else if(address == FRC_INDOORHUM_SOURCE)//1017 FREE COOLING INDOOR HUMIDITY SOURCE SELECT, INTERNAL OR NETWORK
					{
					temp1 = 0;
					temp2 = indoorhum.source;

				}
				else if(address == FRC_INDOORHUM_VALUE)//1002FREE COOLING SPACE TEMPERATURE SOURCE SELECT, INTERNAL OR NETWORK
				{
					temp1 = (indoorhum.value >> 8)& 0xff;
					temp2 = indoorhum.value & 0xff;
				}

				else if(address == FRC_INDOORHUM_ID)//1018 ??? COULD BE INTERNAL SENSOR, AI1 TO AI8 OR GET FROM NETWORK
					{
					temp1 = 0;//FrcIndoorHumIDHi;
					temp2 = indoorhum.id;

				}
				else if(address == FRC_INDOORHUM_LASTUPDATE)//1019 FREE COOLING OUTDOOR HUMIDITY LAST UPDATE TIME, UNIT: MINUTS
					{
					temp1 = 0;
					temp2 = 60 - indoorhum.update ;

				}
				else if(address == FRC_INDOORHUM_STATUE)//1020 CHECK IF INDOOR HUMIDITY SOURCE IS WORK WELL
					{
					temp1 = 0;
					temp2 =  indoorhum.status;

				}
				else if(address == FRC_INDOORHUM_CONFIG)//1021 FREE COOLING INDOOR HUMIDITY STATUS, CHECK RANGE,UNIT
				{
					temp1 = 0;
					temp2 = indoorhum.config ;

				}
   		 //------------------------------------------
				else if(address == FRC_OUTDOORHUM_SOURCE)//1022 FREE COOLING OUTDOOR HUMIDITY SOURCE SELECT, INTERNAL OR NETWORK
				{
					temp1 = 0;
					temp2 = outdoorhum.source;

				}
				else if(address == FRC_OUTDOORHUM_VALUE)//1002FREE COOLING SPACE TEMPERATURE SOURCE SELECT, INTERNAL OR NETWORK
				{
					temp1 = (outdoorhum.value >> 8)& 0xff;
					temp2 = outdoorhum.value & 0xff;
				}

				else if(address == FRC_OUTDOORHUM_ID)//1023 ??? COULD BE INTERNAL SENSOR, AI1 TO AI8 OR GET FROM NETWORK
				{
					temp1 = 0;//FrcOutdoorHumIDHi;
					temp2 = outdoorhum.id;

				}
				else if(address == FRC_OUTDOORHUM_LASTUPDATE)//1024 FREE COOLING OUTDOOR HUMIDITY LAST UPDATE TIME, UNIT: MINUTS
				{
					temp1 = 0;
					temp2 = 60 - outdoorhum.update ;

				}
				else if(address == FRC_OUTDOORHUM_STATUE)//1025 CHECK IF OUTDOOR HUMIDITY SOURCE IS WORK WELL
				{
					temp1 = 0;
					temp2 = outdoorhum.status ;

				}
				else if(address == FRC_OUTDOORHUM_CONFIG)//1026 FREE COOLING OUTDOOR HUMIDITY STATUS, CHECK RANGE,UNIT
				{
					temp1 = 0;
					temp2 = outdoorhum.config ;

				}
			//-----------------------------------------
				else if(address == FRC_MIN_FRESH_AIR)//1027 MINIMAL FRESH AIR, UNIT PERCENTAGE
				{
					temp1 = 0;
					temp2 =  FrcMinFreshAir;

				}
				else if(address == FRC_MIN_FRESH_AIRTEM)//1028 MINIMAL FRESH AIR TEMPERATURE, UNIT DEGREE C/F
				{
					temp1 = FrcMinFreshAirTemHi;
					temp2 = FrcMinFreshAirTemLow ;

				}
				else if(address == FRC_OUTPUT_SELECT)
				{
					temp1 = 0;
					temp2 = frc_output_select;
				}
				else if(address == FRC_OUTPUT_CONFIG)  //1029 FREE COOLING OUTPUT CHECK, OUTPUT SOURCE, OUTPUT FUNCTION
				{
					temp1 = 0;
					temp2 =  FrcOutputConfig;

				}
				else if(address == FRC_PID2_CONFIG)   //1030 FREE COOLING PID2 CONFIG CHECK, CHECK IF PID2 HAS INPUT FROM SUPPLY SENSOR, ALSO CHECK IF IT IS SET TO NEGATIVE COOLING
				{
					temp1 = 0;
					temp2 = FrcPid2Config ;

				}
//				else if(address == FRC_COOLING_DEADBAND)//1031 FREE COOLING DEADBAND UNIT: DEGREE C/F
//				{
//					temp1 = 0;
//					temp2 = FrcCoolDeadband ;
//
//				}
				else if(address == FRC_OUTPUT_MODE)  //1032 SET FAN TO OFF
				{
					temp1 = 0;
					temp2 = FrcOutputMode;

				}
//				else if(address == FRC_FULL_FRESH)//1033 SET FULL FRESH AIR INPUT
//				{
//					temp1 = 0;
//					temp2 = FrcFullFresh;
//
//				}
				else if(address == FRC_TOTAL_CONFIG)//1034 MINIMAL SUPPLY AIR TEMPERATURE UNIT: DEGREE C/F
				{
					temp1 = 0;//FrcTotalConfig;
					temp2 = FrcTotalConfig;

				}
				else if(address == FRC_PRESENT_MODE)//1035
				{
					temp1 = 0;
					temp2 = FrcPresentMode;
				}
				else if(address == FRC_OUTDOOR_ENTHALPY)
				{
					temp1 = (frc_outdoorhum_enthalpy >> 8) & 0xff;
					temp2 = frc_outdoorhum_enthalpy & 0xff;
				}
				else if(address == FRC_INDOOR_ENTHALPY)
				{
					temp1 = (frc_indoorhum_enthalpy >> 8) & 0xff;
					temp2 = frc_indoorhum_enthalpy & 0xff;
				}
				else if(address == FRC_PID2_VALUE)
				{
					temp1 = 0;
					temp2 = pid[1] & 0xff;
				}
				else if(address == BACNET_STATION_NUM)
				{
					temp1 = 0;
					temp2 = Station_NUM;
				}

				else if(address == BACNET_INSTANCE_LOW)
				{
					temp1 = (Instance >> 8) & 0XFF;
					temp2 = Instance & 0xff;
				}
				else if(address == BACNET_INSTANCE_HIGH)
				{
					temp1 = (Instance >> 24) & 0XFF;
					temp2 = (Instance>>16) & 0xff;
				}

				else if(address == MODBUS_4TO20MA_BOTTOM)
				{
					temp1 = bottom_of_4to20ma >> 8;
					temp2 = bottom_of_4to20ma & 0xff;
				}

				else if(address == MODBUS_4TO20MA_TOP)
				{
					temp1 = top_of_4to20ma >> 8;
					temp2 = top_of_4to20ma & 0xff;
				}

				else if(address == MODBUS_4TO20MA_UNIT_HI)
				{
					temp1 = read_eeprom(EEP_4TO20MA_UNIT_HI + 1);
					temp2 = read_eeprom(EEP_4TO20MA_UNIT_HI);
				}

				else if(address == MODBUS_4TO20MA_UNIT_LO)
				{
					temp1 = read_eeprom(EEP_4TO20MA_UNIT_LO + 1);
					temp2 = read_eeprom(EEP_4TO20MA_UNIT_LO);
				}
				else if((address >= VOC_BASELINE1)	&& (address <= VOC_BASELINE4))
				{
					temp1 = 0;
					temp2 = voc_baseline(address - VOC_BASELINE1);
				}
				else if(address == VOC_DATA)
				{
					temp1 = (voc_value >> 8) & 0xff;
					temp2 = voc_value & 0xff;
				}
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

//				else if((address >= BAC_T1) && (address <= BAC_T8))
//				{
//					temp1 = 0;
//					temp2 = read_eeprom(BAC_TEST1 + address - BAC_T1);
//				}

				else
					{
					temp1 = 0 ;
					temp2 = 0;
					}
					if(uartsel == 0)//
					{
						uart_send[send_cout++] = temp1 ;
						uart_send[send_cout++] = temp2 ;
						crc16_byte(temp1);
						crc16_byte(temp2);
					}
					else
					{
//						#ifdef TSTAT_WIFI
						uart_sendB[UIP_HEAD + send_cout++] = temp1 ;
						uart_sendB[UIP_HEAD + send_cout++] = temp2 ;
//						#else
//						uart_sendB[send_cout++] = temp1 ;
//						uart_sendB[send_cout++] = temp2 ;
//						crc16_byte(temp1);
//						crc16_byte(temp2);
//						#endif //TSTAT_WIFI
					}

				}

			TransID =  (u16)(*(bufadd) << 8) | (*(bufadd + 1));
			uart_sendB[0] = TransID >> 8;			//	TransID
			uart_sendB[1] = TransID;
			uart_sendB[2] = 0;			//	ProtoID
			uart_sendB[3] = 0;
			uart_sendB[4] = (3 + num * 2) >> 8;	//	Len
			uart_sendB[5] = (U8_T)(3 + num * 2) ;
		}


	else if(*(bufadd+1 + headlen) == CHECKONLINE)
	{
	if(uartsel == 0)
	{
		uart_send[send_cout++] = *(bufadd + headlen) ; //0xff
		uart_send[send_cout++] = *(bufadd+1 + headlen) ;	//0x19
	}
	else
	{
		uart_sendB[send_cout++] = *(bufadd + headlen) ; //0xff
		uart_sendB[send_cout++] = *(bufadd+1 + headlen) ;	//0x19
	}
	crc16_byte(*(bufadd + headlen));
	crc16_byte(*(bufadd+1 + headlen));

	if(uartsel == 0)
		uart_send[send_cout++] = laddress;					//modbus ID
	else
		uart_sendB[send_cout++] = laddress;					//modbus ID

		crc16_byte(laddress);
	if(uartsel == 0)
	{
		uart_send[send_cout++] = SerialNumber(0);//serialnumber[0];		//SN first byte
		uart_send[send_cout++] = SerialNumber(1);//serialnumber[1];		//SN second byte
		uart_send[send_cout++] = SerialNumber(2);//serialnumber[2];		//SN third byte
		uart_send[send_cout++] = SerialNumber(3);//serialnumber[3];		//SN fourth byte
	}
	else
	{
		uart_sendB[send_cout++] = SerialNumber(0);//serialnumber[0];		//SN first byte
		uart_sendB[send_cout++] = SerialNumber(1);//serialnumber[1];		//SN second byte
		uart_sendB[send_cout++] = SerialNumber(2);//serialnumber[2];		//SN third byte
		uart_sendB[send_cout++] = SerialNumber(3);//serialnumber[3];		//SN fourth byte
	}
		crc16_byte(SerialNumber(0));
		crc16_byte(SerialNumber(1));
		crc16_byte(SerialNumber(2));
		crc16_byte(SerialNumber(3));
	}

	else
			return;

	temp1 = CRChi ;
	temp2 = CRClo;

		if(type == WIFI)
		{
			memcpy(modbus_wifi_buf,uart_sendB,UIP_HEAD + send_cout);
			modbus_wifi_len = UIP_HEAD + send_cout;
		}
		else
		{

			uart_send[send_cout++] = temp1 ;
			uart_send[send_cout++] = temp2 ;
			if(type == BAC_TO_MODBUS)
			{
				//memcpy(&bacnet_to_modbus,&uart_send[3],reg_num*2);
//				Test[0]++;
				memcpy(&bacnet_to_modbus,&uart_send[3],reg_num*2);
			}
			else
				USART_SendDataString(send_cout);
		}

}




uint8 checkdata(void)
{
uint16 xdata crc_val ;
uint8 i;
uint8 minaddr,maxaddr, variable_delay;

if((USART_RX_BUFB[0] != 255) && (USART_RX_BUFB[0] != laddress) && (USART_RX_BUFB[0] != 0))
	return false;

if(USART_RX_BUFB[1] != READ_VARIABLES && USART_RX_BUFB[1] != WRITE_VARIABLES && USART_RX_BUFB[1] != MULTIPLE_WRITE_VARIABLES && USART_RX_BUFB[1] != CHECKONLINE)
	return false;

if(USART_RX_BUFB[1] == CHECKONLINE)
	{
	crc_val = crc16(USART_RX_BUFB,4);
	if(crc_val != USART_RX_BUFB[4]*256 + USART_RX_BUFB[5])
		{
		return false;
		}
	minaddr = (USART_RX_BUFB[2] >= USART_RX_BUFB[3] ) ? USART_RX_BUFB[3] : USART_RX_BUFB[2] ;
	maxaddr = (USART_RX_BUFB[2] >= USART_RX_BUFB[3] ) ? USART_RX_BUFB[2] : USART_RX_BUFB[3] ;
	if(laddress < minaddr || laddress > maxaddr)
		return false;
	else
		{	// in the TRUE case, we add a random delay such that the Interface can pick up the packets
		//srand();
		variable_delay = rand() % 10;

		for (i=0; i<variable_delay; i++)
			{
			if(EEP_Baudrate == 0)
				delay_ms(5);
			else if(EEP_Baudrate == 1)
				delay_ms(4);
			else if(EEP_Baudrate == 2)
				delay_ms(3);
			else if(EEP_Baudrate == 3)
				delay_ms(2);
			else if(EEP_Baudrate == 4)
				delay_ms(1);
			else
				delay_ms(10);
			}
		return true;
		}

	}

if(USART_RX_BUFB[2]*256 + USART_RX_BUFB[3] ==  PLUG_N_PLAY)

{
	if(USART_RX_BUFB[4] == 0x55)  //if this is a plug and play write command
		{
		if(USART_RX_BUFB[6] != SerialNumber(0))
		return FALSE;
		if(USART_RX_BUFB[7] != SerialNumber(1))
		return FALSE;
		if(USART_RX_BUFB[8] != SerialNumber(2))
		return FALSE;
		if(USART_RX_BUFB[9] != SerialNumber(3))
		return FALSE;
		}
}

return true;
}
*/
