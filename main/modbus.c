#include <stdio.h>
#include "esp_err.h"

#include "mbcontroller.h"
#include "define.h"
#include "esp_log.h"            // for log_write
#include "driver/gpio.h"
#include "modbus.h"
#include "i2c_task.h"
#include "flash.h"
#include "wifi.h"
#include "bacnet.h"
#include "commsub.h"
#include "scan.h"
#include "user_data.h"
#include "rtc.h"
#include "rs485.h"
#include "airlab.h"
#include "flash.h"
#include "controls.h"
#include "lcd.h"
#include "sntp_app.h"




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

#define GPIO_SUB_EN_PIN		GPIO_NUM_13
#define GPIO_MAIN_EN_PIN 	GPIO_NUM_2

//#define SUB_EN_PIN_SEL	(1ULL<<GPIO_SUB_EN_PIN)
//#define MAIN_EN_PIN_SEL	(1ULL<<GPIO_MAIN_EN_PIN)
//static const char *TAG = "MODBUS_SLAVE_APP";
//uint8_t uart_sendB[512];
const int uart_num_sub = UART_NUM_0;
const int uart_num_main = UART_NUM_2;
STR_MODBUS Modbus;
U8_T SNWriteflag;
U8_T update_flash;
//extern U32_T Instance;
U8_T CRChi = 0xff;
U8_T CRClo = 0xff;

uint8_t Eth_IP_Change = 0;
uint8_t ip_change_count = 0;

//uint8_t flag_uart_reading[3];
extern lcdconfig display_lcd;

extern uint8_t gIdentify;
extern uint8_t count_gIdentify;
extern uint16_t input_cal[24];
extern U8_T lcddisplay[7];

void set_output_raw(U8_T point,U16_T value);
void Sync_timestamp(S16_T newTZ,S16_T oldTZ,S8_T newDLS,S8_T oldDLS);
void dealwith_write_setting(Str_Setting_Info * ptr);
uint16_t read_user_data_by_block(uint16_t addr);
uint16_t read_tstat10_data_by_block(uint16_t addr);
uint8_t flag_change_uart0 = 0;
uint8_t flag_change_uart2 = 0;
uint8_t count_change_uart0 = 0;
uint8_t count_change_uart2 = 0;
void Check_change_uart(void)
{
	if(flag_change_uart0 == 1)
	{
		if(count_change_uart0++ > 3)
		{
			flag_change_uart0 = 0;
			count_change_uart0 = 0;
			uart_init(0);
			Count_com_config();
		}

	}

	if(flag_change_uart2 == 1)
	{
		if(count_change_uart2++ > 3)
		{
			flag_change_uart2 = 0;
			count_change_uart2 = 0;
			uart_init(2);
			Count_com_config();
		}

	}
}


extern U8_T 	bacnet_to_modbus[300];
#define MAX_REG 50
char reg_name[MAX_REG][15] = {
		/*"MAC_ADDR_1",// = 60,
		"MAC_ADDR_2",//,
		"MAC_ADDR_3",
		"MAC_ADDR_4",
		"MAC_ADDR_5",
		"MAC_ADDR_6", //65
		"IP_MODE", //66*/
		"IP_ADDR_1",
		"IP_ADDR_2",
		"IP_ADDR_3",
		"IP_ADDR_4",
		"IP_SUB_MASK_1",//71
		"IP_SUB_MASK_2",
		"IP_SUB_MASK_3",
		"IP_SUB_MASK_4",
		"IP_GATE_WAY_1",
		"IP_GATE_WAY_2",
		"IP_GATE_WAY_3",
		"IP_GATE_WAY_4",

};


uint16_t get_reg_value(uint16_t index)
{
	uint16_t value = 0;
	index = index + 7;
	switch(index)
	{
	/*case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		value = SSID_Info.mac_addr[index];
		break;
	case 6:
		break;*/
	case 7:
	case 8:
	case 9:
	case 10:
		value = SSID_Info.ip_addr[index - 7];
		break;
	case 11:
	case 12:
	case 13:
	case 14:
		value = SSID_Info.net_mask[index - 11];
		break;
	case 15:
	case 16:
	case 17:
	case 18:
		value = SSID_Info.getway[index - 15];
		break;
	default:
		break;
	}
	return value;
};


uint16_t Write_reg_value(uint16_t index,uint16_t value)
{
	//uint16_t value = 0;
	index = index + 7;
	switch(index)
	{
	/*case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		value = SSID_Info.mac_addr[index];
		break;
	case 6:
		break;*/
	case 7:
	case 8:
	case 9:
	case 10:
		SSID_Info.ip_addr[index - 7] = value;
		break;
	case 11:
	case 12:
	case 13:
	case 14:
		SSID_Info.net_mask[index - 11] = value;
		break;
	case 15:
	case 16:
	case 17:
	case 18:
		SSID_Info.getway[index - 15] = value;
		break;
	default:
		break;
	}
	return value;
};

uint16_t read_wifi_data_by_block(uint16_t addr);
// Table of CRC values for high??order byt
U8_T const auchCRCHi[256] = {
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};

// Table of CRC values for low Corder byte
U8_T const auchCRCLo[256] = {
	0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04,
	0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8,
	0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
	0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3, 0x11, 0xD1, 0xD0, 0x10,
	0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
	0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
	0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C,
	0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26, 0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0,
	0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
	0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
	0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C,
	0xB4, 0x74, 0x75, 0xB5, 0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
	0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54,
	0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98,
	0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
	0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};


void init_crc16(void)
{
	CRClo = 0xFF;
	CRChi = 0xFF;
}
//calculate crc with one byte
void crc16_byte(uint8_t ch)
{
	U8_T uIndex;
	uIndex = CRChi^ch; // calculate the CRC
	CRChi = CRClo^auchCRCHi[uIndex];
	CRClo = auchCRCLo[uIndex];
}

uint16_t crc16(uint8_t *p, uint8_t length)
{
	U16_T uchCRCHi = 0xff;	// high byte of CRC initialized
	U8_T uchCRCLo = 0xff;	// low byte of CRC initialized
	U8_T uIndex;			// will index into CRC lookup table
	U8_T i = 0;

	while(length--)//pass through message buffer
	{
		uIndex = uchCRCHi^p[i++]; // calculate the CRC
		uchCRCHi = uchCRCLo^auchCRCHi[uIndex] ;
		uchCRCLo = auchCRCLo[uIndex];
	}
	return (uchCRCHi << 8 | uchCRCLo);
}

// Set register values into known state
static void setup_reg_data()
{
    // Define initial state of parameters
   /* discrete_reg_params.discrete_input1 = 1;
    discrete_reg_params.discrete_input3 = 1;
    discrete_reg_params.discrete_input5 = 1;
    discrete_reg_params.discrete_input7 = 1;*/

    //holding_reg_params.serial_number_lo = 25;
    //holding_reg_params.serial_number_hi = 0;
 //   Modbus.version_number_lo = 26;
//    Modbus.version_number_hi = 0;
    //holding_reg_params.modbus_address = MB_DEV_ADDR;
 //   if(Modbus.p == PROJECT_FAN_MODULE)
//    	Modbus.product_model = 97;//62;//
 //   else
    	Modbus.product_model = 88;
    Modbus.hardRev = 2;
   // Modbus.readyToUpdate = 0;

  /*  coil_reg_params.coil0 = 1;
    coil_reg_params.coil2 = 1;
    coil_reg_params.coil4 = 1;
    coil_reg_params.coil6 = 1;
    coil_reg_params.coil7 = 1;

    input_reg_params.data_chan0 = 1.34;
    input_reg_params.data_chan1 = 2.56;
    input_reg_params.data_chan2 = 3.78;
    input_reg_params.data_chan3 = 4.90;*/
}


//void* mbc_slave_handler = NULL;
//mb_communication_info_t comm_info;
//mb_register_area_descriptor_t reg_area; // Modbus register area descriptor structure
//mb_communication_info_t tcp_info;

void uart_init(uint8_t uart)
{
	int baud;
	switch(Modbus.baudrate[uart])
	{
	case UART_1200:
		baud = 1200;
		break;
	case UART_2400:
		baud = 2400;
		break;
	case UART_3600:
		baud = 3600;
		break;
	case UART_4800:
		baud = 4800;
		break;
	case UART_7200:
		baud = 7200;
		break;
	case UART_9600:
		baud = 9600;
		break;
	case UART_19200:
		baud = 19200;
		break;
	case UART_38400:
		baud = 38400;
		break;
	case UART_57600:
		baud = 57600;
		break;
	case UART_76800:
		baud = 76800;
		break;
	case UART_115200:
		baud = 115200;
		break;
	case UART_921600:
		baud = 921600;
		break;
	default:
		baud = 115200;
		break;
	}
	if((Modbus.mini_type == PROJECT_AIRLAB) && (uart == 2))
	{
		baud = 115200;
	}
//	mb_communication_info_t tcp_info; // Modbus communication parameters

	    uart_config_t uart_config = {
	        .baud_rate = baud,
	        .data_bits = UART_DATA_8_BITS,
	        .parity = UART_PARITY_DISABLE,
	        .stop_bits = UART_STOP_BITS_1,
	        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	       // .rx_flow_ctrl_thresh = 122,
	    };
// Configure UART parameters
	if(uart == 0) // sub port
	{
		uart_param_config(uart_num_sub, &uart_config);
		uart_set_pin(uart_num_sub, GPIO_NUM_1, GPIO_NUM_3, GPIO_SUB_EN_PIN, UART_PIN_NO_CHANGE);
		uart_driver_install(uart_num_sub, MB_BUF_SIZE * 2, 0, 0, NULL, 0);
		uart_set_mode(uart_num_sub, UART_MODE_RS485_HALF_DUPLEX);
	}
	else if(uart == 2)//  (uart == 1) main port
	{
		if(Modbus.mini_type == PROJECT_FAN_MODULE)
		{
			uart_param_config(uart_num_main, &uart_config);
			uart_set_pin(uart_num_main, GPIO_NUM_12, GPIO_NUM_15, GPIO_MAIN_EN_PIN, UART_PIN_NO_CHANGE);
			uart_driver_install(uart_num_main, MB_BUF_SIZE * 2, 0, 0, NULL, 0);
			uart_set_mode(uart_num_main, UART_MODE_RS485_HALF_DUPLEX);
		}
		else if(Modbus.mini_type == PROJECT_AIRLAB)  // PM2.5
		{
			uart_param_config(uart_num_main, &uart_config);
			uart_set_pin(uart_num_main, GPIO_NUM_12, GPIO_NUM_15, 0, UART_PIN_NO_CHANGE);
			uart_driver_install(uart_num_main, MB_BUF_SIZE * 2, 0, 0, NULL, 0);
			uart_set_mode(uart_num_main, UART_MODE_RS485_HALF_DUPLEX);
		}
		else
		{
			uart_param_config(uart_num_main, &uart_config);
			uart_set_pin(uart_num_main, GPIO_NUM_12, GPIO_NUM_15, GPIO_MAIN_EN_PIN, UART_PIN_NO_CHANGE);
			uart_driver_install(uart_num_main, MB_BUF_SIZE * 2, 0, 0, NULL, 0);
			uart_set_mode(uart_num_main, UART_MODE_RS485_HALF_DUPLEX);
		}
	}
}



bool checkdata(uint8_t* data,uint16_t len)
{
	uint16_t crc_val ;
	if((data[0] != 255) && (data[0] != Modbus.address) && (data[0] != 0))
		return false;

	if(data[1] != READ_VARIABLES
			&& data[1] != WRITE_VARIABLES
			&& data[1] != MULTIPLE_WRITE_VARIABLES && data[1] != CHECKONLINE
			&& data[1] != TEMCO_MODBUS
			)
		return false;

	if(data[1] == CHECKONLINE)
	{
		crc_val = crc16(data,4);
		if(crc_val != data[4]*256 + data[5])
		{
			return false;
		}
		else
			return true;
	}

	crc_val = crc16(data, len - 2);
	if(crc_val == ((U16_T)data[len - 2] << 8 | data[len - 1]))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}

}
void debug_info(char *string);



extern uint8 led_sub_tx;
extern uint8 led_sub_rx;
extern uint8 led_main_tx;
extern uint8 led_main_rx;
void uart0_rx_task(void)
{
//	uint8_t modbus_send_buf[500];
//	uint16_t modbus_send_len;
//	memset(modbus_send_buf,0,500);
//	modbus_send_len = 0;
	//uint8_t testCmd[8] = {0xff,0x03,0x00,0x00,0x00,0x64,0x51,0xff};
	//uint8_t i;
	//mb_param_info_t reg_info; // keeps the Modbus registers access information
	//printf("MODBUS_TASK&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\r\n");
	// Configure a temporary buffer for the incoming data
	//uint8_t *data = (uint8_t *) malloc(MB_BUF_SIZE);
	//char* test_str = "This is a test string.\n";
//	char prefix[] = "RS485 Received: [";
	//modbus_init();
	setup_reg_data();
	//uint8_t* uart_rsv = (uint8_t*)malloc(512);
	uint8_t uart_rsv[512];

	debug_info("modbous initial \r\n");

	task_test.enable[7] = 1;
	while (1) {
		task_test.count[7]++;

			if(Modbus.com_config[0] == MODBUS_SLAVE)
			{
				int len = uart_read_bytes(uart_num_sub, uart_rsv, 512, 100 / portTICK_RATE_MS);

				if(len > 0)
				{led_sub_rx++;
					com_rx[0] += len;
					flagLED_sub_rx = 1;
					//flag_debug_rx = 1; memcpy(udp_debug_str,uart_rsv,len); debug_rx_len = len;

					if(checkdata(uart_rsv,len))
					{
						if(uart_rsv[1] == TEMCO_MODBUS)	// temco private modbus
						{
							if(uart_rsv[0] ==  Modbus.address || uart_rsv[0] == 255)
							{
								handler_private_transfer(uart_rsv,0,NULL,0xa0);
							}
						}
						else
						{
							if(uart_rsv[0] ==  Modbus.address || uart_rsv[0] == 255)
							{
								init_crc16();
								responseModbusCmd(SERIAL, uart_rsv, len,modbus_send_buf,&modbus_send_len,0);
							}
						}
					}
				}

			}
			else if(Modbus.com_config[0] == BACNET_MASTER)
			{
				if(system_timer / 1000 > 10)
				{
					int len = uart_read_bytes(UART_NUM_0, uart_rsv, 512, 100 / portTICK_RATE_MS);

					if(len > 0)
					{
						led_sub_rx++;
						com_rx[0] += len;
						flagLED_sub_rx = 1;
						//flag_debug_rx = 1; memcpy(udp_debug_str,uart_rsv,len); debug_rx_len = len;
						Timer_Silence_Reset();

					}
				}
				else
					vTaskDelay(500 / portTICK_RATE_MS);
			}
			else
			{
				if((Modbus.com_config[0] == 0) /*|| ((Modbus.com_config[0] == MODBUS_MASTER) && (com_rx[0] == 0))*/)
				{
					int len = uart_read_bytes(uart_num_sub, uart_rsv, 50, 10 / portTICK_RATE_MS);

					if(len>0)
					{
						led_sub_rx++;
						com_rx[0] += len;
						flagLED_sub_rx = 1;
						//flag_debug_rx = 1; memcpy(udp_debug_str,uart_rsv,len); debug_rx_len = len;
						if(checkdata(uart_rsv,len))
						{
							Modbus.com_config[0] = MODBUS_SLAVE;
						}
					}
				}
				else
					vTaskDelay(50 / portTICK_RATE_MS);
			}

	}

}

void uart2_rx_task(void)
{
	//uint8_t modbus_send_buf[500];
	//uint16_t modbus_send_len;
	//memset(modbus_send_buf,0,500);
	//modbus_send_len = 0;
	setup_reg_data();
	//uint8_t* uart_rsv = (uint8_t*)malloc(512);
	uint8_t uart_rsv[512];
	task_test.enable[10] = 1;
	while (1) {
		task_test.count[10]++;
		if(Modbus.com_config[2] == MODBUS_SLAVE)
		{
			int len = uart_read_bytes(uart_num_main, uart_rsv, 512, 100 / portTICK_RATE_MS);

			if(len>0)
			{
				led_main_rx++;
				com_rx[2] += len;
				flagLED_main_rx = 1;

				if(checkdata(uart_rsv,len))
				{
					if(uart_rsv[1] == TEMCO_MODBUS)	// temco private modbus
					{
						if(uart_rsv[0] ==  Modbus.address || uart_rsv[0] == 255)
							handler_private_transfer(uart_rsv,0,NULL,0xa2);
					}
					else
					{
						if(uart_rsv[0] ==  Modbus.address || uart_rsv[0] == 255)
						{
							init_crc16();
							responseModbusCmd(SERIAL, uart_rsv, len,modbus_send_buf,&modbus_send_len,2);
						}
					}
				}
			}

		}
		else if(Modbus.com_config[2] == BACNET_MASTER)
		{
			if(system_timer / 1000 > 10)
			{
				int len = uart_read_bytes(UART_NUM_2, uart_rsv, 512, 100 / portTICK_RATE_MS);

				if(len > 0)
				{
					led_main_rx++;
					com_rx[2] += len;
					flagLED_main_rx = 1;
					Timer_Silence_Reset();
				}
			}
			else
				vTaskDelay(500 / portTICK_RATE_MS);
		}
		else
		{
			if((Modbus.com_config[2] == 0)/* || ((Modbus.com_config[2] == MODBUS_MASTER) && (com_rx[2] == 0))*/)
			{
				int len = uart_read_bytes(uart_num_main, uart_rsv, 50, 10 / portTICK_RATE_MS);

				if(len>0)
				{
					led_main_rx++;
					com_rx[2] += len;
					flagLED_main_rx = 1;
					if(checkdata(uart_rsv,len))
					{
						Modbus.com_config[2] = MODBUS_SLAVE;

					}
				}
			}
			else
				vTaskDelay(500 / portTICK_RATE_MS);
		}
	}
}

void responseModbusData(uint8_t  *bufadd, uint8_t type, uint16_t rece_size,uint8_t *resData ,uint16_t *modbus_send_len,uint8_t port)
{
   uint8_t num = 0, i, temp1, temp2;
   uint16_t temp;
   uint16_t send_cout = 0 ;
   uint16_t address;
   uint8_t headlen = 0;
   uint16_t TransID;
   uint8_t *uart_sendB = NULL;
   uint8_t *uart_send = NULL;
   //char test;

   if(type == WIFI)
   {
      headlen = UIP_HEAD;
      uart_sendB = malloc(512);
   }
   else
   {
      headlen = 0;
      uart_send = malloc(512);
   }
   if(*(bufadd + 1 + headlen) == WRITE_VARIABLES)
   {
      if(type == WIFI){ // for wifi
         for(i = 0; i < 6; i++)
            uart_sendB[UIP_HEAD + send_cout++] = *(bufadd + i + headlen);

         uart_sendB[0] = *bufadd;//0;         //   TransID
         uart_sendB[1] = *(bufadd + 1);//TransID++;
         uart_sendB[2] = 0;         //   ProtoID
         uart_sendB[3] = 0;
         uart_sendB[4] = 0;   //   Len
         uart_sendB[5] = 6;

         memcpy(resData,uart_sendB,UIP_HEAD + send_cout);
         *modbus_send_len = UIP_HEAD + send_cout;
      }
	  else
	  {
         //uart_write_bytes(UART_NUM_0, "WRITE_VARIABLES\r\n", sizeof("WRITE_VARIABLES\r\n"));
         for(i = 0; i < rece_size; i++)
            uart_send[send_cout++] = *(bufadd+i);
         if(type == BAC_TO_MODBUS)
         {
        	memcpy(&bacnet_to_modbus,&bufadd[3],bufadd[5] * 2);
         }
         else if(type == SERIAL) // MODBUS
         {
            uart_send_string((const char *)uart_send, send_cout,port);
         }
      }
   }
   else if(*(bufadd + 1 + headlen) == MULTIPLE_WRITE)
   {
      //init_crc16();
      for(i = 0; i < 6; i++)
      {
          if(type != WIFI)
               uart_send[send_cout++] = *(bufadd+i) ;
          else
          {
               uart_sendB[UIP_HEAD + send_cout++] = *(bufadd + i + headlen);

          }
         crc16_byte(*(bufadd+i));
      }

      
		if(type == BAC_TO_MODBUS)
		{
			memcpy(&bacnet_to_modbus,&uart_send[3],reg_num*2);
		}
		else if(type == SERIAL)
		{
			uart_send[6] = CRChi;
			uart_send[7] = CRClo;
			uart_send_string((const char *)uart_send, 8,port);

		}      
		else  // WIFI OR ETHERNET
		{
			 uart_sendB[0] = *bufadd;//0;         //   TransID
			 uart_sendB[1] = *(bufadd + 1);//TransID++;
			 uart_sendB[2] = 0;         //   ProtoID
			 uart_sendB[3] = 0;
			 uart_sendB[4] = 0;   //   Len
			 uart_sendB[5] = 6;
			 memcpy(resData,uart_sendB,UIP_HEAD + send_cout);
			*modbus_send_len = UIP_HEAD + send_cout;
		}
   }
   else if(*(bufadd + 1 + headlen) == READ_VARIABLES)
   {
      num = *(bufadd+5 + headlen);
      if(type == SERIAL || type == BAC_TO_MODBUS)
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

          if((address >= MODBUS_AIRLAB_REG_START) &&
                 		 (address <= MODBUS_AIRLAB_REG_START + 2000))
		  {
			if(Modbus.mini_type == PROJECT_AIRLAB)
			{
				// run specail register
				 U16_T temp;
				 temp = read_airlab_by_block(address);
				 temp1 = (temp>>8) & 0xff;
				 temp2 = temp & 0xff;
			}
			else
			{
				temp1 = 0;
				temp2 = 0;
			}
		  }
          else
         if(address == SERIALNUMBER_LOWORD )
         {
            temp1 = 0 ;
            temp2 = (uint8_t)Modbus.serialNum[0];
         }
         else if(address == (SERIALNUMBER_LOWORD +1))
         {
            temp1 = 0 ;
            temp2 = (uint8_t)(Modbus.serialNum[1]);
         }
         else if(address == SERIALNUMBER_HIWORD)
         {
            temp1 = 0 ;
            temp2 = Modbus.serialNum[2];
         }
         else if(address == (SERIALNUMBER_HIWORD +1))
         {
            
            temp1 = 0;
            temp2 = Modbus.serialNum[3];
         }
         else if(address == VERSION_NUMBER_LO)
         {            
            temp1 = 0;
            temp2 = SW_REV % 100;
         }
         else if(address == VERSION_NUMBER_HI)
         {            
            temp1 = 0;
            temp2 = SW_REV / 100;
         }
         else if(address == MODBUS_ADDRESS)
         {
            temp1 = 0; ;
            temp2 = Modbus.address & 0xff;
         }
         else if(address == PRODUCT_MODEL)
         {
            temp1 = 0 ;
            temp2 = Modbus.product_model&0xff;
         }
         else if(address == HARDWARE_REV)
         {
            temp1 = 0 ;
            temp2 = Modbus.hardRev ;
         }
         else if(address == MODBUS_ISP_VER)
         {
        	 temp1 = 0 ;
        	 temp2 = Modbus.IspVer ;
         }
         else if(address == MODBUS_UART0_BAUDRATE)
         {
            temp1 = 0;
            temp2 = Modbus.baudrate[0];
         }
         else if(address == MODBUS_UART2_BAUDRATE)
		  {
			 temp1 = 0;
			 temp2 = Modbus.baudrate[2];
		  }
         else if(address == MODBUS_ETHERNET_STATUS)
         {
            temp1 = 0;
            temp2 = Modbus.ethernet_status;
         }
         else if(address == MODBUS_ENABLE_DEBUG)
		  {
			 temp1 = 0;
			 temp2 = Modbus.enable_debug;
		  }
         else if((address >= MODBUS_IN1_CAL)&&(address <= MODBUS_IN16_CAL))
		 {
			temp1 = (input_cal[address - MODBUS_IN1_CAL] >> 8) & 0xff;
			temp2 = input_cal[address - MODBUS_IN1_CAL] & 0xff;
		 }
         else if((address >= MODBUS_TEST_1)&&(address < MODBUS_TEST_50))
         {
            temp1 = (Test/*holding_reg_params.testBuf*/[address-MODBUS_TEST_1]>>8)&0xff;
            temp2 = Test/*holding_reg_params.testBuf*/[address-MODBUS_TEST_1]&0xff;
         }
         else if(address == MODBUS_COM0_TYPE)
		{
        	 temp1 = 0;
        	 temp2 = Modbus.com_config[0];
		}
        /* else if(address == MODBUS_COM1_TYPE)
		{
			 temp1 = 0;
			 temp2 = Modbus.com_config[1];
		}*/
         else if(address == MODBUS_COM2_TYPE)
		{
			 temp1 = 0;
			 temp2 = Modbus.com_config[2];
		}
		else if(address == MODBUS_INSTANCE_LO)
		{
			temp1 = (U8_T)(Instance >> 8);
			temp2 = (U8_T)Instance;
		}
		else if(address == MODBUS_MINI_TYPE)
		{
			temp1 = 0;
			temp2 = Modbus.mini_type;
		}
		else if(address == MODBUS_INSTANCE_HI)
		{
			temp1 = (U8_T)(Instance >> 24);
			temp2 = (U8_T)(Instance >> 16);
		}
		else if(address == MODBUS_PANEL_NUMBER)
		{
			temp1 = 0;
			temp2 = Modbus.address;
		}
		else if(address == MODBUS_STATION_NUM)
		{
			temp1 = 0;
			temp2 = Modbus.address;
		}
		else if(address == MODBUS_NETWORK_NUMBER)
		{
			temp1 = Modbus.network_number >> 8;
			temp2 = Modbus.network_number;
		}
		else if(address == MODBUS_MSTP_NETWORK)
		{
			temp1 = Modbus.mstp_network >> 8;
			temp2 = Modbus.mstp_network;
		}
		else if(address == MODBUS_TIME_ZONE)
		{
			temp1 = timezone >> 8;
			temp2 = timezone;
		}
		else if(address == MODBUS_DSL)
		{
			temp1 = 0;
			temp2 = Daylight_Saving_Time;
		}
		else if(address == MODBUS_SNTP_EN)
		{
			temp1 = 0;
			temp2 =  Modbus.en_sntp;
		}
		else if(address == MODBUS_TCP_PORT)
		{
			temp1 = Modbus.tcp_port >> 8;
			temp2 = Modbus.tcp_port;
		}
		else if(address == MODBUS_READ_POINT_TIMER)
		{
			temp1 = READ_POINT_TIMER >> 8;
			temp2 = READ_POINT_TIMER;
		}
		else if(address == IP_MODE)
		{
			temp1 = 0;
			temp2 = Modbus.tcp_type;
		}
		else if(address == MODBUS_DEAD_MASTER_FOR_PLC)
		{
			temp1 = 0;
			temp2 = Modbus.dead_master_for_PLC;
		}
         else if((address >= MAC_ADDR_1) && (address <= MAC_ADDR_6))
         {
            temp1 = 0;
            temp2 = Modbus.mac_addr[address - MAC_ADDR_1];
         }
         else if((address >= IP_ADDR_1) && (address <= IP_ADDR_4))
         {
            temp1 = 0;
            temp2 = Modbus.ip_addr[address - IP_ADDR_1];
         }
         else if((address >= IP_SUB_MASK_1)&& (address <= IP_SUB_MASK_4))
         {
            temp1 = 0;
            temp2 = Modbus.subnet[address - IP_SUB_MASK_1];
         }

         else if((address >= IP_GATE_WAY_1)&& (address <= IP_GATE_WAY_4))
         {
            temp1 = 0;
            temp2 = Modbus.getway[address - IP_GATE_WAY_1];
         }
         else if(address >= MODBUS_TIMER_ADDRESS && address <= MODBUS_TIMER_ADDRESS + 7)
		 {
        	if(address - MODBUS_TIMER_ADDRESS == 0)  // second
			{
				temp1 = 0;
				temp2 =  rtc_date.second;
			}
        	else if(address - MODBUS_TIMER_ADDRESS == 1)  // minute
			{
				temp1 = 0;
				temp2 =  rtc_date.minute;
			}
        	else if(address - MODBUS_TIMER_ADDRESS == 2)  // hour
			{
				temp1 = 0;
				temp2 =  rtc_date.hour;
			}
        	else if(address - MODBUS_TIMER_ADDRESS == 3)  // day
			{
				temp1 = 0;
				temp2 =  rtc_date.day;
			}
			else if(address - MODBUS_TIMER_ADDRESS == 4)  // weekday
			{
				temp1 = 0;
				temp2 =  rtc_date.weekday;
			}
			else if(address - MODBUS_TIMER_ADDRESS == 5)  // month
			{
				temp1 = 0;
				temp2 =  rtc_date.month;
			}
        	else if(address - MODBUS_TIMER_ADDRESS == 6)
			{ // day of year
        		temp1 = rtc_date.year >> 8;
				temp2 =  rtc_date.year;
			}
        	else if(address - MODBUS_TIMER_ADDRESS == 7)//day of year
			{ // day of year
				temp1 = rtc_date.day_of_year >> 8;
				temp2 =  rtc_date.day_of_year;
			}
		}
        else if(address == MODBUS_RUN_TIME_LO)
		{
        	 temp1 = (system_timer / 1000) >> 8;
        	 temp2 = (system_timer / 1000);
		}
		else if(address == MODBUS_RUN_TIME_HI)
		{
			temp1 = (system_timer / 1000) >> 24;
			temp2 = (system_timer / 1000) >> 16;
		}
		else if(address == MODBUS_MAX_VARS)
		{
			 temp1 = 0;
			 temp2 = max_vars;
		}
		else if(address == MODBUS_MAX_INS)
		{
			 temp1 = 0;
			 temp2 = max_inputs;
		}
		else if(address == MODBUS_MAX_OUTS)
		{
			 temp1 = 0;
			 temp2 = max_outputs;
		}
        else if(address == MODBUS_TOTAL_NO)
		{
        	 temp1 = 0;
        	 temp2 =  sub_no;
		}
		else if(address >= MODBUS_SUBADDR_FIRST && address <= MODBUS_SUBADDR_LAST)
		{
			temp1 = (current_online[scan_db[address  - MODBUS_SUBADDR_FIRST].id / 8] & (1 << (scan_db[address  - MODBUS_SUBADDR_FIRST].id % 8))) ? 1 : 0;
			temp2 =  scan_db[address  - MODBUS_SUBADDR_FIRST].id;
		}
		else if(address >= MODBUS_TASK_TEST && address < MODBUS_TASK_TEST + 64)
		{
			U16_T reg = (address - MODBUS_TASK_TEST) / 4;
			U16_T index = (address - MODBUS_TASK_TEST) % 4;
			if(index == 0)
			{
				temp1 = task_test.count[reg] >> 8;
				temp2 = task_test.count[reg] & 0xff;
			}
			else if(index == 1)
			{
				temp1 = task_test.old_count[reg] >> 8;
				temp2 = task_test.old_count[reg] & 0xff;
			}
			else if(index == 2)
			{
				temp1 = 0;
				temp2 = task_test.enable[reg];
			}
			else if(index == 3)
			{
				temp1 = task_test.inactive_count[reg] >> 8;
				temp2 = task_test.inactive_count[reg] & 0xff;
			}
		}
         else if(address == WIFI_RSSI)
         {
            temp1 = 0xff;
            temp2 = SSID_Info.rssi;
         }

         else if((address >= MODBUS_WIFI_START)&& (address <= MODBUS_WIFI_END))
		 {
        	 temp = read_wifi_data_by_block(address);
			 temp1 = (temp >> 8) & 0xFF;
			 temp2 = temp & 0xFF;
		 }
         else if(address >= MODBUS_NG2_TEMP1 && address <= MODBUS_NG2_HUM3)
         {
        	 U16_T temp;
        	 Str_points_ptr ptr;
        	 ptr = put_io_buf(IN,16 + (address - MODBUS_NG2_TEMP1));
        	 temp = ptr.pin->value / 100;
        	 temp1 = (temp >> 8) & 0xFF;//temp & 0xFF;
        	 temp2 = temp & 0xFF;//(temp >> 8) & 0xFF;
         }

		 else if((address >= MODBUS_TSTAT10_START)&& (address <= MODBUS_TSTAT10_END))
		 {
			 temp = read_tstat10_data_by_block(address);
			 temp1 = (temp >> 8) & 0xFF;
			 temp2 = temp & 0xFF;
		 }
/******************* read IN OUT by block start ******************************************/
		else if( address >= MODBUS_USER_BLOCK_FIRST && address <= MODBUS_USER_BLOCK_LAST)
		{
			U16_T temp;
			temp = read_user_data_by_block(address);
			temp2 = (temp >> 8) & 0xFF;//temp & 0xFF;
			temp1 = temp & 0xFF;//(temp >> 8) & 0xFF;
		}
/*********************read IN OUT by block endf ******************************************/
		else if(( address >= MODBUS_IO_REG_START && address <= MODBUS_IO_REG_END) ||
				(address >= MODBUS_EXIO_REG_START && address <= MODBUS_EXIO_REG_END))
		{
			U16_T temp;
			temp = read_IO_reg(address);
			temp2 = temp & 0xFF;
			temp1 = (temp >> 8) & 0xFF;
		}
		else if(address == MODBUS_EX_MOUDLE_EN)
		{
			temp1 = 0;
			temp2 = 0x55;
		}
		else if(address == MODBUS_EX_MOUDLE_FLAG12)
		{	// tbd:
			temp1 = 0;
			temp2 = 0x55;
		}
		else
		{
			temp1 = 0 ;
			temp2 = 0;
		}
		 
         if(type == SERIAL || type == BAC_TO_MODBUS)
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
            uart_sendB[0] = TransID >> 8;         //   TransID
            uart_sendB[1] = TransID;
            uart_sendB[2] = 0;         //   ProtoID
            uart_sendB[3] = 0;
            uart_sendB[4] = (3 + num * 2) >> 8;   //   Len
            uart_sendB[5] = (uint8_t)(3 + num * 2) ;
         }
      }
   
	   temp1 = CRChi ;
	   temp2 = CRClo;
	   if(type == BAC_TO_MODBUS)
	   {
		 memcpy(&bacnet_to_modbus,&uart_send[3],num*2);
	   }
	   else if(type == WIFI)
	   {
		  memcpy(resData,uart_sendB,UIP_HEAD + send_cout);
		  *modbus_send_len = UIP_HEAD + send_cout;
	   }
	   else  // MODBUS/485
	   {
		  uart_send[send_cout++] = temp1 ;
		  uart_send[send_cout++] = temp2 ;
		  uart_send_string((const char *)uart_send, send_cout,port);
	   }
   }
   else if(*(bufadd+1 + headlen) == CHECKONLINE)
   { // only support modbus
      if(type == SERIAL)
	  {
         uart_send[send_cout++] = 0xff;
         uart_send[send_cout++] = 0x19;
         uart_send[send_cout++] = Modbus.address;
         uart_send[send_cout++] = Modbus.serialNum[0];
         uart_send[send_cout++] = Modbus.serialNum[1];
         uart_send[send_cout++] = Modbus.serialNum[2];
         uart_send[send_cout++] = Modbus.serialNum[3];
         for(i=0;i<send_cout;i++)
            crc16_byte(uart_send[i]);      	  
	  
		   temp1 = CRChi ;
		   temp2 = CRClo;
		   
		   uart_send[send_cout++] = temp1 ;
		   uart_send[send_cout++] = temp2 ;
		   uart_send_string((const char *)uart_send, send_cout,port);
	  }
   }
   

   if(type == WIFI)
   {
      free(uart_sendB);
   }
   else
   {
      free(uart_send);
   }

}


void responseModbusCmd(uint8_t type, uint8_t *pData, uint16_t len,uint8_t *resData ,uint16_t *modbus_send_len,uint8_t port)
{

   if(type == WIFI)
   {
      reg_num = pData[4]*256 + pData[5];
      responseModbusData(pData,WIFI, len,resData,modbus_send_len,port);
      internalDeal(pData,WIFI);
   }
   else if(type == BAC_TO_MODBUS)
   {
   	   responseModbusData(pData,BAC_TO_MODBUS,len,resData,modbus_send_len,port);
       internalDeal(pData,BAC_TO_MODBUS);
   }
   else  // MODBUS/RS485
   {
      reg_num = pData[4]*256 + pData[5];
      responseModbusData(pData,SERIAL,len,resData,modbus_send_len,port);
      internalDeal(pData,SERIAL);
   }
}


void write_user_data_by_block(U16_T StartAdd,U8_T HeadLen,U8_T *pData)
{
	U8_T far i,j;
	uint8_t *ptr;
	if(StartAdd  >= MODBUS_SETTING_BLOCK_FIRST && StartAdd  <= MODBUS_SETTING_BLOCK_LAST)
	{
		if((StartAdd - MODBUS_SETTING_BLOCK_FIRST) % 100 == 0)
		{
			i = (StartAdd - MODBUS_SETTING_BLOCK_FIRST) / 100;
			memcpy(&Setting_Info.all[i * 200],&pData[HeadLen + 7],200);

			if(i == 1)
			{
				dealwith_write_setting(&Setting_Info);
			}
		}

	}
	else if(StartAdd  >= MODBUS_OUTPUT_BLOCK_FIRST && StartAdd  <= MODBUS_OUTPUT_BLOCK_LAST)
	{
		if((StartAdd - MODBUS_OUTPUT_BLOCK_FIRST) % ((sizeof(Str_out_point) + 1) / 2) == 0)
		{
			S32_T old_value;
			i = (StartAdd - MODBUS_OUTPUT_BLOCK_FIRST) / ((sizeof(Str_out_point) + 1) / 2);
#if NEW_IO
			if(i < max_outputs)
			{
				ptr = new_outputs + i;
				memcpy(ptr,&pData[HeadLen + 7],sizeof(Str_out_point));
			}
#else
			memcpy(&outputs[i],&pData[HeadLen + 7],sizeof(Str_out_point));
#endif
			check_output_priority_array(i,0);

		}
	}
	else if(StartAdd  >= MODBUS_INPUT_BLOCK_FIRST && StartAdd  <= MODBUS_INPUT_BLOCK_LAST)
	{
		uint8_t *ptr;
		if((StartAdd - MODBUS_INPUT_BLOCK_FIRST) % ((sizeof(Str_in_point) + 1	) / 2) == 0)
		{
			i = (StartAdd - MODBUS_INPUT_BLOCK_FIRST) / ((sizeof(Str_in_point) + 1) / 2);
#if NEW_IO
			if(i < max_inputs)
			{
				ptr = new_inputs + i;
				memcpy(ptr,&pData[HeadLen + 7],sizeof(Str_in_point));
			}
#else
			memcpy(&inputs[i],&pData[HeadLen + 7],sizeof(Str_in_point));
#endif

		}
	}
	else if(StartAdd  >= MODBUS_VAR_BLOCK_FIRST && StartAdd  <= MODBUS_VAR_BLOCK_LAST)
	{
		if((StartAdd - MODBUS_VAR_BLOCK_FIRST) % ((sizeof(Str_variable_point) + 1	) / 2) == 0)
		{
			i = (StartAdd - MODBUS_VAR_BLOCK_FIRST) / ((sizeof(Str_variable_point) + 1) / 2);
#if NEW_IO
			if(i < max_vars)
			{
				ptr = new_vars + i;
				memcpy(ptr,&pData[HeadLen + 7],sizeof(Str_variable_point));
			}
#else
			memcpy(&vars[i],&pData[HeadLen + 7],sizeof(Str_variable_point));
#endif
		}
	}
	else if(StartAdd  >= MODBUS_PRG_BLOCK_FIRST && StartAdd  <= MODBUS_PRG_BLOCK_LAST)
	{
		if((StartAdd - MODBUS_PRG_BLOCK_FIRST) % ((sizeof(Str_program_point) + 1	) / 2) == 0)
		{
			i = (StartAdd - MODBUS_PRG_BLOCK_FIRST) / ((sizeof(Str_program_point) + 1) / 2);
			memcpy(&programs[i],&pData[HeadLen + 7],sizeof(Str_program_point));
		}
	}
	else if(StartAdd  >= MODBUS_WR_BLOCK_FIRST && StartAdd  <= MODBUS_WR_BLOCK_LAST)
	{
		if((StartAdd - MODBUS_WR_BLOCK_FIRST) % ((sizeof(Str_weekly_routine_point) + 1	) / 2) == 0)
		{
			i = (StartAdd - MODBUS_WR_BLOCK_FIRST) / ((sizeof(Str_weekly_routine_point) + 1) / 2);
			memcpy(&weekly_routines[i],&pData[HeadLen + 7],sizeof(Str_weekly_routine_point));
		}
	}
	else if(StartAdd  >= MODBUS_AR_BLOCK_FIRST && StartAdd  <= MODBUS_AR_BLOCK_LAST)
	{
		if((StartAdd - MODBUS_AR_BLOCK_FIRST) % ((sizeof(Str_annual_routine_point) + 1	) / 2) == 0)
		{
			i = (StartAdd - MODBUS_AR_BLOCK_FIRST) / ((sizeof(Str_annual_routine_point) + 1) / 2);
			memcpy(&annual_routines[i],&pData[HeadLen + 7],sizeof(Str_annual_routine_point));
		}
	}
	else if(StartAdd  >= MODBUS_WR_TIME_FIRST && StartAdd  <= MODBUS_WR_TIME_LAST)
	{
		if((StartAdd - MODBUS_WR_TIME_FIRST) % (sizeof(Wr_one_day) * MAX_SCHEDULES_PER_WEEK / 2) == 0)
		{
			i = (StartAdd - MODBUS_WR_TIME_FIRST) / (sizeof(Wr_one_day) * MAX_SCHEDULES_PER_WEEK / 2);
			memcpy(&wr_times[i],&pData[HeadLen + 7],(sizeof(Wr_one_day) * MAX_SCHEDULES_PER_WEEK));
		}
	}
	else if(StartAdd  >= MODBUS_AR_TIME_FIRST && StartAdd  <= MODBUS_AR_TIME_LAST)
	{
		if((StartAdd - MODBUS_AR_TIME_FIRST) % (AR_DATES_SIZE / 2) == 0)
		{
			i = ((StartAdd - MODBUS_AR_TIME_FIRST) / (AR_DATES_SIZE / 2));
			memcpy(&ar_dates[i],&pData[HeadLen + 7],AR_DATES_SIZE);
		}
	}
	else if(StartAdd  >= MODBUS_CONTROLLER_BLOCK_FIRST && StartAdd  <= MODBUS_CONTROLLER_BLOCK_LAST)
	{
		if((StartAdd - MODBUS_CONTROLLER_BLOCK_FIRST) % (sizeof(Str_controller_point) / 2) == 0)
		{
			i = ((StartAdd - MODBUS_CONTROLLER_BLOCK_FIRST) / (sizeof(Str_controller_point) / 2));
			memcpy(&controllers[i],&pData[HeadLen + 7],sizeof(Str_controller_point));
		}
	}
	else if (StartAdd >= MODBUS_WR_FLAG_FIRST && StartAdd <= MDOBUS_WR_FLAG_LAST)
	{
		if ((StartAdd - MODBUS_WR_FLAG_FIRST) % ((MAX_SCHEDULES_PER_WEEK * 8 + 1) / 2) == 0)
		{
			i = (StartAdd - MODBUS_WR_FLAG_FIRST) / ((MAX_SCHEDULES_PER_WEEK * 8 + 1) / 2);
			memcpy(&wr_time_on_off[i], &pData[HeadLen + 7], MAX_SCHEDULES_PER_WEEK * 8);
		}

	}
#if NEW_IO
	else if(StartAdd >= MODBUS_EXIO_REG_START && StartAdd < 49999)
	{
		U16_T ex_out_start = MODBUS_EXIO_REG_START;
		U16_T ex_out_end = ex_out_start;
		U16_T ex_in_start = ex_out_start;
		U16_T ex_in_end = ex_out_start;
		U16_T ex_var_start = ex_out_start;
		U16_T ex_var_end = ex_out_start;
		// addtional outputs
		if(max_outputs > ORIGINAL_OUTS)
		{
			ex_out_end = ex_out_start + ( (sizeof(Str_out_point) + 1) / 2) * (max_outputs - ORIGINAL_OUTS);
			ex_in_start = ex_out_end;
			ex_in_end = ex_in_start;
			ex_var_start = ex_in_start;
			ex_var_end = ex_in_start;
		}
		// addtional inputs
		if(max_inputs > ORIGINAL_INS)
		{
			ex_in_end = ex_in_start + ( (sizeof(Str_in_point) + 1) / 2) * (max_inputs - ORIGINAL_INS);
			ex_var_start = ex_in_end;
			ex_var_end = ex_var_start;
		}
		// addtional vars
		if(max_vars > ORIGINAL_VARS)
		{
			ex_var_end = ex_var_start + ( (sizeof(Str_variable_point) + 1) / 2) * (max_vars - ORIGINAL_VARS);
		}

		if( StartAdd >= ex_out_start && StartAdd < ex_out_end )
		{
			i = (StartAdd - ex_out_start) / ( (sizeof(Str_out_point) + 1) / 2);
			if(new_outputs != NULL)
				if(ORIGINAL_OUTS + i < max_outputs)
				{
					ptr = new_outputs + ORIGINAL_INS + i;
					memcpy(ptr,&pData[HeadLen + 7],sizeof(Str_out_point));
				}
		}
		else if( StartAdd >= ex_in_start && StartAdd < ex_in_end )
		{
			i = (StartAdd - ex_in_start) / ((sizeof(Str_in_point) + 1) / 2);
			if(new_inputs != NULL)
				if(ORIGINAL_INS + i < max_inputs)
				{
					ptr = new_inputs + ORIGINAL_INS + i;
					memcpy(ptr,&pData[HeadLen + 7],sizeof(Str_in_point));
				}
		}
		else if( StartAdd >= ex_var_start && StartAdd < ex_var_end )
		{
			i = (StartAdd - ex_var_start) / ((sizeof(Str_variable_point) + 1) / 2);
			if(new_vars != NULL)
				if(ORIGINAL_VARS + i < max_vars)
				{
					ptr = new_vars + ORIGINAL_VARS + i;
					memcpy(ptr,&pData[HeadLen + 7],sizeof(Str_variable_point));
				}
		}
	}
#endif
}


uint16_t read_user_data_by_block(uint16_t addr)
{
	U8_T  index = 0,item = 0;
	U16_T *block = NULL;
	if( addr >= MODBUS_SETTING_BLOCK_FIRST && addr <= MODBUS_SETTING_BLOCK_LAST )
	{
		index = (addr - MODBUS_SETTING_BLOCK_FIRST) / 100;
		block = (U16_T *)&Setting_Info.all[index * 200];
		item = (addr - MODBUS_SETTING_BLOCK_FIRST) % 100;
	}
	else if( addr >= MODBUS_OUTPUT_BLOCK_FIRST && addr <= MODBUS_OUTPUT_BLOCK_LAST )
	{
		index = (addr - MODBUS_OUTPUT_BLOCK_FIRST) / ( (sizeof(Str_out_point) + 1) / 2);
#if NEW_IO
		if(index < max_outputs)
			block = (U16_T *)(new_outputs + index);
#else
		block = (U16_T *)&outputs[index];
#endif
		item = (addr - MODBUS_OUTPUT_BLOCK_FIRST) % ((sizeof(Str_out_point) + 1) / 2);
	}
	else if( addr >= MODBUS_INPUT_BLOCK_FIRST && addr <= MODBUS_INPUT_BLOCK_LAST )
	{
		index = (addr - MODBUS_INPUT_BLOCK_FIRST) / ((sizeof(Str_in_point) + 1) / 2);
#if NEW_IO
		if(index < max_inputs)
			block = (U16_T *)(new_inputs + index);
#else
		block = (U16_T *)&inputs[index];
#endif
		item = (addr - MODBUS_INPUT_BLOCK_FIRST) % ((sizeof(Str_in_point) + 1) / 2);
	}
	else if( addr >= MODBUS_VAR_BLOCK_FIRST && addr <= MODBUS_VAR_BLOCK_LAST )
	{
		index = (addr - MODBUS_VAR_BLOCK_FIRST) / ((sizeof(Str_variable_point) + 1) / 2);
#if NEW_IO
		if(index < max_vars)
			block = (U16_T *)(new_vars + index);
#else
		block = (U16_T *)&vars[index];
#endif
		item = (addr - MODBUS_VAR_BLOCK_FIRST) % ((sizeof(Str_variable_point) + 1) / 2);
	}
	else if( addr >= MODBUS_PRG_BLOCK_FIRST && addr <= MODBUS_PRG_BLOCK_LAST )
	{
		index = (addr - MODBUS_PRG_BLOCK_FIRST) / ((sizeof(Str_program_point) + 1) / 2);
		block = (U16_T *)&programs[index];
		item = (addr - MODBUS_PRG_BLOCK_FIRST) % ((sizeof(Str_program_point) + 1) / 2);
	}
	else if( addr >= MODBUS_CODE_BLOCK_FIRST && addr <= MODBUS_CODE_BLOCK_LAST )
	{
		index = (addr - MODBUS_CODE_BLOCK_FIRST) / 100;
		block = (U16_T *)&prg_code[index / (CODE_ELEMENT * MAX_CODE / 200)][CODE_ELEMENT * MAX_CODE % 200];
		item = (addr - MODBUS_CODE_BLOCK_FIRST) % 100;
	}
	else if( addr >= MODBUS_WR_BLOCK_FIRST && addr <= MODBUS_WR_BLOCK_LAST )
	{
		index = (addr - MODBUS_WR_BLOCK_FIRST) / ((sizeof(Str_weekly_routine_point) + 1) / 2);
		block = (U16_T *)&weekly_routines[index];
		item = (addr - MODBUS_WR_BLOCK_FIRST) % ((sizeof(Str_weekly_routine_point) + 1) / 2);
	}
	else if( addr >= MODBUS_WR_TIME_FIRST && addr <= MODBUS_WR_TIME_LAST )
	{
		index = (addr - MODBUS_WR_TIME_FIRST) / ((sizeof(Wr_one_day) * MAX_SCHEDULES_PER_WEEK + 1) / 2);
		block = (U16_T *)&wr_times[index];
		item = (addr - MODBUS_WR_TIME_FIRST) % ((sizeof(Wr_one_day) * MAX_SCHEDULES_PER_WEEK + 1) / 2);
	}
	else if( addr >= MODBUS_AR_BLOCK_FIRST && addr <= MODBUS_AR_BLOCK_LAST )
	{
		index = (addr - MODBUS_AR_BLOCK_FIRST) / ((sizeof(Str_annual_routine_point) + 1) / 2);
		block = (U16_T *)&annual_routines[index];
		item = (addr - MODBUS_AR_BLOCK_FIRST) % ((sizeof(Str_annual_routine_point) + 1) / 2);

	}
	else if( addr >= MODBUS_AR_TIME_FIRST && addr <= MODBUS_AR_TIME_LAST )
	{
		index = (addr - MODBUS_AR_TIME_FIRST) / (AR_DATES_SIZE / 2);
		block = (U16_T *)&ar_dates[index];
		item = (addr - MODBUS_AR_TIME_FIRST) % (AR_DATES_SIZE / 2);
	}
	else if( addr >= MODBUS_CONTROLLER_BLOCK_FIRST && addr <= MODBUS_CONTROLLER_BLOCK_LAST )
	{
		index = (addr - MODBUS_CONTROLLER_BLOCK_FIRST) / (sizeof(Str_controller_point) / 2);
		block = (U16_T *)&controllers[index];
		item = (addr - MODBUS_CONTROLLER_BLOCK_FIRST) % (sizeof(Str_controller_point) / 2);
	}
	else if (addr >= MODBUS_WR_FLAG_FIRST && addr <= MDOBUS_WR_FLAG_LAST)
	{
		index = (addr - MODBUS_WR_FLAG_FIRST) / ((MAX_SCHEDULES_PER_WEEK * 8 + 1) / 2);
		block = (U16_T *)&wr_time_on_off[index];
		item = (addr - MODBUS_WR_FLAG_FIRST) % ((MAX_SCHEDULES_PER_WEEK * 8 + 1) / 2);
	}
#if NEW_IO
	else if(addr >= MODBUS_EXIO_REG_START && addr < 49999)
	{
		U16_T ex_out_start = MODBUS_EXIO_REG_START;
		U16_T ex_out_end = ex_out_start;
		U16_T ex_in_start = ex_out_start;
		U16_T ex_in_end = ex_out_start;
		U16_T ex_var_start = ex_out_start;
		U16_T ex_var_end = ex_out_start;
		// addtional outputs
		if(max_outputs > ORIGINAL_OUTS)
		{
			ex_out_end = ex_out_start + ( (sizeof(Str_out_point) + 1) / 2) * (max_outputs - ORIGINAL_OUTS);
			ex_in_start = ex_out_end;
			ex_in_end = ex_in_start;
			ex_var_start = ex_in_start;
			ex_var_end = ex_in_start;
		}
		// addtional inputs
		if(max_inputs > ORIGINAL_INS)
		{
			ex_in_end = ex_in_start + ( (sizeof(Str_in_point) + 1) / 2) * (max_inputs - ORIGINAL_INS);
			ex_var_start = ex_in_end;
			ex_var_end = ex_var_start;
		}
		// addtional vars
		if(max_vars > ORIGINAL_VARS)
		{
			ex_var_end = ex_var_start + ( (sizeof(Str_variable_point) + 1) / 2) * (max_vars - ORIGINAL_VARS);
		}


		if( addr >= ex_out_start && addr < ex_out_end )
		{
			index = (addr - ex_out_start) / ( (sizeof(Str_out_point) + 1) / 2);
			//block = (U16_T *)&outputs[ORIGINAL_OUTS + index];
			block = (U16_T *)(new_outputs + ORIGINAL_OUTS + index);
			item = (addr - ex_out_start) % ((sizeof(Str_out_point) + 1) / 2);
		}
		else if( addr >= ex_in_start && addr < ex_in_end )
		{
			index = (addr - ex_in_start) / ((sizeof(Str_in_point) + 1) / 2);
			//block = (U16_T *)&inputs[ORIGINAL_INS + index];
			block = (U16_T *)(new_inputs + ORIGINAL_INS + index);
			item = (addr - ex_in_start) % ((sizeof(Str_in_point) + 1) / 2);
		}
		else if( addr >= ex_var_start && addr < ex_var_end )
		{
			index = (addr - ex_var_start) / ((sizeof(Str_variable_point) + 1) / 2);
			//block = (U16_T *)&vars[ORIGINAL_VARS + index];
			block = (U16_T *)(new_vars + ORIGINAL_VARS + index);
			item = (addr - ex_var_start) % ((sizeof(Str_variable_point) + 1) / 2);
		}

	}
#endif
	if(block != NULL)
		return block[item];
	else
		return 0;
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
      save_block(FLASH_BLOCK1_SSID);//save_wifi_info();
      //re_init_wifi = true;
      esp_restart();
   }
   else if(StartAdd == MODBUS_WIFI_RESTORE)
   {
      //if(pData[HeadLen + 5] == 1)
      //   Restore_WIFI();
   }
   else if(StartAdd == MODBUS_WIFI_MODE)
   {
      SSID_Info.IP_Auto_Manual = pData[HeadLen + 5];
//      write_eeprom(WIFI_IP_AM,pData[HeadLen + 5]);
      save_wifi_info();
      		//re_init_wifi = true;
      //esp_restart();

   }
   else if(StartAdd == MODBUS_WIFI_BACNET_PORT)
   {
      SSID_Info.bacnet_port = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
   }
   else if(StartAdd == MODBUS_WIFI_MODBUS_PORT)
   {
      SSID_Info.modbus_port = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
//      write_eeprom(WIFI_MODBUS_PORT,SSID_Info.modbus_port);
//      write_eeprom(WIFI_MODBUS_PORT + 1,SSID_Info.modbus_port >> 8);
   }
   else if(StartAdd == MODBUS_WIIF_START_SMART)
   {
      // write 1, start smart cofigure
      //if(pData[HeadLen + 5] == 1)
      //   Start_Smart_Config();
      //else
      //   Stop_Smart_Config();
   }
   else if(StartAdd == MODBUS_WIFI_WRITE_MAC)
   {
//      if(pData[HeadLen + 5] == 0)
//         write_eeprom(EEP_WRITE_WIFI_MAC,0);
   }
   else if(StartAdd >= MODBUS_WIFI_SSID_START && StartAdd <= MODBUS_WIFI_SSID_END)
   {
      if((StartAdd - MODBUS_WIFI_SSID_START) % 32 == 0)
      {
         memset(&SSID_Info.name,'\0',64);
         memcpy(&SSID_Info.name,&pData[HeadLen + 7],64);
      }
      //save_block(FLASH_BLOCK1_SSID);//save_wifi_info();
   }
   else if(StartAdd >= MODBUS_WIFI_PASS_START && StartAdd <= MODBUS_WIFI_PASS_END)
   {
      if((StartAdd - MODBUS_WIFI_PASS_START) % 16 == 0)
      {
         memset(&SSID_Info.password,'\0',32);
         memcpy(&SSID_Info.password,&pData[HeadLen + 7],32);
      }
      //save_block(FLASH_BLOCK1_SSID);//save_wifi_info();
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

         save_wifi_info();

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


uint16_t read_tstat10_data_by_block(uint16_t addr)
{
   uint8_t item;
   uint16_t *block;
   uint8_t *block1;
   uint8_t temp;
   if(addr == MODBUS_ICON_CONFIG)
   {
      return Modbus.icon_config;
   }
   else if(addr == MODBUS_DISALBE_TSTAT10_DIS)
   {
      return 0;//SSID_Info.IP_Auto_Manual;
   }
   else if(addr == MODBUS_TEMPERATURE)
   {
      return 0;//SSID_Info.IP_Wifi_Status;
   }
   else if(addr == MODBUS_TVOC)
   {
      return 0;//SSID_Info.modbus_port;
   }
   else if(addr == MODBUS_HUMIDY)
   {
      return 0;//SSID_Info.bacnet_port;
   }
   else if(addr == MODBUS_OCCUPID)
   {
      return 0;//SSID_Info.rev;
   }
   else if(addr == MODBUS_CO2)
   {
      return 0;
   }
   else if(addr == MODBUS_LIGHT)
	{
	 return 0;
	}
   else if(addr == MODBUS_VOICE)
	{
	  return 0;
	}
   else
      return 0;

}


static void write_tstat10_data_by_block(uint16_t StartAdd,uint8_t HeadLen,uint8_t *pData,uint8_t type)
{
   //uint8_t i,j;

   if(StartAdd == MODBUS_ICON_CONFIG)
   {
      Modbus.icon_config = pData[HeadLen + 5];
      save_uint8_to_flash( FLASH_ICON_CONFIG, Modbus.icon_config);
   }
   else if(StartAdd >= MODBUS_LCD_CONFIG_FIRST && StartAdd <= MODBUS_LCD_CONFIG_END)
   {
//		vTaskSuspend(Handle_Menu);
		display_lcd.lcddisplay[StartAdd - MODBUS_LCD_CONFIG_FIRST] = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
		memcpy(Setting_Info.reg.display_lcd.lcddisplay,display_lcd.lcddisplay,sizeof(lcdconfig));
		// clear first screen
		/*disp_str(FORM15X30, 6,  32, " ",SCH_COLOR,TSTAT8_BACK_COLOR);
		disp_ch(0,FIRST_CH_POS,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
		disp_ch(0,SECOND_CH_POS,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
		disp_ch(0,THIRD_CH_POS,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
		disp_ch(0,UNIT_POS,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);*/
		// save it to flash memory
		//write_page_en[25] = 1;
		//Flash_Write_Mass();
		Save_Lcd_config();
//		vTaskResume(Handle_Menu);

   }
}


extern  EventGroupHandle_t s_wifi_event_group;
void internalDeal(uint8_t  *bufadd,uint8_t type)
{
   uint16_t address;
   uint16_t  temp_i;
   uint8_t i;
   //uint8_t  HeadLen;
   if(type == SERIAL || type == BAC_TO_MODBUS)  // modbus packet
   {
      //HeadLen = 0;
   }
   else    // TCP packet or wifi
   {
      //HeadLen = UIP_HEAD;
      bufadd = bufadd + 6;
   }

   address = ((uint16_t)*(bufadd+2) <<8) + *(bufadd+3); //get modbus register number

   if(*(bufadd+1) == MULTIPLE_WRITE_VARIABLES)
   {
      temp_i = (uint16_t)(*(bufadd+2) << 8) + *(bufadd+3);
      if(Modbus.mini_type == PROJECT_AIRLAB)
      {
    	write_airlab_by_block(temp_i,0,bufadd,0);
      }
      if(temp_i >= MODBUS_WIFI_START && temp_i <= MODBUS_WIFI_END)
      {
         write_wifi_data_by_block(temp_i,0,bufadd,0);
      }

      /******************* write IN OUT by block start ******************************************/
	 else if(temp_i  >= MODBUS_USER_BLOCK_FIRST && temp_i  <= MODBUS_USER_BLOCK_LAST)
	 {
		 // dealwith_block
		 write_user_data_by_block(temp_i,0,bufadd);
		 save_point_info(0);
	 }
      /*********************read IN OUT by block endf ***************************************/
      else if(temp_i == SERIALNUMBER_LOWORD )
      {
         Modbus.serialNum[0] = *(bufadd + 8);
         Modbus.serialNum[1] = *(bufadd + 10);
         Modbus.serialNum[2] = *(bufadd + 12);
         Modbus.serialNum[3] = *(bufadd + 13);
         save_uint8_to_flash(FLASH_SERIAL_NUM1,Modbus.serialNum[0]);
         save_uint8_to_flash(FLASH_SERIAL_NUM2,Modbus.serialNum[1]);
         save_uint8_to_flash(FLASH_SERIAL_NUM3,Modbus.serialNum[2]);
         save_uint8_to_flash(FLASH_SERIAL_NUM4,Modbus.serialNum[3]);
       }
      else if((temp_i >= MODBUS_IO_REG_START && temp_i <= MODBUS_IO_REG_END) ||
    		  (temp_i >= MODBUS_EXIO_REG_START && temp_i <= MODBUS_EXIO_REG_END))
      {
    	  MulWrite_IO_reg(address,bufadd);
      }
   }
   if(*(bufadd+1) == WRITE_VARIABLES)
   {
	   if(Modbus.mini_type == PROJECT_AIRLAB)
		{
		   write_airlab_by_block(address,0,bufadd,0);
		}

	   if (address >= SERIALNUMBER_LOWORD && address <= SERIALNUMBER_LOWORD + 3 )
		{
			if((address == SERIALNUMBER_LOWORD) && (SNWriteflag & 0x01) == 0)
			{
				 Modbus.serialNum[0] = *(bufadd + 5);
				 Modbus.serialNum[1] = *(bufadd + 4);
				save_uint8_to_flash(FLASH_SERIAL_NUM1,Modbus.serialNum[0]);
				save_uint8_to_flash(FLASH_SERIAL_NUM2,Modbus.serialNum[1]);

				SNWriteflag |= 0x01;
				if(SNWriteflag & 0x02)
					update_flash = 0;
			}
			else if((address == SERIALNUMBER_HIWORD) && (SNWriteflag & 0x02) == 0)
			{
				Modbus.serialNum[2] = *(bufadd + 5);
				Modbus.serialNum[3] = *(bufadd + 4);
		        save_uint8_to_flash(FLASH_SERIAL_NUM3,Modbus.serialNum[2]);
		        save_uint8_to_flash(FLASH_SERIAL_NUM4,Modbus.serialNum[3]);
				SNWriteflag |= 0x02;
				if(SNWriteflag & 0x01)
					update_flash = 0;
			}
			save_uint8_to_flash(FLASH_SN_WRITE,SNWriteflag);
		}
	   if(address >= MODBUS_WIFI_START && address <= MODBUS_WIFI_END)
		{
			write_wifi_data_by_block(address,0,bufadd,0);
		}
	   if(address >= MODBUS_TSTAT10_START && address <= MODBUS_TSTAT10_END)
	   {

		   write_tstat10_data_by_block(address,0,bufadd,0);
	   }
	  /*if(address == MODBUS_WIFI_START)
	  {
		  if(*(bufadd + 5)== 2)
		  {
			  SSID_Info.MANUEL_EN = 2;
			  save_block(FLASH_BLOCK1_SSID);//save_wifi_info();
			  SSID_Info.IP_Wifi_Status = WIFI_DISCONNECTED;
			  esp_wifi_stop();
		  }
		  if(*(bufadd + 5)== 1)
		  {
			  SSID_Info.MANUEL_EN = 1;
			  save_block(FLASH_BLOCK1_SSID);//save_wifi_info();
			  esp_restart();
			 //wifi_init_sta();

		  }
	  }*/
      if(address == MODBUS_ADDRESS)
      {
         if((*(bufadd + 5)!=0)&&(*(bufadd + 5)!=0xff))
         {
        	 Modbus.address = *(bufadd + 5);
        	 Station_NUM = Modbus.address;
			Setting_Info.reg.MSTP_ID = Station_NUM;
			dlmstp_init(NULL);
            save_uint8_to_flash( FLASH_MODBUS_ID, Modbus.address);
         }
      }

      else if(address == MODBUS_INSTANCE_LO)
      {
		U32_T		temp;
		temp = Instance;
		if((U16_T)(Instance >> 16) != (*(bufadd + 5)) + (*(bufadd + 4)) * 256)
		{
			temp = temp & 0xffff0000L;
			temp |= (*(bufadd + 5)) + (*(bufadd + 4)) * 256;
			Instance = temp;
			save_uint8_to_flash(FLASH_INSTANCE1,*(bufadd + 5));
			save_uint8_to_flash(FLASH_INSTANCE2,*(bufadd + 4));
			Device_Set_Object_Instance_Number(Instance);

		}
      }
	  else if(address == MODBUS_INSTANCE_HI)
		{
			if((U16_T)(Instance >> 16) != (*(bufadd + 5)) + (*(bufadd + 4)) * 256)
			{
				Instance = ((U32_T)(*(bufadd + 5)) << 16) + ((U32_T)(*(bufadd + 4)) << 24) + (U16_T)Instance;
				save_uint8_to_flash(FLASH_INSTANCE3,*(bufadd + 5));
				save_uint8_to_flash(FLASH_INSTANCE4,*(bufadd + 4));
				Device_Set_Object_Instance_Number(Instance);
			}
		}
      else if(address == MODBUS_UART0_BAUDRATE)
      {
         if(*(bufadd + 5)<10)
         {
            Modbus.baudrate[0] = *(bufadd + 5);
            save_uint8_to_flash( FLASH_BAUD_RATE, Modbus.baudrate[0]);
            //uart_init(0);
            flag_change_uart0 = 1;
            count_change_uart0 = 0;
         }
      }
      else if(address == MODBUS_UART2_BAUDRATE)
		{
		   if(*(bufadd + 5)<10)
		   {
			  Modbus.baudrate[2] = *(bufadd + 5);
			  save_uint8_to_flash( FLASH_BAUD_RATE2, Modbus.baudrate[2]);
			  //uart_init(2);
			  flag_change_uart2 = 1;
			  count_change_uart2 = 0;
		   }
		}
      else if(address == MODBUS_ENABLE_DEBUG)
      {
		  Modbus.enable_debug = *(bufadd + 5);
		  //save_uint8_to_flash( FLASH_ENABLE_DEBUG, Modbus.enable_debug);

      }
      else if(address == MODBUS_TIME_ZONE)
	   {
		  timezone = *(bufadd + 5) + (*(bufadd + 4)) * 256;
		  save_uint8_to_flash( FLASH_TIME_ZONE,timezone);

	   }
      else if(address == MODBUS_DSL)
	   {
    	  Daylight_Saving_Time = *(bufadd + 5);
		  save_uint8_to_flash( FLASH_DSL, Daylight_Saving_Time);

	   }
      else if(address == MODBUS_SNTP_EN)
	   {
			Modbus.en_sntp =  *(bufadd + 5);
			sntp_select_time_server(Modbus.en_sntp);
			save_uint8_to_flash(FLASH_EN_SNTP, Modbus.en_sntp);
			if(Modbus.en_sntp >= 2)
			{
				flag_Update_Sntp = 0;
				Update_Sntp_Retry = 0;
			}
	   }
      else if(address == MODBUS_TCP_PORT)
	   {
    	  Modbus.tcp_port = *(bufadd + 5) + (*(bufadd + 4)) * 256;
		  save_uint16_to_flash( FLASH_TCP_PORT,Modbus.tcp_port);
	   }
      else if(address == MODBUS_READ_POINT_TIMER)
	   {
		  READ_POINT_TIMER = *(bufadd + 5) + (*(bufadd + 4)) * 256;
		  READ_POINT_TIMER_FROM_EEP = READ_POINT_TIMER;
		  save_uint16_to_flash( FLASH_READ_POINT_TIMER,READ_POINT_TIMER);
	   }
      else if(address == IP_MODE)
      {
    	  Modbus.tcp_type = *(bufadd + 5);
    	  save_uint8_to_flash( FLASH_TCP_TYPE, Modbus.tcp_type);
      }
	  else if(address == MODBUS_COM0_TYPE)
		{
		   if(*(bufadd + 5) < MAX_COM_TYPE)
		   {
			  Modbus.com_config[0] = *(bufadd + 5);
			  save_uint8_to_flash( FLASH_UART_CONFIG, Modbus.com_config[0]);
			  //uart_init(0);
			  flag_change_uart0 = 1;
			  count_change_uart0 = 0;
			  Count_com_config();
			  if(Modbus.com_config[0] == BACNET_SLAVE || Modbus.com_config[0] == BACNET_MASTER)
				{
					//Send_Whois_Flag = 1;
					Recievebuf_Initialize(0);
				}
		   }
		}
		else if(address == MODBUS_COM2_TYPE)
		{
		   if(*(bufadd + 5) < MAX_COM_TYPE)
		   {
			  Modbus.com_config[2] = *(bufadd + 5);
			  save_uint8_to_flash( FLASH_UART2_CONFIG, Modbus.com_config[2]);
			  flag_change_uart2 = 1;
			  count_change_uart2 = 0;
			  //uart_init(2);
			  Count_com_config();
			  if(Modbus.com_config[2] == BACNET_SLAVE || Modbus.com_config[2] == BACNET_MASTER)
			  {
					Recievebuf_Initialize(2);
			  }
		   }
		}
		else if(address == MODBUS_TEST_CMD)
		{
			// show identify
			// control led on/off
			if(*(bufadd + 5) == 77)
			{
				gIdentify = 1;
				count_gIdentify = 0;
			}
		}
		else if(address == MODBUS_MINI_TYPE)
		{
			if(*(bufadd + 5) <= MAX_MINI_TYPE && *(bufadd + 5) >= MINI_BIG_ARM)
			{
				Modbus.mini_type = *(bufadd + 5);
				save_uint8_to_flash( FLASH_MINI_TYPE, Modbus.mini_type);
			}
		}
		else if(address == MODBUS_PANEL_NUMBER)
		{
			if(*(bufadd + 5) != 0 && *(bufadd + 5) != 255)
			{
				panel_number = *(bufadd + 5);

				change_panel_number_in_code(Setting_Info.reg.panel_number,panel_number);
				Setting_Info.reg.panel_number	= panel_number;
				Modbus.address = panel_number;
				Station_NUM = panel_number;
				save_uint8_to_flash( FLASH_MODBUS_ID, Modbus.address);
			}
		}
		else if(address == MODBUS_STATION_NUM)
		{
			if(*(bufadd + 5) != 0 && *(bufadd + 5) != 255)
			{
				Station_NUM = *(bufadd + 5);
				Modbus.address = Station_NUM;
				panel_number = Station_NUM;
				save_uint8_to_flash( FLASH_MODBUS_ID,panel_number);
				Setting_Info.reg.MSTP_ID	= Station_NUM;
				dlmstp_init(NULL);
			}
		}
		else if(address == MODBUS_NETWORK_NUMBER)
		{
			Modbus.network_number = (*(bufadd + 5)) + (*(bufadd + 4)) * 256;
			save_uint16_to_flash(FLASH_NETWORK_NUMBER,Modbus.network_number);
		}
		else if(address == MODBUS_MSTP_NETWORK)
		{
			Modbus.mstp_network = (*(bufadd + 5)) + (*(bufadd + 4)) * 256;
			save_uint16_to_flash(FLASH_MSTP_NETWORK,Modbus.mstp_network);
		}
		else if(address == MODBUS_MAX_VARS)
		{
			max_vars = *(bufadd + 5);
			save_uint8_to_flash(FLASH_MAX_VARS,max_vars);
		}
		else if(address == MODBUS_MAX_INS)
		{
			max_inputs = *(bufadd + 5);
			save_uint8_to_flash(FLASH_MAX_INS,max_inputs);
		}
		else if(address == MODBUS_MAX_OUTS)
		{
			max_outputs = *(bufadd + 5);
			save_uint8_to_flash(FLASH_MAX_OUTS,max_outputs);
		}
      else if (address == UPDATE_STATUS)
      {
    	 update_flash = *(bufadd + 5);
		 if (update_flash == 0x7f)
		 {
			 start_fw_update();
		 }
		 else if((update_flash == 0x8E) || (update_flash == 0x8F))
		 {
			if(update_flash == 0x8e)
			{
				SNWriteflag = 0x00;
				//E2prom_Write_Byte(EEP_SERIALNUMBER_WRITE_FLAG, SNWriteflag);
			}
		 }
      }
      else if(address >= MODBUS_IN1_CAL && address <= MODBUS_IN16_CAL)
      {
    	  input_cal[address - MODBUS_IN1_CAL] = ((uint16_t)*(bufadd+0 + 4)<<8) + *(bufadd + 5);
    	  if(address == MODBUS_IN1_CAL)
    		  save_uint16_to_flash(FLASH_IN1_CAL,input_cal[0]);
    	  else if(address == MODBUS_IN2_CAL)
    		  save_uint16_to_flash(FLASH_IN2_CAL,input_cal[1]);
    	  else if(address == MODBUS_IN3_CAL)
    		  save_uint16_to_flash(FLASH_IN3_CAL,input_cal[2]);
    	  else if(address == MODBUS_IN4_CAL)
    		  save_uint16_to_flash(FLASH_IN4_CAL,input_cal[3]);
    	  else if(address == MODBUS_IN5_CAL)
    		  save_uint16_to_flash(FLASH_IN5_CAL,input_cal[4]);
    	  else if(address == MODBUS_IN6_CAL)
    		  save_uint16_to_flash(FLASH_IN6_CAL,input_cal[5]);
    	  else if(address == MODBUS_IN7_CAL)
    		  save_uint16_to_flash(FLASH_IN7_CAL,input_cal[6]);
    	  else if(address == MODBUS_IN8_CAL)
    		  save_uint16_to_flash(FLASH_IN8_CAL,input_cal[7]);
    	  else if(address == MODBUS_IN9_CAL)
    		  save_uint16_to_flash(FLASH_IN9_CAL,input_cal[8]);
    	  else if(address == MODBUS_IN10_CAL)
    		  save_uint16_to_flash(FLASH_IN10_CAL,input_cal[9]);
    	  else if(address == MODBUS_IN11_CAL)
    		  save_uint16_to_flash(FLASH_IN11_CAL,input_cal[10]);
    	  else if(address == MODBUS_IN12_CAL)
    		  save_uint16_to_flash(FLASH_IN12_CAL,input_cal[11]);
    	  else if(address == MODBUS_IN13_CAL)
    		  save_uint16_to_flash(FLASH_IN13_CAL,input_cal[12]);
    	  else if(address == MODBUS_IN14_CAL)
    		  save_uint16_to_flash(FLASH_IN14_CAL,input_cal[13]);
    	  else if(address == MODBUS_IN15_CAL)
    		  save_uint16_to_flash(FLASH_IN15_CAL,input_cal[14]);
    	  else if(address == MODBUS_IN16_CAL)
    		  save_uint16_to_flash(FLASH_IN16_CAL,input_cal[15]);
      }
      else if(address >= MODBUS_TEST_1 && address <= MODBUS_TEST_50)
      {
    	  Test[address - MODBUS_TEST_1] = (((uint16_t)*(bufadd+0 + 4)<<8) + *(bufadd + 5));
      }
      else if(address >= MODBUS_TIMER_ADDRESS && address <= MODBUS_TIMER_ADDRESS + 6)
     {
		if(address - MODBUS_TIMER_ADDRESS == 0)
			rtc_date.second = *(bufadd + 5);// sec
		else if(address - MODBUS_TIMER_ADDRESS == 1)
			rtc_date.minute = *(bufadd + 5);
		else if(address - MODBUS_TIMER_ADDRESS == 2)  // sec
			rtc_date.hour = *(bufadd + 5);
		else if(address - MODBUS_TIMER_ADDRESS == 3)  // sec
			rtc_date.day = *(bufadd + 5);
		else if(address - MODBUS_TIMER_ADDRESS == 4)  // sec
			rtc_date.weekday = *(bufadd + 5);
		else if(address - MODBUS_TIMER_ADDRESS == 5)  // sec
			rtc_date.month = *(bufadd + 5);
		else if(address - MODBUS_TIMER_ADDRESS == 6)  // sec
			rtc_date.year = (((uint16_t)*(bufadd+0 + 4)<<8) + *(bufadd + 5));
		PCF_SetDateTime(&rtc_date);
		update_timers();
	  }
      else if((address >= MODBUS_IO_REG_START && address <= MODBUS_IO_REG_END) ||
    		  (address >= MODBUS_EXIO_REG_START && address <= MODBUS_EXIO_REG_END))
      {
    	  Write_IO_reg(address,bufadd);
      }

   }
}


uint16_t read_IO_reg(uint16_t addr)
{
	uint8_t sendbuf[2];
	Str_points_ptr ptr;
	if((addr >= MODBUS_OUTPUT_FIRST && addr <= MODBUS_OUTPUT_LAST) ||
		(addr >= MODBUS_EXOUTPUT_FIRST && addr <= MODBUS_EXOUTPUT_LAST))
	{
		U8_T index;
		U16_T base;
		if((addr >= MODBUS_OUTPUT_FIRST && addr <= MODBUS_OUTPUT_LAST))
		{
			base = MODBUS_OUTPUT_FIRST;
			index = (addr - base) / 2;
		}
		else
		{
			base = MODBUS_EXOUTPUT_FIRST;
			index = (addr - base) / 2 + 64;
		}

		ptr = put_io_buf(OUT,index);
		if((addr - base) % 2 == 0)  // high word
		{
			if(ptr.pout->digital_analog == 0) // digtial
			{
				sendbuf[0] = 0;
				sendbuf[1] = 0;
			}
			else
			{
				sendbuf[0] = (U8_T)(ptr.pout->value >> 24);
				sendbuf[1] = (U8_T)(ptr.pout->value >> 16);
			}
		}
		else // low word
		{
			if(ptr.pout->digital_analog == 0) // digtial
			{
				if((ptr.pout->range >= ON_OFF  && ptr.pout->range <= HIGH_LOW)
					||(ptr.pout->range >= custom_digital1 // customer digital unit
					&& ptr.pout->range <= custom_digital8
					&& digi_units[ptr.pout->range - custom_digital1].direct == 1))
				{  // inverse logic
					if(ptr.pout->control == 1)
					{
						sendbuf[0] = 0;
						sendbuf[1] = 0;
					}
					else
					{
						sendbuf[0] = 0;
						sendbuf[1] = 1;
					}
				}
				else
				{
					if(ptr.pout->control == 1)
					{
						sendbuf[0] = 0;
						sendbuf[1] = 1;
					}
					else
					{
						sendbuf[0] = 0;
						sendbuf[1] = 0;
					}
				}
			}
			else  // analog
			{
				sendbuf[0] = (U8_T)(ptr.pout->value >> 8);
				sendbuf[1] = (U8_T)(ptr.pout->value);
			}
		}
	}
	else if(addr >= MODBUS_OUTPUT_SWICH_FIRST && addr <= MODBUS_OUTPUT_SWICH_LAST)
	{
		U8_T index;
		index = (addr - MODBUS_OUTPUT_SWICH_FIRST);
		ptr = put_io_buf(OUT,index);
		sendbuf[0] = 0;
		sendbuf[1] = ptr.pout->switch_status;
	}
	else if(addr >= MODBUS_OUTPUT_RANGE_FIRST && addr <= MODBUS_OUTPUT_RANGE_LAST)
	{
		U8_T index;
		index = (addr - MODBUS_OUTPUT_RANGE_FIRST);
		ptr = put_io_buf(OUT,index);
		sendbuf[0] = 0;
		sendbuf[1] = ptr.pout->range;
	}
	else if(addr >= MODBUS_OUTPUT_AM_FIRST && addr <= MODBUS_OUTPUT_AM_LAST)
	{
		U8_T index;
		index = (addr - MODBUS_OUTPUT_AM_FIRST);
		ptr = put_io_buf(OUT,index);
		sendbuf[0] = 0;
		sendbuf[1] = ptr.pout->auto_manual;
	}
	else if(addr >= MODBUS_OUTPUT_AD_FIRST && addr <= MODBUS_OUTPUT_AD_LAST)
	{
		U8_T index;
		index = (addr - MODBUS_OUTPUT_AD_FIRST);
		ptr = put_io_buf(OUT,index);
		sendbuf[0] = 0;
		sendbuf[1] = ptr.pout->digital_analog;
	}
		// end output
		// start input
	else if((addr >= MODBUS_INPUT_FIRST && addr <= MODBUS_INPUT_LAST)
	 || (addr >= MODBUS_EXINPUT_FIRST && addr <= MODBUS_EXINPUT_LAST))
	{
		U8_T index;
		U16_T base;
		if((addr >= MODBUS_INPUT_FIRST && addr <= MODBUS_INPUT_LAST))
		{
			base = MODBUS_INPUT_FIRST;
			index = (addr - base) / 2;
		}
		else
		{
			base = MODBUS_EXINPUT_FIRST;
			index = (addr - base) / 2 + 64;
		}

		ptr = put_io_buf(IN,index);

		if((addr - base) % 2 == 0)
		{
			if(ptr.pin->digital_analog == 0)  // digital
			{
				sendbuf[0] = 0;
				sendbuf[1] = 0;
			}
			else
			{
				sendbuf[0] = (U8_T)(ptr.pin->value >> 24);
				sendbuf[1] = (U8_T)(ptr.pin->value >> 16);
			}
		}
		else
		{
			if(ptr.pin->digital_analog == 0)  // digital
			{
				if((ptr.pin->range >= ON_OFF  && ptr.pin->range <= HIGH_LOW)
					||(ptr.pin->range >= custom_digital1 // customer digital unit
					&& ptr.pin->range <= custom_digital8
					&& digi_units[ptr.pin->range - custom_digital1].direct == 1))
				{  // inverse logic
					if(ptr.pin->control == 1)
					{
						sendbuf[0] = 0;
						sendbuf[1] = 0;
					}
					else
					{
						sendbuf[0] = 0;
						sendbuf[1] = 1;
					}
				}
				else
				{
					if(ptr.pin->control == 1)
					{
						sendbuf[0] = 0;
						sendbuf[1] = 1;
					}
					else
					{
						sendbuf[0] = 0;
						sendbuf[1] = 0;
					}
				}
			}
			else  // analog
			{
				sendbuf[0] = (U8_T)(ptr.pin->value >> 8);
				sendbuf[1] = (U8_T)(ptr.pin->value);
			}
		}

	 }
	 else if(addr >= MODBUS_INPUT_FILTER_FIRST && addr <= MODBUS_INPUT_FILTER_LAST)
	 {
			U8_T index;
			index = (addr - MODBUS_INPUT_FILTER_FIRST);
			ptr = put_io_buf(IN,index);
			sendbuf[0] = 0;
			sendbuf[1] = ptr.pin->filter;
	 }
	 else if(addr >= MODBUS_INPUT_CAL_FIRST && addr <= MODBUS_INPUT_CAL_LAST)
	 {
			U8_T index;
			index = (addr - MODBUS_INPUT_CAL_FIRST);
			ptr = put_io_buf(IN,index);
			sendbuf[0] = ptr.pin->calibration_hi;
			sendbuf[1] = ptr.pin->calibration_lo;
	 }
	 else if(addr >= MODBUS_INPUT_RANGE_FIRST && addr <= MODBUS_INPUT_RANGE_LAST)
	 {
			U8_T index;
			index = (addr - MODBUS_INPUT_RANGE_FIRST);
			ptr = put_io_buf(IN,index);
			sendbuf[0] = ptr.pin->digital_analog;
			sendbuf[1] = ptr.pin->range;
	 }
	 else if(addr >= MODBUS_INPUT_AM_FIRST && addr <= MODBUS_INPUT_AM_LAST)
	 {
			U8_T index;
			index = (addr - MODBUS_INPUT_AM_FIRST);
			ptr = put_io_buf(IN,index);
			sendbuf[0] = 0;
			sendbuf[1] = ptr.pin->auto_manual;
	 }
	 else if(addr >= MODBUS_INPUT_CAL_SIGN_FIRST && addr <= MODBUS_INPUT_CAL_SIGN_LAST)
	 {
			U8_T index;
			index = (addr - MODBUS_INPUT_CAL_SIGN_FIRST);
			ptr = put_io_buf(IN,index);
			sendbuf[0] = 0;
			sendbuf[1] = ptr.pin->calibration_sign;

	 }
// end input
// start variable
	 else if((addr >= MODBUS_VAR_FIRST && addr <= MODBUS_VAR_LAST) ||
		(addr >= MODBUS_EXVAR_FIRST && addr <= MODBUS_EXVAR_LAST))
	 {
		U8_T index;
		U16_T base;
		 if(addr >= MODBUS_VAR_FIRST && addr <= MODBUS_VAR_LAST)
		 {
			 base = MODBUS_VAR_FIRST;
			 index = (addr - base) / 2;
		 }
		 else
		 {
			 base = MODBUS_EXVAR_FIRST;
			 index = (addr - base) / 2 + 128;
		 }

		ptr = put_io_buf(VAR,index);
		if((addr - base) % 2 == 0)   // high word
		{
			if(ptr.pvar->digital_analog == 0)  // digital
			{
				sendbuf[0] = 0;
				sendbuf[1] = 0;
			}
			else
			{
				sendbuf[0] = (U8_T)(ptr.pvar->value >> 24);
				sendbuf[1] = (U8_T)(ptr.pvar->value >> 16);
			}
		}
		else   // low word
		{
			if(ptr.pvar->digital_analog == 0)  // digital
			{
				if((ptr.pvar->range >= ON_OFF  && ptr.pvar->range <= HIGH_LOW)
					||(ptr.pvar->range >= custom_digital1 // customer digital unit
					&& ptr.pvar->range <= custom_digital8
					&& digi_units[ptr.pvar->range - custom_digital1].direct == 1))
				{  // inverse logic
					if(ptr.pvar->control == 1)
					{
						sendbuf[0] = 0;
						sendbuf[1] = 0;
					}
					else
					{
						sendbuf[0] = 0;
						sendbuf[1] = 1;
					}
				}
				else
				{
					if(ptr.pvar->control == 1)
					{
						sendbuf[0] = 0;
						sendbuf[1] = 1;
					}
					else
					{
						sendbuf[0] = 0;
						sendbuf[1] = 0;
					}
				}
			}
			else  // analog
			{
				sendbuf[0] = (U8_T)(ptr.pvar->value >> 8);
				sendbuf[1] = (U8_T)(ptr.pvar->value);
			}
		}
	}
	else if(addr >= MODBUS_VAR_AM_FIRST && addr <= MODBUS_VAR_AM_LAST)
	{
		U8_T index;
		index = addr - MODBUS_VAR_AM_FIRST;
		ptr = put_io_buf(VAR,index);
		sendbuf[0] = 0;
		sendbuf[1] = ptr.pvar->auto_manual;
	}
	else if(addr >= MODBUS_VAR_RANGE_FIRST && addr <= MODBUS_VAR_RANGE_LAST)
	{
		U8_T index;
		index = addr - MODBUS_VAR_RANGE_FIRST;
		ptr = put_io_buf(VAR,index);
		sendbuf[0] = 0;
		sendbuf[1] = ptr.pvar->range;
	}
	else if(addr >= MODBUS_VAR_AD_FIRST && addr <= MODBUS_VAR_AD_LAST)
	{
		U8_T index;
		index = addr - MODBUS_VAR_AD_FIRST;
		ptr = put_io_buf(VAR,index);
		sendbuf[0] = 0;
		sendbuf[1] = ptr.pvar->digital_analog;
	}
// end variable
// start weekly
	else if(addr >= MODBUS_WR_AM_FIRST && addr <= MODBUS_WR_AM_LAST)
	{
		sendbuf[0] = 0;
		sendbuf[1] = weekly_routines[addr - MODBUS_WR_AM_FIRST].auto_manual;
	}
	else if(addr >= MODBUS_WR_OUT_FIRST && addr <= MODBUS_WR_OUT_LAST)
	{
		sendbuf[0] = 0;
		sendbuf[1] = weekly_routines[addr - MODBUS_WR_OUT_FIRST].value;
	}
	else if(addr >= MODBUS_WR_HOLIDAY1_FIRST && addr <= MODBUS_WR_HOLIDAY1_LAST)
	{
		sendbuf[0] = 0;
		sendbuf[1] = weekly_routines[addr - MODBUS_WR_HOLIDAY1_FIRST].override_1.number;
	}
	else if(addr >= MODBUS_WR_STATE1_FIRST && addr <= MODBUS_WR_STATE1_LAST)
	{
		sendbuf[0] = 0;
		sendbuf[1] = weekly_routines[addr - MODBUS_WR_STATE1_FIRST].override_1_value;
	}
	else if(addr >= MODBUS_WR_HOLIDAY2_FIRST && addr <= MODBUS_WR_HOLIDAY2_LAST)
	{
		sendbuf[0] = 0;
		sendbuf[1] = weekly_routines[addr - MODBUS_WR_HOLIDAY2_FIRST].override_2.number;
	}
	else if(addr >= MODBUS_WR_STATE2_FIRST && addr <= MODBUS_WR_STATE2_LAST)
	{
		sendbuf[0] = 0;
		sendbuf[1] = weekly_routines[addr - MODBUS_WR_STATE2_FIRST].override_2_value;
	}
// weekly_time
//					else if(StartAdd + loop >= MODBUS_WR_TIME_FIRST && StartAdd + loop <= MODBUS_WR_TIME_LAST)
//					{
//						U8_T i,j,k;
//						i = (StartAdd + loop - MODBUS_WR_TIME_FIRST) / (MAX_SCHEDULES_PER_WEEK * 8); // week index
//						j = (StartAdd + loop - MODBUS_WR_TIME_FIRST) %(MAX_SCHEDULES_PER_WEEK * 8) / 8; // day index
//						k = (StartAdd + loop - MODBUS_WR_TIME_FIRST) % (MAX_SCHEDULES_PER_WEEK * 8) % 8;  // seg index
//						sendbuf[HeadLen + 3 + loop * 2] = wr_times[i][j].time[k].hours;
//						sendbuf[HeadLen + 3 + loop * 2 + 1] = wr_times[i][j].time[k].minutes;
//					}
// end weekly

// start annual
	else if(addr >= MODBUS_AR_AM_FIRST && addr <= MODBUS_AR_AM_LAST)
	{
		sendbuf[0] = 0;
		sendbuf[1] = annual_routines[addr - MODBUS_AR_AM_FIRST].auto_manual;
	}
	else if(addr >= MODBUS_AR_OUT_FIRST && addr <= MODBUS_AR_OUT_LAST)
	{
		sendbuf[0] = 0;
		sendbuf[1] = annual_routines[addr - MODBUS_AR_OUT_FIRST].value;
	}
	else if(addr >= MODBUS_AR_TIME_FIRST && addr <= MODBUS_AR_TIME_LAST)
	{
		U8_T i,j;
		i = (addr - MODBUS_AR_TIME_FIRST) / AR_DATES_SIZE;
		j = (addr - MODBUS_AR_TIME_FIRST) % AR_DATES_SIZE;
		sendbuf[0] = 0;
		sendbuf[1] = ar_dates[i][j];
	}
	return sendbuf[0] * 256 + sendbuf[1];
}

void Write_IO_reg(uint16_t StartAdd,uint8_t * pData)
{
	uint8_t i = 0;
	Str_points_ptr ptr;
	if((StartAdd  >= MODBUS_OUTPUT_FIRST && StartAdd  <= MODBUS_OUTPUT_LAST) ||
			(StartAdd  >= MODBUS_EXOUTPUT_FIRST && StartAdd  <= MODBUS_EXOUTPUT_LAST))
	{
		int32_t tempval;
		uint32_t base;
		if(StartAdd  >= MODBUS_OUTPUT_FIRST && StartAdd  <= MODBUS_OUTPUT_LAST)
		{
			base = MODBUS_OUTPUT_FIRST;
			i = (StartAdd - base) / 2;
		}
		else
		{
			base = MODBUS_EXOUTPUT_FIRST;
			i = (StartAdd - base) / 2 + 64;
		}

		ptr = put_io_buf(OUT,i);
		tempval = ptr.pout->value;
		if((StartAdd - base) % 2 == 0)  // high word
		{
			tempval &= 0x0000ffff;
			tempval += 65536L * (pData[5] +  256 * pData[4]);
		}
		else  // low word
		{
			S32_T old_value = 0;

			tempval &= 0xffff0000;
			tempval += (pData[5] + 256 * pData[4]);

			if(ptr.pout->digital_analog == 0)  // digital
			{
				if(i < get_max_internal_output())
				{
					if(( ptr.pout->range >= ON_OFF && ptr.pout->range <= HIGH_LOW )
						||(ptr.pout->range >= custom_digital1 // customer digital unit
						&& ptr.pout->range <= custom_digital8
						&& digi_units[ptr.pout->range - custom_digital1].direct == 1))
					{ // inverse
						if(output_priority[i][9] == 0xff)
							output_priority[i][7] = (pData[5]+ (U16_T)(pData[4] << 8)) ? 0 : 1;
						else
							output_priority[i][10] = (pData[5]+ (U16_T)(pData[4] << 8)) ? 0 : 1;
						ptr.pout->control = Binary_Output_Present_Value(i) ? 0 : 1;
					}
					else
					{
						if(output_priority[i][9] == 0xff)
							output_priority[i][7] = (pData[5]+ (U16_T)(pData[4] << 8)) ? 1 : 0;
						else
							output_priority[i][10] = (pData[5]+ (U16_T)(pData[4] << 8)) ? 1 : 0;
						ptr.pout->control = Binary_Output_Present_Value(i) ? 1 : 0;
					}
				}
				else
				{
					old_value = ptr.pout->control;

					if(( ptr.pout->range >= ON_OFF && ptr.pout->range <= HIGH_LOW )
						||(ptr.pout->range >= custom_digital1 // customer digital unit
						&& ptr.pout->range <= custom_digital8
						&& digi_units[ptr.pout->range - custom_digital1].direct == 1))
					{// inverse
						ptr.pout->control = (pData[5]+ (U16_T)(pData[4] << 8)) ? 0 : 1;
					}
					else
					{
						ptr.pout->control =  (pData[5]+ (U16_T)(pData[4] << 8)) ? 1 : 0;
					}
				}

				if(ptr.pout->control)
					set_output_raw(i,1000);
				else
					set_output_raw(i,0);

				if(i >= get_max_internal_output())
				{
					if(old_value != ptr.pout->control)
					{
						if(i < base_out)
						{
							push_expansion_out_stack(ptr.pout,i,0);
						}
					}
				}

				ptr.pout->value = Binary_Output_Present_Value(i) * 1000;

			}
			else if(ptr.pout->digital_analog)
			{
				if(i < get_max_internal_output())
				{
					if(output_priority[i][9] == 0xff)
						output_priority[i][7] = (float)(pData[5]+ (U16_T)(pData[4] << 8)) / 1000;
					else
						output_priority[i][10] = (float)(pData[5]+ (U16_T)(pData[4] << 8)) / 1000;
				}
				// if external io

				if(i >= get_max_internal_output())
				{
					old_value = ptr.pout->value;

					output_raw[i] = (float)(pData[5]+ (U16_T)(pData[4] << 8));
					if(old_value != (float)(pData[5]+ (U16_T)(pData[4] << 8)))
					{
						if(i < base_out)
						{
							push_expansion_out_stack(ptr.pout,i,0);
						}
					}
				}

				ptr.pout->value = Analog_Output_Present_Value(i) * 1000;

			}
		}

#if OUTPUT_DEATMASTER
		clear_dead_master();
#endif

		if(i >= get_max_internal_output())
		{
			if( ptr.pout->range >= OFF_ON && ptr.pout->range <= LOW_HIGH )
			{
				if(pData[5]+ (U16_T)(pData[4] << 8) == 1)
						ptr.pout->control = 1;
					else
						ptr.pout->control = 0;
			}
			ptr.pout->value = tempval;
			push_expansion_out_stack(ptr.pout,i,0);
		}

		ChangeFlash = 1;
	}
	else if(StartAdd  >= MODBUS_OUTPUT_RANGE_FIRST && StartAdd  <= MODBUS_OUTPUT_RANGE_LAST)
	{
		i = (StartAdd - MODBUS_OUTPUT_RANGE_FIRST);
		ptr = put_io_buf(OUT,i);
		ptr.pout->range = pData[5];
		ChangeFlash = 1;

		push_expansion_out_stack(ptr.pout,i,1);
	}
	else if(StartAdd  >= MODBUS_OUTPUT_AM_FIRST && StartAdd  <= MODBUS_OUTPUT_AM_LAST)
	{
		i = (StartAdd - MODBUS_OUTPUT_AM_FIRST);
		ptr = put_io_buf(OUT,i);
		if(ptr.pout->switch_status == SW_AUTO)
			ptr.pout->auto_manual = pData[5];
		ChangeFlash = 1;

		push_expansion_out_stack(ptr.pout,i,1);
	}
	else if(StartAdd  >= MODBUS_OUTPUT_AD_FIRST && StartAdd  <= MODBUS_OUTPUT_AD_LAST)
	{
		i = (StartAdd - MODBUS_OUTPUT_AD_FIRST);
		ptr = put_io_buf(OUT,i);
		ptr.pout->digital_analog = pData[5];
		ChangeFlash = 1;
		push_expansion_out_stack(ptr.pout,i,1);
	}
	else if((StartAdd  >= MODBUS_INPUT_FIRST && StartAdd  <= MODBUS_INPUT_LAST) ||
			(StartAdd  >= MODBUS_EXINPUT_FIRST && StartAdd  <= MODBUS_EXINPUT_LAST))
	{
		int32_t tempval;
		uint32_t base;
		if(StartAdd  >= MODBUS_INPUT_FIRST && StartAdd  <= MODBUS_INPUT_LAST)
		{
			base = MODBUS_INPUT_FIRST;
			i = (StartAdd - base) / 2;
		}
		else
		{
			base = MODBUS_EXINPUT_FIRST;
			i = (StartAdd - base) / 2 + 64;
		}

		ptr = put_io_buf(IN,i);
		tempval = ptr.pin->value;
		if((StartAdd - base) % 2 == 0)  // high word
		{
			tempval &= 0x0000ffff;
			tempval += 65536L * (pData[5] +  256 * pData[4]);
		}
		else  // low word
		{
			tempval &= 0xffff0000;
			tempval += (pData[5] + 256 * pData[4]);

			if(ptr.pin->digital_analog == 0)  // digital
			{
				if(( ptr.pin->range >= ON_OFF && ptr.pin->range <= HIGH_LOW )
				||(ptr.pin->range >= custom_digital1 // customer digital unit
				&& ptr.pin->range <= custom_digital8
				&& digi_units[ptr.pin->range - custom_digital1].direct == 1))
				{ // inverse
					if(pData[5]+ (U16_T)(pData[4] << 8) == 1)
						ptr.pin->control = 0;
					else
						ptr.pin->control = 1;
				}
				else
				{
					if(pData[5]+ (U16_T)(pData[4] << 8) == 1)
						ptr.pin->control = 1;
					else
						ptr.pin->control = 0;
				}
			}
			else if(ptr.pin->digital_analog == 1)
			{
				 ptr.pin->value = 1000l * (pData[5]+ (U16_T)(pData[4] << 8));
			}

			if(ptr.pin->auto_manual == 1)  // manual
			{
				/*if((ptr.pin->range == HI_spd_count) || (ptr.pin->range == N0_2_32counts)
					|| (ptr.pin->range == RPM)
				)
				{
					if(swap_double(ptr.pin->value) == 0)
					{
						high_spd_counter[i] = 0; // clear high spd count
						clear_high_spd[i] = 1;

					}
				}*/
			}
		}

		ptr.pin->value = tempval;
		ChangeFlash = 1;

		push_expansion_in_stack(ptr.pin);
	}
	else if(StartAdd  >= MODBUS_INPUT_FILTER_FIRST && StartAdd  <= MODBUS_INPUT_FILTER_LAST)
	{
		i = (StartAdd - MODBUS_INPUT_FILTER_FIRST);
		ptr = put_io_buf(IN,i);
		ptr.pin->filter = pData[5];
		ChangeFlash = 1;

		push_expansion_in_stack(ptr.pin);
	}
	else if(StartAdd  >= MODBUS_INPUT_CAL_FIRST && StartAdd  <= MODBUS_INPUT_CAL_LAST)
	{
		i = (StartAdd - MODBUS_INPUT_CAL_FIRST);
		ptr = put_io_buf(IN,i);
		ptr.pin->calibration_hi = pData[4];
		ptr.pin->calibration_lo = pData[5];
		ChangeFlash = 1;
		push_expansion_in_stack(ptr.pin);
	}
	else if(StartAdd  >= MODBUS_INPUT_CAL_SIGN_FIRST && StartAdd  <= MODBUS_INPUT_CAL_SIGN_LAST)
	{
		i = (StartAdd - MODBUS_INPUT_CAL_SIGN_FIRST);
		ptr = put_io_buf(IN,i);
		ptr.pin->calibration_sign = pData[5];
		ChangeFlash = 1;
		push_expansion_in_stack(ptr.pin);
	}
	else if(StartAdd  >= MODBUS_INPUT_RANGE_FIRST && StartAdd  <= MODBUS_INPUT_RANGE_LAST)
	{
		i = (StartAdd - MODBUS_INPUT_RANGE_FIRST);
		ptr = put_io_buf(IN,i);
		ptr.pin->digital_analog = pData[4];
		ptr.pin->range = pData[5];
		ChangeFlash = 1;
		push_expansion_in_stack(ptr.pin);
	}
	else if(StartAdd  >= MODBUS_INPUT_AM_FIRST && StartAdd  <= MODBUS_INPUT_AM_LAST)
	{
		i = (StartAdd - MODBUS_INPUT_AM_FIRST);
		ptr = put_io_buf(IN,i);
		ptr.pin->auto_manual = pData[5];
		ChangeFlash = 1;
		push_expansion_in_stack(ptr.pin);
	}
	/*else if(StartAdd  >= MODBUS_INPUT_HI_SPD_EN_FIRST && StartAdd  <= MODBUS_INPUT_HI_SPD_EN_LAST)
	{
		i = (StartAdd - MODBUS_INPUT_HI_SPD_EN_FIRST);
		high_spd_en[i] = pData[5];
		ChangeFlash = 1;
	}
	else if(StartAdd  >= MODBUS_INPUT_TYPE_FIRST && StartAdd  <= MODBUS_INPUT_TYPE_LAST)
	{
		i = (StartAdd - MODBUS_INPUT_TYPE_FIRST);
		input_type[i] = pData[5];
	}*/
	else if((StartAdd  >= MODBUS_VAR_FIRST && StartAdd  <= MODBUS_VAR_LAST) ||
			(StartAdd  >= MODBUS_EXVAR_FIRST && StartAdd  <= MODBUS_EXVAR_LAST))
	{
		int32_t tempval;
		uint32_t base;

		if(StartAdd  >= MODBUS_EXVAR_FIRST && StartAdd  <= MODBUS_EXVAR_LAST)
		{
			base = MODBUS_VAR_FIRST;
			i = (StartAdd - base) / 2;
		}
		else
		{
			base = MODBUS_EXVAR_FIRST;
			i = (StartAdd - base) / 2 + 128;
		}


		ptr = put_io_buf(VAR,i);
		tempval = ptr.pvar->value;
		if((StartAdd - base) % 2 == 0)  // high word
		{
			tempval &= 0x0000ffff;
			tempval += 65536L * (pData[5] +  256 * pData[4]);
		}
		else  // low word
		{
			tempval &= 0xffff0000;
			tempval += (pData[5] + 256 * pData[4]);

			if(ptr.pvar->digital_analog == 0)  // digital
			{
				if(( ptr.pvar->range >= ON_OFF && ptr.pvar->range <= HIGH_LOW )
					||(ptr.pvar->range >= custom_digital1 // customer digital unit
					&& ptr.pvar->range <= custom_digital8
					&& digi_units[ptr.pvar->range - custom_digital1].direct == 1))
				{// inverse
					if(pData[5]+ (U16_T)(pData[4] << 8) == 1)
						ptr.pvar->control = 0;
					else
						ptr.pvar->control = 1;
				}
				else
				{
					if(pData[5]+ (U16_T)(pData[4] << 8) == 1)
						ptr.pvar->control = 1;
					else
						ptr.pvar->control = 0;
				}
			}
		}

		ptr.pvar->value = tempval;
		ChangeFlash = 1;

	}
	else if(StartAdd  >= MODBUS_VAR_AM_FIRST && StartAdd  <= MODBUS_VAR_AM_LAST)
	{
		i = (StartAdd - MODBUS_VAR_AM_FIRST);
		ptr = put_io_buf(VAR,i);
		ptr.pvar->auto_manual = pData[5];
		ChangeFlash = 1;
	}
	else if(StartAdd  >= MODBUS_VAR_AD_FIRST && StartAdd  <= MODBUS_VAR_AD_LAST)
	{
		i = (StartAdd - MODBUS_VAR_AD_FIRST);
		ptr = put_io_buf(VAR,i);
		ptr.pvar->digital_analog = pData[5];
		ChangeFlash = 1;
	}
	else if(StartAdd  >= MODBUS_VAR_RANGE_FIRST && StartAdd  <= MODBUS_VAR_RANGE_LAST)
	{
		i = (StartAdd - MODBUS_VAR_RANGE_FIRST);
		ptr = put_io_buf(VAR,i);
		ptr.pvar->range = pData[5];
		ChangeFlash = 1;
	}
	else if(StartAdd >= MODBUS_WR_AM_FIRST && StartAdd <= MODBUS_WR_AM_LAST)
	{
		i = (StartAdd - MODBUS_WR_AM_FIRST);		
		weekly_routines[i].auto_manual = pData[5];
		ChangeFlash = 1;
	}
	else if(StartAdd >= MODBUS_WR_OUT_FIRST && StartAdd <= MODBUS_WR_OUT_LAST)
	{
		i = (StartAdd - MODBUS_WR_OUT_FIRST);
		weekly_routines[i].value = pData[5] ? 1 : 0;
		ChangeFlash = 1;
	}
	else if(StartAdd >= MODBUS_WR_HOLIDAY1_FIRST && StartAdd <= MODBUS_WR_HOLIDAY1_LAST)
	{
		i = (StartAdd - MODBUS_WR_HOLIDAY1_FIRST);
		weekly_routines[i].override_1.number = pData[5];
		ChangeFlash = 1;
	}
	else if(StartAdd >= MODBUS_WR_STATE1_FIRST && StartAdd <= MODBUS_WR_STATE1_LAST)
	{
		i = (StartAdd - MODBUS_WR_STATE1_FIRST);
		weekly_routines[i].override_1_value = pData[5];
		ChangeFlash = 1;
	}
	else if(StartAdd >= MODBUS_WR_HOLIDAY2_FIRST && StartAdd <= MODBUS_WR_HOLIDAY2_LAST)
	{
		i = (StartAdd - MODBUS_WR_HOLIDAY2_FIRST);
		weekly_routines[i].override_2.number = pData[5];
		ChangeFlash = 1;
	}
	else if(StartAdd >= MODBUS_WR_STATE2_FIRST && StartAdd <= MODBUS_WR_STATE2_LAST)
	{
		i = (StartAdd - MODBUS_WR_STATE2_FIRST);
		weekly_routines[i].override_2_value = pData[5];
		ChangeFlash = 1;
	}

// weekly_time
	else if(StartAdd >= MODBUS_WR_TIME_FIRST && StartAdd <= MODBUS_WR_TIME_LAST)
	{
		U8_T i,j,k;
		i = (StartAdd - MODBUS_WR_TIME_FIRST) / (MAX_SCHEDULES_PER_WEEK * 8); // week index
		j = (StartAdd - MODBUS_WR_TIME_FIRST) %(MAX_SCHEDULES_PER_WEEK * 8) / 8; // day index
		k = (StartAdd - MODBUS_WR_TIME_FIRST) % (MAX_SCHEDULES_PER_WEEK * 8) % 8;  // seg index
		wr_times[i][j].time[k].hours = pData[4];
		wr_times[i][j].time[k].minutes = pData[5];
		ChangeFlash = 1;
	}
// end weekly

// start annual
	else if(StartAdd >= MODBUS_AR_AM_FIRST && StartAdd <= MODBUS_AR_AM_LAST)
	{
		i = (StartAdd - MODBUS_AR_AM_FIRST);
		annual_routines[i].auto_manual = pData[5];
		ChangeFlash = 1;
	}
	else if(StartAdd >= MODBUS_AR_OUT_FIRST && StartAdd <= MODBUS_AR_OUT_LAST)
	{
		i = (StartAdd - MODBUS_AR_OUT_FIRST);
		annual_routines[i].value = pData[5] ? 1 : 0;
		ChangeFlash = 1;
	}
	else if(StartAdd >= MODBUS_AR_TIME_FIRST && StartAdd <= MODBUS_AR_TIME_LAST)
	{
		U8_T i,j;
		i = (StartAdd - MODBUS_AR_TIME_FIRST) / AR_DATES_SIZE;
		j = (StartAdd - MODBUS_AR_TIME_FIRST) % AR_DATES_SIZE;
		ar_dates[i][j] = pData[5];
		ChangeFlash = 1;
	}
}

void MulWrite_IO_reg(uint16_t StartAdd,uint8_t * pData)
{
	uint16_t i;
	Str_points_ptr ptr;
	if((StartAdd >= MODBUS_OUTPUT_FIRST && StartAdd <= MODBUS_OUTPUT_LAST) ||
			(StartAdd >= MODBUS_EXOUTPUT_FIRST && StartAdd <= MODBUS_EXOUTPUT_LAST))
	{
		if(pData[5] == 0x02)
		{
			int32_t tempval;
			uint32_t base;

			if(StartAdd >= MODBUS_OUTPUT_FIRST && StartAdd <= MODBUS_OUTPUT_LAST)
			{
				base = MODBUS_OUTPUT_FIRST;
				i = (StartAdd - base) / 2;
			}
			else
			{
				base = MODBUS_EXOUTPUT_FIRST;
				i = (StartAdd - base) / 2 + 64;
			}

			ptr = put_io_buf(OUT,i);
			tempval = pData[10] + (U16_T)(pData[9] << 8) \
				+ ((U32_T)pData[8] << 16) + ((U32_T)pData[7] << 24);
			
			ptr.pout->value = tempval;

			if(ptr.pout->digital_analog == 0)  // digital
			{
				if(( ptr.pout->range >= ON_OFF && ptr.pout->range <= HIGH_LOW )
					||(ptr.pout->range >= custom_digital1 // customer digital unit
					&& ptr.pout->range <= custom_digital8
					&& digi_units[ptr.pout->range - custom_digital1].direct == 1))
				{// inverse
					if(tempval == 1)
						ptr.pout->control = 0;
					else
						ptr.pout->control = 1;
				}
				else
				{
					if(tempval == 1)
						ptr.pout->control = 1;
					else
						ptr.pout->control = 0;
				}

				/*if(ptr.pout->control)  tbd:
					set_output_raw(i,1000);
				else
					set_output_raw(i,0);*/
			}
			else
			{
				// set output_raw
				//????????
			}
		}
		ChangeFlash = 1;
	}
	else if((StartAdd >= MODBUS_INPUT_FIRST && StartAdd <= MODBUS_INPUT_LAST) ||
			(StartAdd >= MODBUS_EXINPUT_FIRST && StartAdd <= MODBUS_EXINPUT_LAST))
	{
		if(pData[5] == 0x02)
		{
			int32_t tempval;
			uint32_t base;
			if(StartAdd >= MODBUS_INPUT_FIRST && StartAdd <= MODBUS_INPUT_LAST)
			{
				base = MODBUS_INPUT_FIRST;
				i = (StartAdd - base) / 2;
			}
			else
			{
				base = MODBUS_EXINPUT_FIRST;
				i = (StartAdd - base) / 2 + 64;
			}
			ptr = put_io_buf(IN,i);
			tempval = pData[10] + (U16_T)(pData[9] << 8) \
				+ ((U32_T)pData[8] << 16) + ((U32_T)pData[7] << 24);
				
			
			
			if(ptr.pin->digital_analog == 0)  // digital
			{
				if(( ptr.pin->range >= ON_OFF && ptr.pin->range <= HIGH_LOW )
					||(ptr.pin->range >= custom_digital1 // customer digital unit
					&& ptr.pin->range <= custom_digital8
					&& digi_units[ptr.pin->range - custom_digital1].direct == 1))
				{// inverse
					if(tempval == 1)
						ptr.pin->control = 0;
					else
						ptr.pin->control = 1;
				}
				else
				{
					if(tempval == 1)
						ptr.pin->control = 1;
					else
						ptr.pin->control = 0;
				}
			}

			ptr.pin->value = tempval;
		}
		ChangeFlash = 1;
	}
	else if((StartAdd >= MODBUS_VAR_FIRST && StartAdd <= MODBUS_VAR_LAST) ||
			(StartAdd >= MODBUS_EXVAR_FIRST && StartAdd <= MODBUS_EXVAR_LAST))
	{
		if(pData[5] == 0x02)
		{
			int32_t tempval;
			uint32_t base;

			if(StartAdd >= MODBUS_VAR_FIRST && StartAdd <= MODBUS_VAR_LAST)
			{
				base = MODBUS_VAR_FIRST;
				i = (StartAdd - MODBUS_VAR_FIRST) / 2;
			}
			else
			{
				base = MODBUS_EXVAR_FIRST;
				i = (StartAdd - MODBUS_EXVAR_FIRST) / 2 + 128;
			}

			ptr = put_io_buf(VAR,i);
			tempval = pData[10] + (U16_T)(pData[9] << 8) \
				+ ((U32_T)pData[8] << 16) + ((U32_T)pData[7] << 24);

			
			if(ptr.pvar->digital_analog == 0)  // digital
			{
				if(( ptr.pvar->range >= ON_OFF && ptr.pvar->range <= HIGH_LOW )
					||(ptr.pvar->range >= custom_digital1 // customer digital unit
					&& ptr.pvar->range <= custom_digital8
					&& digi_units[ptr.pvar->range - custom_digital1].direct == 1))
				{// inverse
					if(tempval == 1)
						ptr.pvar->control = 0;
					else
						ptr.pvar->control = 1;
				}
				else
				{
					if(tempval == 1)
						ptr.pvar->control = 1;
					else
						ptr.pvar->control = 0;
				}
				//ptr.pvar->value = ptr.pvar->control ? 1000 : 0;
			}

			ptr.pvar->value = tempval;
		}
		ChangeFlash = 1;

	}
}
void set_default_parameters(void);


void dealwith_write_setting(Str_Setting_Info * ptr)
{
// compare sn to check whether it is current panel

	if(ptr->reg.sn == (Modbus.serialNum[0] + (U16_T)(Modbus.serialNum[1] << 8)	+ ((U32_T)Modbus.serialNum[2] << 16) + ((U32_T)Modbus.serialNum[3] << 24)))
	{
		if(memcmp(panelname,ptr->reg.panel_name,20))
		{
			ptr->reg.panel_name[19] = 0;
			memcpy(panelname,ptr->reg.panel_name,20);
			Set_Object_Name(panelname);
		}

		if(Modbus.en_time_sync_with_pc != ptr->reg.en_time_sync_with_pc)
		{
			flag_Update_Sntp = 0;
			Update_Sntp_Retry = 0;
			count_sntp = 0;
			// start SYNC with PC
			Setting_Info.reg.update_time_sync_pc = 1;
			Modbus.en_time_sync_with_pc = ptr->reg.en_time_sync_with_pc;
			save_uint8_to_flash(FLASH_EN_TIME_SYNC_PC,Modbus.en_time_sync_with_pc);
		}

		if(timezone != ptr->reg.time_zone)
		{
			Sync_timestamp(ptr->reg.time_zone,timezone,0,0);
			timezone = ptr->reg.time_zone;
			save_uint8_to_flash( FLASH_TIME_ZONE, timezone);
		}
		if(Daylight_Saving_Time!= ptr->reg.Daylight_Saving_Time)
		{
			Sync_timestamp(0,0,ptr->reg.Daylight_Saving_Time,Daylight_Saving_Time);
			Daylight_Saving_Time = ptr->reg.Daylight_Saving_Time;
//			update_timers();
			save_uint8_to_flash( FLASH_DSL, Daylight_Saving_Time);
		}

		if((Modbus.en_sntp != ptr->reg.en_sntp) || ((Modbus.en_sntp == 5) && memcmp(sntp_server,Setting_Info.reg.sntp_server,30)))
		{
			Modbus.en_sntp = ptr->reg.en_sntp;
			if(Modbus.en_sntp <= 5)
			{
				save_uint8_to_flash(FLASH_EN_SNTP,Modbus.en_sntp);
				if(Modbus.en_sntp >= 2)
				{
						if(Modbus.en_sntp == 5)  // defined by customer
						{
							memcpy(sntp_server,Setting_Info.reg.sntp_server,30);
							Save_SNTP_sever();
						}
						sntp_select_time_server(Modbus.en_sntp);
						flag_Update_Sntp = 0;
						Update_Sntp_Retry = 0;
						count_sntp = 0;

						//SNTPC_Start(timezone, (((U32_T)SntpServer[0]) << 24) | ((U32_T)SntpServer[1] << 16) | ((U32_T)SntpServer[2] << 8) | (SntpServer[3]));

				}
			}
		}
#if 0
//!(ARM_TSTAT_WIFI )
		if(Modbus.en_dyndns != ptr->reg.en_dyndns)
		{
			Modbus.en_dyndns = ptr->reg.en_dyndns;
			if(Modbus.en_dyndns == 1)
			{
				dyndns_select_domain(dyndns_provider);
			// reconnect dyndns server again
				flag_Update_Dyndns = 0;
				Update_Dyndns_Retry = 0;
				DynDNS_Init();
			}
			E2prom_Write_Byte(EEP_EN_DYNDNS,Modbus.en_dyndns);

		}

		if(dyndns_provider != ptr->reg.dyndns_provider)
		{
			dyndns_provider = ptr->reg.dyndns_provider;
			E2prom_Write_Byte(EEP_DYNDNS_PROVIDER,dyndns_provider);
		}
		if(dyndns_update_time != swap_word(ptr->reg.dyndns_update_time))
		{
			dyndns_update_time = swap_word(ptr->reg.dyndns_update_time);
//							if(dyndns_update_time < 6)	dyndns_update_time = 6;
			E2prom_Write_Byte(EEP_DYNDNS_UPDATE_LO,dyndns_update_time);
			E2prom_Write_Byte(EEP_DYNDNS_UPDATE_HI,dyndns_update_time >> 8);
		}

		memcpy(dyndns_domain_name,ptr->reg.dyndns_domain,MAX_DOMAIN_SIZE);
		memcpy(dyndns_username,ptr->reg.dyndns_user,MAX_USERNAME_SIZE);
		memcpy(dyndns_password,ptr->reg.dyndns_pass,MAX_PASSWORD_SIZE);
#endif

		memcpy(Modbus.mac_addr,ptr->reg.mac_addr,6);
		if(Modbus.com_config[0] != ptr->reg.com_config[0])
		{

			Modbus.com_config[0] = ptr->reg.com_config[0];
			//if(Modbus.com_config[0] == BACNET_SLAVE || Modbus.com_config[0] == BACNET_MASTER)
			//	Recievebuf_Initialize(0);

			Count_com_config();
			save_uint8_to_flash( FLASH_UART_CONFIG, Modbus.com_config[0]);
		}
		/*if(Modbus.com_config[1] != ptr->reg.com_config[1])
		{
			Modbus.com_config[1] = ptr->reg.com_config[1];
			if((Modbus.com_config[1] == MODBUS_SLAVE) || (Modbus.com_config[1] == 0))
				;//uart_serial_restart(1);
			if(Modbus.com_config[1] == MODBUS_MASTER)
			{
				Count_com_config();
				count_send_id_to_zigbee = 0;

			}
			Count_com_config();

#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI )
#if (ARM_MINI || ASIX_MINI)
			if((Modbus.mini_type == MINI_BIG_ARM) || (Modbus.mini_type == MINI_SMALL_ARM))
			{
				if(Modbus.com_config[1] == MODBUS_MASTER)
					UART1_SW = 1;
				else
					UART1_SW = 0;
			}
#endif
#endif
			//E2prom_Write_Byte(EEP_COM1_CONFIG, Modbus.com_config[1]);
		}*/
		if(Modbus.com_config[2] != ptr->reg.com_config[2])
		{
			Modbus.com_config[2] = ptr->reg.com_config[2];
            Count_com_config();
           	save_uint8_to_flash( FLASH_UART2_CONFIG, Modbus.com_config[2]);
		}

		if(Modbus.uart_parity[0] != ptr->reg.uart_parity[0])
		{
			Modbus.uart_parity[0] = ptr->reg.uart_parity[0];
			//E2prom_Write_Byte(EEP_UART0_PARITY, Modbus.uart_parity[0]);
			//UART_Init(0);
		}
		if(Modbus.uart_parity[2] != ptr->reg.uart_parity[2])
		{
			Modbus.uart_parity[2] = ptr->reg.uart_parity[2];
			//E2prom_Write_Byte(EEP_UART2_PARITY, Modbus.uart_parity[2]);
			//UART_Init(2);
		}
#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI )
		if(Modbus.uart_stopbit[0] != ptr->reg.uart_stopbit[0])
		{
			Modbus.uart_stopbit[0] = ptr->reg.uart_stopbit[0];
			E2prom_Write_Byte(EEP_UART0_STOPBIT, Modbus.uart_stopbit[0]);
			UART_Init(0);
		}
		if(Modbus.uart_stopbit[2] != ptr->reg.uart_stopbit[2])
		{
			Modbus.uart_stopbit[2] = ptr->reg.uart_stopbit[2];
			E2prom_Write_Byte(EEP_UART2_STOPBIT, Modbus.uart_stopbit[2]);
			UART_Init(2);
		}
#endif

		if(Modbus.en_username != ptr->reg.en_username)
		{
			Modbus.en_username = ptr->reg.en_username;
			//E2prom_Write_Byte(EEP_USER_NAME,Modbus.en_username);
			save_uint8_to_flash( FLASH_EN_USERNAME, Modbus.en_username);
		}
		if(Modbus.cus_unit != ptr->reg.cus_unit)
		{
			Modbus.cus_unit = ptr->reg.cus_unit;
			//E2prom_Write_Byte(EEP_USER_NAME,Modbus.cus_unit);
		}

		if(Modbus.address != ptr->reg.modbus_id)
		{
			if(ptr->reg.modbus_id > 0 && ptr->reg.modbus_id < 255)
			{
				Modbus.address = ptr->reg.modbus_id;
				//E2prom_Write_Byte(EEP_ADDRESS,Modbus.address);
				Station_NUM = Modbus.address;
				Setting_Info.reg.MSTP_ID = Station_NUM;
				dlmstp_init(NULL);
				save_uint8_to_flash( FLASH_MODBUS_ID, Modbus.address);
			}
		}
		if((Station_NUM != ptr->reg.MSTP_ID) && (ptr->reg.MSTP_ID != 0) && (ptr->reg.MSTP_ID != 255))
		{
			Station_NUM = ptr->reg.MSTP_ID;
			//E2prom_Write_Byte(EEP_STATION_NUM,Station_NUM);
			Setting_Info.reg.MSTP_ID = Station_NUM;
			Modbus.address = Station_NUM;
			//E2prom_Write_Byte(EEP_ADDRESS,  Modbus.address);
// reboot mstp communication

			dlmstp_init(NULL);

			save_uint8_to_flash( FLASH_MODBUS_ID, Modbus.address);
		}

		if(panel_number != ptr->reg.panel_number)
		{
			if(ptr->reg.panel_number > 0 && ptr->reg.panel_number < 255)
			{
				panel_number = ptr->reg.panel_number;
				//E2prom_Write_Byte(EEP_PANEL_NUMBER,panel_number);
				change_panel_number_in_code(Setting_Info.reg.panel_number,panel_number);
				Setting_Info.reg.panel_number	= panel_number;
				Modbus.address = panel_number;
				Station_NUM = panel_number;
				save_uint8_to_flash( FLASH_MODBUS_ID, Modbus.address);
			}
		}
		if(Modbus.network_number != (ptr->reg.network_number + 256L * ptr->reg.network_number_hi))
		{
			Modbus.network_number = ptr->reg.network_number + 256L * ptr->reg.network_number_hi;
			save_uint16_to_flash( FLASH_NETWORK_NUMBER, Modbus.network_number);
		}

		if(Modbus.mstp_network != ptr->reg.mstp_network_number)
		{
			Modbus.mstp_network = ptr->reg.mstp_network_number;
			save_uint16_to_flash( FLASH_MSTP_NETWORK, Modbus.mstp_network);
		}

		if((Instance != ptr->reg.instance) && (ptr->reg.instance != 0))
		{
			Instance = ptr->reg.instance;
			Device_Set_Object_Instance_Number(Instance);
			save_uint8_to_flash(FLASH_INSTANCE1,Instance);
			save_uint8_to_flash(FLASH_INSTANCE2,Instance >> 8);
			save_uint8_to_flash(FLASH_INSTANCE3,Instance >> 16);
			save_uint8_to_flash(FLASH_INSTANCE4,Instance >> 24);
		}

//		if(Modbus.refresh_flash_timer != ptr->reg.refresh_flash_timer)
//		{
//			Modbus.refresh_flash_timer = ptr->reg.refresh_flash_timer;
//			E2prom_Write_Byte(EEP_REFRESH_FLASH, Modbus.refresh_flash_timer );
//		}
		if(Modbus.external_nodes_plug_and_play != ptr->reg.en_plug_n_play)
		{
			Modbus.external_nodes_plug_and_play = ptr->reg.en_plug_n_play;
			//E2prom_Write_Byte(EEP_EN_NODE_PLUG_N_PLAY, Modbus.external_nodes_plug_and_play);
		}

		if(Modbus.baudrate[0] != ptr->reg.com_baudrate[0]) // com_baudrate[2]??T3000
		{
			Modbus.baudrate[0] = ptr->reg.com_baudrate[0];

			/*if((Modbus.com_config[0] == MODBUS_SLAVE) || (Modbus.com_config[0] == NOUSE) || (Modbus.com_config[0] == MODBUS_MASTER)
				|| (Modbus.com_config[0] == BACNET_SLAVE) || (Modbus.com_config[0] == BACNET_MASTER))*/
			save_uint8_to_flash(FLASH_BAUD_RATE, Modbus.baudrate[0]);
			flag_change_uart0 = 1;
			count_change_uart0 = 0;
#if 0
			uart_init(0);
			Count_com_config();
#endif
		}
		if(Modbus.baudrate[2] != ptr->reg.com_baudrate[2]) // com_baudrate[2]??T3000
		{
			Modbus.baudrate[2] = ptr->reg.com_baudrate[2];

			/*if((Modbus.com_config[2] == MODBUS_SLAVE) || (Modbus.com_config[2] == NOUSE) || (Modbus.com_config[2] == MODBUS_MASTER)
				|| (Modbus.com_config[2] == BACNET_SLAVE) || (Modbus.com_config[2] == BACNET_MASTER))*/
			save_uint8_to_flash(FLASH_BAUD_RATE2, Modbus.baudrate[2]);
			flag_change_uart2 = 1;
			count_change_uart2 = 0;
#if 0
			uart_init(2);
			Count_com_config();
#endif
		}

		if(ptr->reg.reset_default == 88)	// reset default
		{
			ptr->reg.reset_default = 0;
			set_default_parameters();
		}
		if(ptr->reg.reset_default == 77)	 // show identify
		{
			ptr->reg.reset_default = 0;
			// show identify
			// control led on/off
			gIdentify = 1;
			count_gIdentify = 0;
		}


		if(ptr->reg.reset_default == 111)	 // reboot
		{
			if(system_timer / 1000 > 10)
				esp_restart();//flag_reboot = 1;//SoftReset();
			ptr->reg.reset_default = 0;
		}
		if(ptr->reg.reset_default == 150)	 // clear db
		{
			ptr->reg.reset_default = 0;
			clear_scan_db();
		}



		if((memcmp(Modbus.ip_addr,ptr->reg.ip_addr,4) && !((ptr->reg.ip_addr[0] == 0) && (ptr->reg.ip_addr[1] == 0) \
				&& (ptr->reg.ip_addr[2] == 0) && (ptr->reg.ip_addr[3] == 0)))
			|| (memcmp(Modbus.subnet,ptr->reg.subnet,4) && !((ptr->reg.subnet[0] == 0) && (ptr->reg.subnet[1] == 0) \
				&& (ptr->reg.subnet[2] == 0) && (ptr->reg.subnet[3] == 0)))
		|| (memcmp(Modbus.getway,ptr->reg.getway,4) && !((ptr->reg.getway[0] == 0) && (ptr->reg.getway[1] == 0) \
				&& (ptr->reg.getway[2] == 0) && (ptr->reg.getway[3] == 0)))
		|| (Modbus.tcp_port != (ptr->reg.tcp_port) && (Modbus.tcp_port != 0))
		|| (Modbus.tcp_type != ptr->reg.tcp_type)
		)
		{
			if(memcmp(Modbus.ip_addr,ptr->reg.ip_addr,4) && !((ptr->reg.ip_addr[0] == 0) && (ptr->reg.ip_addr[1] == 0) \
				&& (ptr->reg.ip_addr[2] == 0) && (ptr->reg.ip_addr[3] == 0)))
			{
				memcpy(Modbus.ip_addr,ptr->reg.ip_addr,4);
				if(Modbus.com_config[2] == BACNET_SLAVE || Modbus.com_config[2] == BACNET_MASTER
					|| Modbus.com_config[0] == BACNET_SLAVE || Modbus.com_config[0] == BACNET_MASTER)
				{
					Send_I_Am_Flag = 1;
				}
			}
			if(memcmp(Modbus.subnet,ptr->reg.subnet,4) && !((ptr->reg.subnet[0] == 0) && (ptr->reg.subnet[1] == 0) \
				&& (ptr->reg.subnet[2] == 0) && (ptr->reg.subnet[3] == 0)))
			{
				memcpy(Modbus.subnet,ptr->reg.subnet,4);
			}
			if(memcmp(Modbus.getway,ptr->reg.getway,4) && !((ptr->reg.getway[0] == 0) && (ptr->reg.getway[1] == 0) \
				&& (ptr->reg.getway[2] == 0) && (ptr->reg.getway[3] == 0)))
			{

				memcpy(Modbus.getway,ptr->reg.getway,4);
			}
			if(Modbus.tcp_port != (ptr->reg.tcp_port) && (Modbus.tcp_port != 0))
			{
				Modbus.tcp_port = (ptr->reg.tcp_port);
				save_uint16_to_flash( FLASH_TCP_PORT,Modbus.tcp_port);
			}
			if(Modbus.tcp_type != ptr->reg.tcp_type)
			{
				Modbus.tcp_type = ptr->reg.tcp_type;
				save_uint8_to_flash( FLASH_TCP_TYPE,Modbus.tcp_type);
			}
			Eth_IP_Change = 1;
			ip_change_count = 0;

		}

#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI )
		if(MAX_MASTER != ptr->reg.MAX_MASTER)
		{
			MAX_MASTER = ptr->reg.MAX_MASTER;
			E2prom_Write_Byte(EEP_MAX_MASTER, ptr->reg.MAX_MASTER);
		}
#endif
		if(webview_json_flash != ptr->reg.webview_json_flash)
		{
			webview_json_flash = ptr->reg.webview_json_flash;
			save_uint8_to_flash( FLASH_JASON,ptr->reg.webview_json_flash);
		}
		if(Modbus.start_month != ptr->reg.start_month)
		{
			Modbus.start_month = ptr->reg.start_month;
			//E2prom_Write_Byte(EEP_DLS_START_MON, ptr->reg.start_month);
			Calculate_DSL_Time();
		}
		if(Modbus.start_day != ptr->reg.start_day)
		{
			Modbus.start_day = ptr->reg.start_day;
			//E2prom_Write_Byte(EEP_DLS_START_DAY, ptr->reg.start_day);
			Calculate_DSL_Time();
		}
		if(Modbus.end_month != ptr->reg.end_month)
		{
			Modbus.end_month = ptr->reg.end_month;
			//E2prom_Write_Byte(EEP_DLS_END_MON, ptr->reg.end_month);
			Calculate_DSL_Time();
		}
		if(Modbus.end_day != ptr->reg.end_day)
		{
			Modbus.end_day = ptr->reg.end_day;
			//E2prom_Write_Byte(EEP_DLS_END_DAY, ptr->reg.end_day);
			Calculate_DSL_Time();
		}
#if NEW_IO
			if(max_vars != ptr->reg.max_var)
			{
				max_vars = ptr->reg.max_var;
				save_uint8_to_flash(FLASH_MAX_VARS,max_vars);
				//free(new_vars);
				new_vars = NULL;
				init_panel();
				Flash_Inital();
				save_point_info(0);
			}
			if(max_inputs != ptr->reg.max_in)
			{
				max_inputs = ptr->reg.max_in;
				save_uint8_to_flash(FLASH_MAX_INS,max_inputs);
				//free(new_inputs);
				new_inputs = NULL;
				Flash_Inital();
				init_panel();
				save_point_info(0);
			}
			if(max_outputs != ptr->reg.max_out)
			{
				max_outputs = ptr->reg.max_out;
				save_uint8_to_flash(FLASH_MAX_OUTS,max_outputs);
				//free(new_outputs);
				new_outputs = NULL;
				Flash_Inital();
				init_panel();
				save_point_info(0);
			}
#endif

			if(memcmp(lcddisplay,ptr->reg.display_lcd.lcddisplay,sizeof(lcdconfig)))
			{
				memcpy(lcddisplay,ptr->reg.display_lcd.lcddisplay,sizeof(lcdconfig));

				// clear first screen
				/*disp_str(FORM15X30, 6,  32, "     ",SCH_COLOR,TSTAT8_BACK_COLOR);
				disp_ch(0,FIRST_CH_POS,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
				disp_ch(0,SECOND_CH_POS,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
				disp_ch(0,THIRD_CH_POS - 16,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
				disp_ch(0,THIRD_CH_POS - 16 + 48,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);*/
				// save it to flash memory
				Save_Lcd_config();
			}


	 }
}

void responseCmd(uint8 type,uint8* pData)
{
	//uint16_t ptr16[10];
	//uint8_t ptr8[10];
	responseModbusCmd(BAC_TO_MODBUS, pData, 0,NULL,NULL,0);
}

