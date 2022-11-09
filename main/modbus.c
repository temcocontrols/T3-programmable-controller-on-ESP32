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


uint32_t my_htonl(uint32_t val)
{
	/*uint8_t t1,t2,t3,t4;
	t1 = val >> 24;
	t2 = val >> 16;
	t3 = val >> 8;
	t4 = val;

	return ((uint32_t)t4 << 24) + ((uint32_t)t3 << 16) +  ((uint16_t)t2 << 16) + t1;*/
	return val;


}

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

#define SUB_EN_PIN_SEL	(1ULL<<GPIO_SUB_EN_PIN)
#define MAIN_EN_PIN_SEL	(1ULL<<GPIO_MAIN_EN_PIN)
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

extern uint8_t gIdentify;
extern uint8_t count_gIdentify;

void dealwith_write_setting(Str_Setting_Info * ptr);
uint16_t read_user_data_by_block(uint16_t addr);
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
		uart_param_config(uart_num_main, &uart_config);
		uart_set_pin(uart_num_main, GPIO_NUM_12, GPIO_NUM_15, GPIO_MAIN_EN_PIN, UART_PIN_NO_CHANGE);
		uart_driver_install(uart_num_main, MB_BUF_SIZE * 2, 0, 0, NULL, 0);
		uart_set_mode(uart_num_main, UART_MODE_RS485_HALF_DUPLEX);
	}
}



bool checkdata(uint8_t* data)
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
	}
	return true;
}
void debug_info(char *string);



extern uint8 led_sub_tx;
extern uint8 led_sub_rx;
extern uint8 led_main_tx;
extern uint8 led_main_rx;
void modbus0_task(void *arg)
{
	uint8_t modbus_send_buf[500];
	uint16_t modbus_send_len;
	memset(modbus_send_buf,0,500);
	modbus_send_len = 0;
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
	uint8_t* uart_rsv = (uint8_t*)malloc(512);


	debug_info("modbous initial \r\n");
	//Modbus.com_config[0] = MODBUS_SLAVE;
	while (1) {
		//Test[21] = inputs[0].range + 5;

			if(Modbus.com_config[0] == MODBUS_SLAVE)
			{
				int len = uart_read_bytes(uart_num_sub, uart_rsv, 512, 100 / portTICK_RATE_MS);

				if(len>0)
				{led_sub_rx++;
					com_rx[0]++;
					flagLED_sub_rx = 1;
					if(checkdata(uart_rsv))
					{
						if(uart_rsv[1] == TEMCO_MODBUS)	// temco private modbus
						{
							handler_private_transfer(uart_rsv,0,NULL,0xa0);
						}
						init_crc16();
						responseModbusCmd(SERIAL, uart_rsv, len,modbus_send_buf,&modbus_send_len,0);
					}
				}

			}
			else
			{
				
				int len = uart_read_bytes(uart_num_sub, uart_rsv, 512, 100 / portTICK_RATE_MS);

				if(len>0)
				{
					led_sub_rx++;
					com_rx[0]++;
					flagLED_sub_rx = 1;
					if(checkdata(uart_rsv))
					{
						Modbus.com_config[0] = MODBUS_SLAVE;
					}
				}
				vTaskDelay(500 / portTICK_RATE_MS);
			}

		}

}

void modbus2_task(void *arg)
{
	uint8_t modbus_send_buf[500];
	uint16_t modbus_send_len;
	memset(modbus_send_buf,0,500);
	modbus_send_len = 0;
	setup_reg_data();
	uint8_t* uart_rsv = (uint8_t*)malloc(512);
	//Test[2] = 100;
	while (1) {
		if(Modbus.com_config[2] == MODBUS_SLAVE)
		{
			int len = uart_read_bytes(uart_num_main, uart_rsv, 512, 100 / portTICK_RATE_MS);

			if(len>0)
			{
				led_main_rx++;
				com_rx[2]++;
				flagLED_main_rx = 1;

				if(checkdata(uart_rsv))
				{
					if(uart_rsv[1] == TEMCO_MODBUS)	// temco private modbus
					{
						handler_private_transfer(uart_rsv,0,NULL,0xa2);
					}
					init_crc16();
					responseModbusCmd(SERIAL, uart_rsv, len,modbus_send_buf,&modbus_send_len,2);
				}
			}

		}
		else
		{		
			int len = uart_read_bytes(uart_num_main, uart_rsv, 512, 100 / portTICK_RATE_MS);

			if(len>0)
			{
				led_main_rx++;
				com_rx[2]++;
				flagLED_main_rx = 1;
				if(checkdata(uart_rsv))
				{
					Modbus.com_config[2] = MODBUS_SLAVE;
				}
			}
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
   else{
      headlen = 0;
      uart_send = malloc(512);
   }
   if(*(bufadd + 1 + headlen) == WRITE_VARIABLES){
      if(type == WIFI){ // for wifi
         for(i = 0; i < 6; i++)
            uart_sendB[UIP_HEAD + send_cout++] = *(bufadd + i + headlen);

         uart_sendB[0] = *bufadd;//0;         //   TransID
         uart_sendB[1] = *(bufadd + 1);//TransID++;
         uart_sendB[2] = 0;         //   ProtoID
         uart_sendB[3] = 0;
         uart_sendB[4] = 0;   //   Len
         uart_sendB[5] = 6;

         //memcpy(modbus_wifi_buf,uart_sendB,UIP_HEAD + send_cout);
         //modbus_wifi_len = UIP_HEAD + send_cout;
      }else{
         //uart_write_bytes(UART_NUM_0, "WRITE_VARIABLES\r\n", sizeof("WRITE_VARIABLES\r\n"));
         for(i = 0; i < rece_size; i++)
            uart_send[send_cout++] = *(bufadd+i);
         if(type == BAC_TO_MODBUS){
        	
        	 memcpy(&bacnet_to_modbus,&bufadd[3],bufadd[5] * 2);
         }
         else
         {
        	
            uart_send_string((const char *)uart_send, send_cout,port);
         }
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
           
            memcpy(&bacnet_to_modbus,&uart_send[3],reg_num*2);
         }
         else{
            uart_send[6] = CRChi;
            uart_send[7] = CRClo;
            uart_send_string((const char *)uart_send, 8,port);
            
            return;
         }
      }else{
         uart_sendB[0] = *bufadd;//0;         //   TransID
         uart_sendB[1] = *(bufadd + 1);//TransID++;
         uart_sendB[2] = 0;         //   ProtoID
         uart_sendB[3] = 0;
         uart_sendB[4] = 0;   //   Len
         uart_sendB[5] = 6;
         //memcpy(modbus_wifi_buf,uart_sendB,UIP_HEAD + send_cout);
         //modbus_wifi_len = UIP_HEAD + send_cout;
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
         else if(address >= MODBUS_TIMER_ADDRESS && address <= MODBUS_TIMER_ADDRESS + 6)
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
/******************* read IN OUT by block start ******************************************/
		else if( address >= MODBUS_USER_BLOCK_FIRST && address <= MODBUS_USER_BLOCK_LAST)
		{
			U16_T temp;
			temp = read_user_data_by_block(address);
			temp2 = (temp >> 8) & 0xFF;//temp & 0xFF;
			temp1 = temp & 0xFF;//(temp >> 8) & 0xFF;
		}
/*********************read IN OUT by block endf ***************************************/
		else if( address >= MODBUS_IO_REG_START && address <= MODBUS_IO_REG_END)
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
         {// tbd:
        	 temp1 = 0;
        	 temp2 = 0x55;
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
            uart_sendB[0] = TransID >> 8;         //   TransID
            uart_sendB[1] = TransID;
            uart_sendB[2] = 0;         //   ProtoID
            uart_sendB[3] = 0;
            uart_sendB[4] = (3 + num * 2) >> 8;   //   Len
            uart_sendB[5] = (uint8_t)(3 + num * 2) ;
         }
      }
   }else if(*(bufadd+1 + headlen) == CHECKONLINE){
      if(type!=WIFI){
         uart_send[send_cout++] = 0xff;
         uart_send[send_cout++] = 0x19;
         uart_send[send_cout++] = Modbus.address;
         uart_send[send_cout++] = Modbus.serialNum[0];
         uart_send[send_cout++] = Modbus.serialNum[1];
         uart_send[send_cout++] = Modbus.serialNum[2];
         uart_send[send_cout++] = Modbus.serialNum[3];
         for(i=0;i<send_cout;i++)
            crc16_byte(uart_send[i]);
      }
   }else
      return;

   temp1 = CRChi ;
   temp2 = CRClo;
   if(type == BAC_TO_MODBUS){

          memcpy(&bacnet_to_modbus,&uart_send[3],num*2);
           }
   if(type == WIFI)
   {
      memcpy(resData,uart_sendB,UIP_HEAD + send_cout);
      *modbus_send_len = UIP_HEAD + send_cout;
      free(uart_sendB);
   }else{
      uart_send[send_cout++] = temp1 ;
      uart_send[send_cout++] = temp2 ;
      uart_send_string((const char *)uart_send, send_cout,port);
     
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
   else
   {
      reg_num = pData[4]*256 + pData[5];
      responseModbusData(pData,SERIAL,len,resData,modbus_send_len,port);
      internalDeal(pData,SERIAL);
   }
}


void write_user_data_by_block(U16_T StartAdd,U8_T HeadLen,U8_T *pData)
{
	U8_T far i,j;

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
			memcpy(&outputs[i],&pData[HeadLen + 7],sizeof(Str_out_point));

			check_output_priority_array(i,0);

		}
	}
	else if(StartAdd  >= MODBUS_INPUT_BLOCK_FIRST && StartAdd  <= MODBUS_INPUT_BLOCK_LAST)
	{
		if((StartAdd - MODBUS_INPUT_BLOCK_FIRST) % ((sizeof(Str_in_point) + 1	) / 2) == 0)
		{
			i = (StartAdd - MODBUS_INPUT_BLOCK_FIRST) / ((sizeof(Str_in_point) + 1) / 2);
			memcpy(&inputs[i],&pData[HeadLen + 7],sizeof(Str_in_point));

		}
	}
	else if(StartAdd  >= MODBUS_VAR_BLOCK_FIRST && StartAdd  <= MODBUS_VAR_BLOCK_LAST)
	{
		if((StartAdd - MODBUS_VAR_BLOCK_FIRST) % ((sizeof(Str_variable_point) + 1	) / 2) == 0)
		{
			i = (StartAdd - MODBUS_VAR_BLOCK_FIRST) / ((sizeof(Str_variable_point) + 1) / 2);
			memcpy(&vars[i],&pData[HeadLen + 7],sizeof(Str_variable_point));

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
		block = (U16_T *)&outputs[index];
		item = (addr - MODBUS_OUTPUT_BLOCK_FIRST) % ((sizeof(Str_out_point) + 1) / 2);
	}
	else if( addr >= MODBUS_INPUT_BLOCK_FIRST && addr <= MODBUS_INPUT_BLOCK_LAST )
	{
		index = (addr - MODBUS_INPUT_BLOCK_FIRST) / ((sizeof(Str_in_point) + 1) / 2);
		block = (U16_T *)&inputs[index];
		item = (addr - MODBUS_INPUT_BLOCK_FIRST) % ((sizeof(Str_in_point) + 1) / 2);
	}
	else if( addr >= MODBUS_VAR_BLOCK_FIRST && addr <= MODBUS_VAR_BLOCK_LAST )
	{
		index = (addr - MODBUS_VAR_BLOCK_FIRST) / ((sizeof(Str_variable_point) + 1) / 2);
		block = (U16_T *)&vars[index];
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
      save_block(FLASH_BLOCK1_SSID);//save_wifi_info();
   }
   else if(StartAdd >= MODBUS_WIFI_PASS_START && StartAdd <= MODBUS_WIFI_PASS_END)
   {
      if((StartAdd - MODBUS_WIFI_PASS_START) % 16 == 0)
      {
         memset(&SSID_Info.password,'\0',32);
         memcpy(&SSID_Info.password,&pData[HeadLen + 7],32);
      }
      save_block(FLASH_BLOCK1_SSID);//save_wifi_info();
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
      else if(temp_i >= MODBUS_IO_REG_START && temp_i <= MODBUS_IO_REG_END)
      {

    	  MulWrite_IO_reg(address,bufadd);
      }
   }
   if(*(bufadd+1) == WRITE_VARIABLES)
   {
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
	  if(address == MODBUS_WIFI_START)
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
			  wifi_init_sta();
		  }
	  }
      if(address == MODBUS_ADDRESS)
      {
         if((*(bufadd + 5)!=0)&&(*(bufadd + 5)!=0xff))
         {
        	 Modbus.address = *(bufadd + 5);
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
				Instance = ((U32_T)(*(bufadd + 5)) << 16) + ((U32_T)(*(bufadd + 5)) << 24) + (U16_T)Instance;
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
            uart_init(0);
         }
      }
      else if(address == MODBUS_UART2_BAUDRATE)
		{
		   if(*(bufadd + 5)<10)
		   {
			  Modbus.baudrate[2] = *(bufadd + 5);
			  save_uint8_to_flash( FLASH_BAUD_RATE, Modbus.baudrate[2]);
			  uart_init(2);
		   }
		}

	  else if(address == MODBUS_COM0_TYPE)
		{
		   if(*(bufadd + 5) < MAX_COM_TYPE)
		   {
			  Modbus.com_config[0] = *(bufadd + 5);
			  save_uint8_to_flash( FLASH_UART_CONFIG, Modbus.com_config[0]);
			 // uart_init(0);
			  Count_com_config();
		   }
		}
		else if(address == MODBUS_COM2_TYPE)
		{
		   if(*(bufadd + 5) < MAX_COM_TYPE)
		   {
			  Modbus.com_config[2] = *(bufadd + 5);
			  save_uint8_to_flash( FLASH_UART2_CONFIG, Modbus.com_config[2]);
			 // uart_init(2);
			  Count_com_config();
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
		else if(address == MODBUS_NETWORK_NUMBER)
		{
			Modbus.network_number = (*(bufadd + 5)) + (*(bufadd + 4)) * 256;
			save_uint16_to_flash(FLASH_NETWORK_NUMBER,Modbus.network_number);
		}
      else if(address == MODBUS_OUTPUT_BLOCK_FIRST)
      {
         //save_uint16_to_flash(FLASH_INPUT_FLAG, 0);
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
	  }
      else if(address >= MODBUS_IO_REG_START && address <= MODBUS_IO_REG_END)
      {
    	  Write_IO_reg(address,bufadd);
      }

   }
}


uint16_t read_IO_reg(uint16_t addr)
{
	uint8_t sendbuf[2];
	if(addr >= MODBUS_OUTPUT_FIRST && addr <= MODBUS_OUTPUT_LAST)
	{
		U8_T index;

		index = (addr - MODBUS_OUTPUT_FIRST) / 2;
		if((addr - MODBUS_OUTPUT_FIRST) % 2 == 0)  // high word
		{
			if(outputs[index].digital_analog == 0) // digtial
			{
				sendbuf[0] = 0;
				sendbuf[1] = 0;
			}
			else
			{
				sendbuf[0] = (U8_T)(outputs[index].value >> 24);
				sendbuf[1] = (U8_T)(outputs[index].value >> 16);
			}
		}
		else // low word
		{
			if(outputs[index].digital_analog == 0) // digtial
			{
				if((outputs[index].range >= ON_OFF  && outputs[index].range <= HIGH_LOW)
					||(outputs[index].range >= custom_digital1 // customer digital unit
					&& outputs[index].range <= custom_digital8
					&& digi_units[outputs[index].range - custom_digital1].direct == 1))
				{  // inverse logic
					if(outputs[index].control == 1)
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
					if(outputs[index].control == 1)
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
				sendbuf[0] = (U8_T)(outputs[index].value >> 8);
				sendbuf[1] = (U8_T)(outputs[index].value);
			}
		}
	}
	else if(addr >= MODBUS_OUTPUT_SWICH_FIRST && addr <= MODBUS_OUTPUT_SWICH_LAST)
	{
		U8_T index;
		index = (addr - MODBUS_OUTPUT_SWICH_FIRST);
		sendbuf[0] = 0;
		sendbuf[1] = outputs[index].switch_status;
	}
	else if(addr >= MODBUS_OUTPUT_RANGE_FIRST && addr <= MODBUS_OUTPUT_RANGE_LAST)
	{
		U8_T index;
		index = (addr - MODBUS_OUTPUT_RANGE_FIRST);
		sendbuf[0] = 0;
		sendbuf[1] = outputs[index].range;
	}
	else if(addr >= MODBUS_OUTPUT_AM_FIRST && addr <= MODBUS_OUTPUT_AM_LAST)
	{
		U8_T index;
		index = (addr - MODBUS_OUTPUT_AM_FIRST);
		sendbuf[0] = 0;
		sendbuf[1] = outputs[index].auto_manual;
	}
	else if(addr >= MODBUS_OUTPUT_AD_FIRST && addr <= MODBUS_OUTPUT_AD_LAST)
	{
		U8_T index;
		index = (addr - MODBUS_OUTPUT_AD_FIRST);
		sendbuf[0] = 0;
		sendbuf[1] = outputs[index].digital_analog;
	}
		// end output
		// start input
	else if((addr >= MODBUS_INPUT_FIRST && addr <= MODBUS_INPUT_LAST)
	 /*|| (addr >= MODBUS_RI_FIRST && addr <= MODBUS_RI_LAST)*/)
	{
		U8_T index;
		U16_T base;
		if((addr >= MODBUS_INPUT_FIRST && addr <= MODBUS_INPUT_LAST))
			base = MODBUS_INPUT_FIRST;
		//else
		//	base = MODBUS_RI_FIRST;

		index = (addr - base) / 2;
		if((addr - base) % 2 == 0)
		{
			if(inputs[index].digital_analog == 0)  // digital
			{
				sendbuf[0] = 0;
				sendbuf[1] = 0;
			}
			else
			{
				sendbuf[0] = (U8_T)(inputs[index].value >> 24);
				sendbuf[1] = (U8_T)(inputs[index].value >> 16);
			}
		}
		else
		{
			if(inputs[index].digital_analog == 0)  // digital
			{
				if((inputs[index].range >= ON_OFF  && inputs[index].range <= HIGH_LOW)
					||(inputs[index].range >= custom_digital1 // customer digital unit
					&& inputs[index].range <= custom_digital8
					&& digi_units[inputs[index].range - custom_digital1].direct == 1))
				{  // inverse logic
					if(inputs[index].control == 1)
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
					if(inputs[index].control == 1)
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
				sendbuf[0] = (U8_T)(inputs[index].value >> 8);
				sendbuf[1] = (U8_T)(inputs[index].value);
			}
		}

	 }
	 else if(addr >= MODBUS_INPUT_FILTER_FIRST && addr <= MODBUS_INPUT_FILTER_LAST)
	 {
			U8_T index;
			index = (addr - MODBUS_INPUT_FILTER_FIRST);
			sendbuf[0] = 0;
			sendbuf[1] = inputs[index].filter;
	 }
	 else if(addr >= MODBUS_INPUT_CAL_FIRST && addr <= MODBUS_INPUT_CAL_LAST)
	 {
			U8_T index;
			index = (addr - MODBUS_INPUT_CAL_FIRST);
			sendbuf[0] = inputs[index].calibration_hi;
			sendbuf[1] = inputs[index].calibration_lo;
	 }
	 else if(addr >= MODBUS_INPUT_RANGE_FIRST && addr <= MODBUS_INPUT_RANGE_LAST)
	 {
			U8_T index;
			index = (addr - MODBUS_INPUT_RANGE_FIRST);
			sendbuf[0] = inputs[index].digital_analog;
			sendbuf[1] = inputs[index].range;
	 }
	 else if(addr >= MODBUS_INPUT_AM_FIRST && addr <= MODBUS_INPUT_AM_LAST)
	 {
			U8_T index;
			index = (addr - MODBUS_INPUT_AM_FIRST);
			sendbuf[0] = 0;
			sendbuf[1] = inputs[index].auto_manual;
	 }
	 else if(addr >= MODBUS_INPUT_CAL_SIGN_FIRST && addr <= MODBUS_INPUT_CAL_SIGN_LAST)
	 {
			U8_T index;
			index = (addr - MODBUS_INPUT_CAL_SIGN_FIRST);
			sendbuf[0] = 0;
			sendbuf[1] = inputs[index].calibration_sign;

	 }
	 /*else if(addr >= MODBUS_INPUT_HI_SPD_COUNTER_FIRST && addr <= MODBUS_INPUT_HI_SPD_COUNTER_LAST)
	 {
			U8_T index;
			index = (addr - MODBUS_INPUT_HI_SPD_COUNTER_FIRST) / 2;
			if((addr - MODBUS_INPUT_HI_SPD_COUNTER_FIRST) % 2 == 0)
			{
				sendbuf[0] = (U8_T)((get_high_spd_counter(index) / 1000) >> 24);
				sendbuf[1] = (U8_T)((get_high_spd_counter(index) / 1000) >> 16);
			}
			else
			{
				sendbuf[0] = (U8_T)((get_high_spd_counter(index) / 1000) >> 8);
				sendbuf[1] = (U8_T)(get_high_spd_counter(index) / 1000);
			}
	 }
	 else if(addr >= MODBUS_INPUT_HI_SPD_EN_FIRST && addr <= MODBUS_INPUT_HI_SPD_EN_LAST)
	 {
			sendbuf[0] = 0;
			sendbuf[1] = high_spd_en[addr - MODBUS_INPUT_HI_SPD_EN_FIRST];
	 }
	 else if(addr >= MODBUS_INPUT_TYPE_FIRST && addr <= MODBUS_INPUT_TYPE_LAST)
	 {
			sendbuf[0] = 0;
			sendbuf[1] = input_type[StartAdd + loop - MODBUS_INPUT_TYPE_FIRST];
	 }*/
// end input
// start variable
	 else if((addr >= MODBUS_VAR_FIRST && addr <= MODBUS_VAR_LAST)/* ||
		(addr >= MODBUS_RV_FIRST && addr <= MODBUS_RV_LAST)*/)
	 {
		U8_T index;
		U16_T base;
		 if(addr >= MODBUS_VAR_FIRST && addr <= MODBUS_VAR_LAST)
			 base = MODBUS_VAR_FIRST;
		 //else
		//	 base = MODBUS_RV_FIRST;

		index = (addr - base) / 2;
		if((addr - base) % 2 == 0)   // high word
		{
			if(vars[index].digital_analog == 0)  // digital
			{
				sendbuf[0] = 0;
				sendbuf[1] = 0;
			}
			else
			{
				sendbuf[0] = (U8_T)(vars[index].value >> 24);
				sendbuf[1] = (U8_T)(vars[index].value >> 16);
			}
		}
		else   // low word
		{
			if(vars[index].digital_analog == 0)  // digital
			{
				if((vars[index].range >= ON_OFF  && vars[index].range <= HIGH_LOW)
					||(vars[index].range >= custom_digital1 // customer digital unit
					&& vars[index].range <= custom_digital8
					&& digi_units[vars[index].range - custom_digital1].direct == 1))
				{  // inverse logic
					if(vars[index].control == 1)
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
					if(vars[index].control == 1)
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
				sendbuf[0] = (U8_T)(vars[index].value >> 8);
				sendbuf[1] = (U8_T)(vars[index].value);
			}
		}
	}
	else if(addr >= MODBUS_VAR_AM_FIRST && addr <= MODBUS_VAR_AM_LAST)
	{
		sendbuf[0] = 0;
		sendbuf[1] = vars[addr - MODBUS_VAR_AM_FIRST].auto_manual;
	}
	else if(addr >= MODBUS_VAR_RANGE_FIRST && addr <= MODBUS_VAR_RANGE_LAST)
	{
		sendbuf[0] = 0;
		sendbuf[1] = vars[addr - MODBUS_VAR_RANGE_FIRST].range;
	}
	else if(addr >= MODBUS_VAR_AD_FIRST && addr <= MODBUS_VAR_AD_LAST)
	{
		sendbuf[0] = 0;
		sendbuf[1] = vars[addr - MODBUS_VAR_AD_FIRST].digital_analog;
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
	if(StartAdd  >= MODBUS_OUTPUT_FIRST && StartAdd  <= MODBUS_OUTPUT_LAST)
	{
		int32_t tempval;
		i = (StartAdd - MODBUS_OUTPUT_FIRST) / 2;
		tempval = outputs[i].value;
		if((StartAdd - MODBUS_OUTPUT_FIRST) % 2 == 0)  // high word
		{
			tempval &= 0x0000ffff;
			tempval += 65536L * (pData[5] +  256 * pData[4]);
		}
		else  // low word
		{
			S32_T old_value;

			tempval &= 0xffff0000;
			tempval += (pData[5] + 256 * pData[4]);

			if(outputs[i].digital_analog == 0)  // digital
			{
			//	if(i < get_max_internal_output())
				{
					if(( outputs[i].range >= ON_OFF && outputs[i].range <= HIGH_LOW )
						||(outputs[i].range >= custom_digital1 // customer digital unit
						&& outputs[i].range <= custom_digital8
						&& digi_units[outputs[i].range - custom_digital1].direct == 1))
					{ // inverse
						if(output_priority[i][9] == 0xff)
							output_priority[i][7] = (pData[5]+ (U16_T)(pData[4] << 8)) ? 0 : 1;
						else
							output_priority[i][10] = (pData[5]+ (U16_T)(pData[4] << 8)) ? 0 : 1;
						outputs[i].control = Binary_Output_Present_Value(i) ? 0 : 1;
					}
					else
					{
						if(output_priority[i][9] == 0xff)
							output_priority[i][7] = (pData[5]+ (U16_T)(pData[4] << 8)) ? 1 : 0;
						else
							output_priority[i][10] = (pData[5]+ (U16_T)(pData[4] << 8)) ? 1 : 0;
						outputs[i].control = Binary_Output_Present_Value(i) ? 1 : 0;
					}
				}
#if 0
				else
				{
					old_value = outputs[i].control;

					if(( outputs[i].range >= ON_OFF && outputs[i].range <= HIGH_LOW )
						||(outputs[i].range >= custom_digital1 // customer digital unit
						&& outputs[i].range <= custom_digital8
						&& digi_units[outputs[i].range - custom_digital1].direct == 1))
					{// inverse
						outputs[i].control = (pData[5]+ (U16_T)(pData[4] << 8)) ? 0 : 1;
					}
					else
					{
						outputs[i].control =  (pData[5]+ (U16_T)(pData[4] << 8)) ? 1 : 0;
					}
				}
#endif
				if(outputs[i].control)
					;//set_output_raw(i,1000);
				else
					;//set_output_raw(i,0);
#if  T3_MAP
			if(i >= get_max_internal_output())
			{
					if(old_value != outputs[i].control)
					{
						if(i < base_out)
						{
					vTaskSuspend(Handle_Scan);	// dont not read expansion io
#if (ARM_MINI || ASIX_MINI)
					vTaskSuspend(xHandler_Output);  // do not control local io
#endif
					push_expansion_out_stack(&outputs[i],i,0);
#if (ARM_MINI || ASIX_MINI)
						// resume output task
					vTaskResume(xHandler_Output);
#endif
					vTaskResume(Handle_Scan);
						}
					}
			}
#endif
				outputs[i].value = Binary_Output_Present_Value(i) * 1000;

			}
			else if(outputs[i].digital_analog)
			{
				//if(i < get_max_internal_output())
				{
					if(output_priority[i][9] == 0xff)
						output_priority[i][7] = (float)(pData[5]+ (U16_T)(pData[4] << 8)) / 1000;
					else
						output_priority[i][10] = (float)(pData[5]+ (U16_T)(pData[4] << 8)) / 1000;
				}
					// if external io
#if  T3_MAP
					if(i >= get_max_internal_output())
					{
						old_value = outputs[i].value;

						output_raw[i] = (float)(pData[HeadLen + 5]+ (U16_T)(pData[HeadLen + 4] << 8));
						if(old_value != (float)(pData[HeadLen + 5]+ (U16_T)(pData[HeadLen + 4] << 8)))
						{
							if(i < base_out)
							{
							vTaskSuspend(Handle_Scan);	// dont not read expansion io
#if (ARM_MINI || ASIX_MINI)
							vTaskSuspend(xHandler_Output);  // do not control local io
#endif
							push_expansion_out_stack(&outputs[i],i,0);
#if (ARM_MINI || ASIX_MINI)
								// resume output task
							vTaskResume(xHandler_Output);
#endif
							vTaskResume(Handle_Scan);
							}
						}
					}

#endif
//						outputs[i].value = Analog_Output_Present_Value(i) * 1000;
//						Set_AO_raw(i,swap_double(outputs[i].value) * 1000);
					outputs[i].value = Analog_Output_Present_Value(i) * 1000;
				// set output_raw
					//Set_AO_raw(i,(float)outputs[i].value);  tbd:

			}
		}

//			check_output_priority_array(i);
#if OUTPUT_DEATMASTER
		clear_dead_master();
#endif
#if  T3_MAP
		if(i >= get_max_internal_output())
		{
			if( outputs[i].range >= OFF_ON && outputs[i].range <= LOW_HIGH )
			{
				if(pData[HeadLen + 5]+ (U16_T)(pData[HeadLen + 4] << 8) == 1)
						outputs[i].control = 1;
					else
						outputs[i].control = 0;
			}
			outputs[i].value = swap_double(tempval);
			push_expansion_out_stack(&outputs[i],i,0);
		}
#endif
		ChangeFlash = 1;
	}
	else if(StartAdd  >= MODBUS_OUTPUT_RANGE_FIRST && StartAdd  <= MODBUS_OUTPUT_RANGE_LAST)
	{
		i = (StartAdd - MODBUS_OUTPUT_RANGE_FIRST);
		outputs[i].range = pData[5];
		ChangeFlash = 1;

		push_expansion_out_stack(&outputs[i],i,1);
	}
	else if(StartAdd  >= MODBUS_OUTPUT_AM_FIRST && StartAdd  <= MODBUS_OUTPUT_AM_LAST)
	{
		i = (StartAdd - MODBUS_OUTPUT_AM_FIRST);
		if(outputs[i].switch_status == SW_AUTO)
			outputs[i].auto_manual = pData[5];
		ChangeFlash = 1;

		push_expansion_out_stack(&outputs[i],i,1);
	}
	else if(StartAdd  >= MODBUS_OUTPUT_AD_FIRST && StartAdd  <= MODBUS_OUTPUT_AD_LAST)
	{
		i = (StartAdd - MODBUS_OUTPUT_AD_FIRST);
		outputs[i].digital_analog = pData[5];
		ChangeFlash = 1;
		push_expansion_out_stack(&outputs[i],i,1);
	}
	else if(StartAdd  >= MODBUS_INPUT_FIRST && StartAdd  <= MODBUS_INPUT_LAST)
	{
		int32_t tempval;
		i = (StartAdd - MODBUS_INPUT_FIRST) / 2;
		tempval = inputs[i].value;
		if((StartAdd - MODBUS_INPUT_FIRST) % 2 == 0)  // high word
		{
			tempval &= 0x0000ffff;
			tempval += 65536L * (pData[5] +  256 * pData[4]);
		}
		else  // low word
		{
			tempval &= 0xffff0000;
			tempval += (pData[5] + 256 * pData[4]);

			if(inputs[i].digital_analog == 0)  // digital
			{
				if(( inputs[i].range >= ON_OFF && inputs[i].range <= HIGH_LOW )
				||(inputs[i].range >= custom_digital1 // customer digital unit
				&& inputs[i].range <= custom_digital8
				&& digi_units[inputs[i].range - custom_digital1].direct == 1))
				{ // inverse
					if(pData[5]+ (U16_T)(pData[4] << 8) == 1)
						inputs[i].control = 0;
					else
						inputs[i].control = 1;
				}
				else
				{
					if(pData[5]+ (U16_T)(pData[4] << 8) == 1)
						inputs[i].control = 1;
					else
						inputs[i].control = 0;
				}
			}
			else if(inputs[i].digital_analog == 1)
			{
				 inputs[i].value = 1000l * (pData[5]+ (U16_T)(pData[4] << 8));
			}

			if(inputs[i].auto_manual == 1)  // manual
			{
				/*if((inputs[i].range == HI_spd_count) || (inputs[i].range == N0_2_32counts)
					|| (inputs[i].range == RPM)
				)
				{
					if(swap_double(inputs[i].value) == 0)
					{
						high_spd_counter[i] = 0; // clear high spd count
						clear_high_spd[i] = 1;

					}
				}*/
			}
		}

		inputs[i].value = tempval;
		ChangeFlash = 1;

		push_expansion_in_stack(&inputs[i]);
	}
	else if(StartAdd  >= MODBUS_INPUT_FILTER_FIRST && StartAdd  <= MODBUS_INPUT_FILTER_LAST)
	{
		i = (StartAdd - MODBUS_INPUT_FILTER_FIRST);
		inputs[i].filter = pData[5];
		ChangeFlash = 1;

		push_expansion_in_stack(&inputs[i]);
	}
	else if(StartAdd  >= MODBUS_INPUT_CAL_FIRST && StartAdd  <= MODBUS_INPUT_CAL_LAST)
	{
		i = (StartAdd - MODBUS_INPUT_CAL_FIRST);
		inputs[i].calibration_hi = pData[4];
		inputs[i].calibration_lo = pData[5];
		ChangeFlash = 1;
		push_expansion_in_stack(&inputs[i]);
	}
	else if(StartAdd  >= MODBUS_INPUT_CAL_SIGN_FIRST && StartAdd  <= MODBUS_INPUT_CAL_SIGN_LAST)
	{
		i = (StartAdd - MODBUS_INPUT_CAL_SIGN_FIRST);
		inputs[i].calibration_sign = pData[5];
		ChangeFlash = 1;
		push_expansion_in_stack(&inputs[i]);
	}
	else if(StartAdd  >= MODBUS_INPUT_RANGE_FIRST && StartAdd  <= MODBUS_INPUT_RANGE_LAST)
	{
		i = (StartAdd - MODBUS_INPUT_RANGE_FIRST);
		inputs[i].digital_analog = pData[4];
		inputs[i].range = pData[5];
		ChangeFlash = 1;
		push_expansion_in_stack(&inputs[i]);
	}
	else if(StartAdd  >= MODBUS_INPUT_AM_FIRST && StartAdd  <= MODBUS_INPUT_AM_LAST)
	{
		i = (StartAdd - MODBUS_INPUT_AM_FIRST);
		inputs[i].auto_manual = pData[5];
		ChangeFlash = 1;
		push_expansion_in_stack(&inputs[i]);
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
	else if(StartAdd  >= MODBUS_VAR_FIRST && StartAdd  <= MODBUS_VAR_LAST)
	{
		int32_t tempval;
		i = (StartAdd - MODBUS_VAR_FIRST) / 2;
		tempval = vars[i].value;
		if((StartAdd - MODBUS_VAR_FIRST) % 2 == 0)  // high word
		{
			tempval &= 0x0000ffff;
			tempval += 65536L * (pData[5] +  256 * pData[4]);
		}
		else  // low word
		{
			tempval &= 0xffff0000;
			tempval += (pData[5] + 256 * pData[4]);

			if(vars[i].digital_analog == 0)  // digital
			{
				if(( vars[i].range >= ON_OFF && vars[i].range <= HIGH_LOW )
					||(vars[i].range >= custom_digital1 // customer digital unit
					&& vars[i].range <= custom_digital8
					&& digi_units[vars[i].range - custom_digital1].direct == 1))
				{// inverse
					if(pData[5]+ (U16_T)(pData[4] << 8) == 1)
						vars[i].control = 0;
					else
						vars[i].control = 1;
				}
				else
				{
					if(pData[5]+ (U16_T)(pData[4] << 8) == 1)
						vars[i].control = 1;
					else
						vars[i].control = 0;
				}
			}
		}

		vars[i].value = tempval;
		ChangeFlash = 1;

	}
	else if(StartAdd  >= MODBUS_VAR_AM_FIRST && StartAdd  <= MODBUS_VAR_AM_LAST)
	{
		i = (StartAdd - MODBUS_VAR_AM_FIRST);
		vars[i].auto_manual = pData[5];
		ChangeFlash = 1;
	}
	else if(StartAdd  >= MODBUS_VAR_AD_FIRST && StartAdd  <= MODBUS_VAR_AD_LAST)
	{
		i = (StartAdd - MODBUS_VAR_AD_FIRST);
		vars[i].digital_analog = pData[5];
		ChangeFlash = 1;
	}
	else if(StartAdd  >= MODBUS_VAR_RANGE_FIRST && StartAdd  <= MODBUS_VAR_RANGE_LAST)
	{
		i = (StartAdd - MODBUS_VAR_RANGE_FIRST);
		vars[i].range = pData[5];
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
	if(StartAdd >= MODBUS_OUTPUT_FIRST && StartAdd <= MODBUS_OUTPUT_LAST)
	{
		if(pData[5] == 0x02)
		{
			int32_t tempval;
			i = (StartAdd - MODBUS_OUTPUT_FIRST) / 2;
			tempval = pData[10] + (U16_T)(pData[9] << 8) \
				+ ((U32_T)pData[8] << 16) + ((U32_T)pData[7] << 24);

			outputs[i].value = tempval;

			if(outputs[i].digital_analog == 0)  // digital
			{
				if(( outputs[i].range >= ON_OFF && outputs[i].range <= HIGH_LOW )
					||(outputs[i].range >= custom_digital1 // customer digital unit
					&& outputs[i].range <= custom_digital8
					&& digi_units[outputs[i].range - custom_digital1].direct == 1))
				{// inverse
					if(tempval == 1)
						outputs[i].control = 0;
					else
						outputs[i].control = 1;
				}
				else
				{
					if(tempval == 1)
						outputs[i].control = 1;
					else
						outputs[i].control = 0;
				}

				/*if(outputs[i].control)  tbd:
					set_output_raw(i,1000);
				else
					set_output_raw(i,0);*/
			}
			else
			{
				// set output_raw
					//Set_AO_raw(i,(float)outputs[i].value);  tbd:
			}
		}
		ChangeFlash = 1;
	}
	else if(StartAdd >= MODBUS_INPUT_FIRST && StartAdd <= MODBUS_INPUT_LAST)
	{
		if(pData[5] == 0x02)
		{
			int32_t tempval;
			i = (StartAdd - MODBUS_INPUT_FIRST) / 2;
			tempval = pData[10] + (U16_T)(pData[9] << 8) \
				+ ((U32_T)pData[8] << 16) + ((U32_T)pData[7] << 24);

			if(inputs[i].digital_analog == 0)  // digital
			{
				if(( inputs[i].range >= ON_OFF && vars[i].range <= HIGH_LOW )
					||(inputs[i].range >= custom_digital1 // customer digital unit
					&& inputs[i].range <= custom_digital8
					&& digi_units[inputs[i].range - custom_digital1].direct == 1))
				{// inverse
					if(tempval == 1)
						inputs[i].control = 0;
					else
						inputs[i].control = 1;
				}
				else
				{
					if(tempval == 1)
						inputs[i].control = 1;
					else
						inputs[i].control = 0;
				}
			}

			inputs[i].value = tempval;
		}
		ChangeFlash = 1;
	}
	else if(StartAdd >= MODBUS_VAR_FIRST && StartAdd <= MODBUS_VAR_LAST)
	{
		if(pData[5] == 0x02)
		{
			int32_t tempval;
			i = (StartAdd - MODBUS_VAR_FIRST) / 2;
			tempval = pData[10] + (U16_T)(pData[9] << 8) \
				+ ((U32_T)pData[8] << 16) + ((U32_T)pData[7] << 24);


			if(vars[i].digital_analog == 0)  // digital
			{
				if(( vars[i].range >= ON_OFF && vars[i].range <= HIGH_LOW )
					||(vars[i].range >= custom_digital1 // customer digital unit
					&& vars[i].range <= custom_digital8
					&& digi_units[vars[i].range - custom_digital1].direct == 1))
				{// inverse
					if(tempval == 1)
						vars[i].control = 0;
					else
						vars[i].control = 1;
				}
				else
				{
					if(tempval == 1)
						vars[i].control = 1;
					else
						vars[i].control = 0;
				}
				//vars[i].value = vars[i].control ? 1000 : 0;
			}

			vars[i].value = tempval;
		}
		ChangeFlash = 1;

	}
}
void set_default_parameters(void);


void dealwith_write_setting(Str_Setting_Info * ptr)
{
	Test[13]++;
// compare sn to check whether it is current panel
	if(ptr->reg.sn == (Modbus.serialNum[0] + (U16_T)(Modbus.serialNum[1] << 8)	+ ((U32_T)Modbus.serialNum[2] << 16) + ((U32_T)Modbus.serialNum[3] << 24)))
	{Test[14]++;
		if(memcmp(panelname,ptr->reg.panel_name,20))
		{
			ptr->reg.panel_name[19] = 0;
			memcpy(panelname,ptr->reg.panel_name,20);
			Set_Object_Name(panelname);
		}
#if 0
		if(Modbus.en_time_sync_with_pc != ptr->reg.en_time_sync_with_pc)
		{
			flag_Update_Sntp = 0;
			Update_Sntp_Retry = 0;
			count_sntp = 0;
			// start SYNC with PC
			Setting_Info.reg.update_time_sync_pc = 1;
			Modbus.en_time_sync_with_pc = ptr->reg.en_time_sync_with_pc;
			E2prom_Write_Byte(EEP_EN_TIME_SYNC_PC,Modbus.en_time_sync_with_pc);
		}

		if(timezone != swap_word(ptr->reg.time_zone))
		{
			Sync_timestamp(swap_word(ptr->reg.time_zone),timezone,0,0);
			timezone = swap_word(ptr->reg.time_zone);
			E2prom_Write_Byte(EEP_TIME_ZONE_HI,(U8_T)(timezone >> 8));
			E2prom_Write_Byte(EEP_TIME_ZONE_LO,(U8_T)timezone);

		}
		if(Daylight_Saving_Time!= ptr->reg.Daylight_Saving_Time)
		{
			Sync_timestamp(0,0,ptr->reg.Daylight_Saving_Time,Daylight_Saving_Time);
			Daylight_Saving_Time = ptr->reg.Daylight_Saving_Time;
//			update_timers();

			E2prom_Write_Byte(EEP_DAYLIGHT_SAVING_TIME,(U8_T)Daylight_Saving_Time);

		}

		if((Modbus.en_sntp != ptr->reg.en_sntp) || ((Modbus.en_sntp == 5) && memcmp(sntp_server,Setting_Info.reg.sntp_server,30)))
		{
			Modbus.en_sntp = ptr->reg.en_sntp;

			if(Modbus.en_sntp <= 5)
			{
				E2prom_Write_Byte(EEP_EN_SNTP,Modbus.en_sntp);
				if(Modbus.en_sntp >= 2)
				{
						if(Modbus.en_sntp == 5)  // defined by customer
						{
							memcpy(sntp_server,Setting_Info.reg.sntp_server,30);
#if (ASIX_MINI || ASIX_CM5)
							DNSC_Start(sntp_server);
#endif

#if (ARM_MINI || ARM_CM5)
							resolv_query(sntp_server);
#endif
						}
						sntp_select_time_server(Modbus.en_sntp);
						flag_Update_Sntp = 0;
						Update_Sntp_Retry = 0;
						count_sntp = 0;

						SNTPC_Start(timezone, (((U32_T)SntpServer[0]) << 24) | ((U32_T)SntpServer[1] << 16) | ((U32_T)SntpServer[2] << 8) | (SntpServer[3]));

				}
			}
		}
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
#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI)
				dlmstp_init(NULL);
#endif
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
#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI)
			dlmstp_init(NULL);
#endif
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

		if((Instance != my_htonl(ptr->reg.instance)) && (ptr->reg.instance != 0))
		{
			Test[15]++;
			//memcpy(&Test[16],&Instance,4);
			//memcpy(&Test[18],&ptr->reg.instance,4);

			Instance = my_htonl(ptr->reg.instance);
			Device_Set_Object_Instance_Number(Instance);
			Store_Instance_To_Eeprom(Instance);


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
			uart_init(0);
			Count_com_config();
		}
		if(Modbus.baudrate[2] != ptr->reg.com_baudrate[2]) // com_baudrate[2]??T3000
		{
			Modbus.baudrate[2] = ptr->reg.com_baudrate[2];
			/*if((Modbus.com_config[2] == MODBUS_SLAVE) || (Modbus.com_config[2] == NOUSE) || (Modbus.com_config[2] == MODBUS_MASTER)
				|| (Modbus.com_config[2] == BACNET_SLAVE) || (Modbus.com_config[2] == BACNET_MASTER))*/
			save_uint8_to_flash(FLASH_BAUD_RATE2, Modbus.baudrate[2]);
			uart_init(2);
			Count_com_config();
		}

		if(ptr->reg.reset_default == 88)	// reset default
		{
			//flag_reset_default = 1;
			//ptr->reg.reset_default = 0;
			set_default_parameters();
		}
		if(ptr->reg.reset_default == 77)	 // show identify
		{
			ptr->reg.reset_default = 0;
			// show identify
			// control led on/off
			gIdentify = 1;
			count_gIdentify = 0;
			Test[0]++;
		}

#if 0
		if(ptr->reg.reset_default == 111)	 // reboot
		{
#if ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI
		    QuickSoftReset();
#else
				 flag_reboot = 1;//SoftReset();
#endif
			ptr->reg.reset_default = 0;
		}
		if(ptr->reg.reset_default == 150)	 // clear db
		{
			ptr->reg.reset_default = 0;
			clear_scan_db();
		}
#endif


#if !(ARM_TSTAT_WIFI )
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
#if (ASIX_MINI || ASIX_CM5)
				E2prom_Write_Byte(EEP_IP + 3, Modbus.ip_addr[0]);
				E2prom_Write_Byte(EEP_IP + 2, Modbus.ip_addr[1]);
				E2prom_Write_Byte(EEP_IP + 1, Modbus.ip_addr[2]);
				E2prom_Write_Byte(EEP_IP + 0, Modbus.ip_addr[3]);
#endif
#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI )
				E2prom_Write_Byte(EEP_IP + 0, Modbus.ip_addr[0]);
				E2prom_Write_Byte(EEP_IP + 1, Modbus.ip_addr[1]);
				E2prom_Write_Byte(EEP_IP + 2, Modbus.ip_addr[2]);
				E2prom_Write_Byte(EEP_IP + 3, Modbus.ip_addr[3]);
#endif
				//flag_reboot = 1;


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
#if (ASIX_MINI || ASIX_CM5)
				E2prom_Write_Byte(EEP_SUBNET + 3, Modbus.subnet[0]);
				E2prom_Write_Byte(EEP_SUBNET + 2, Modbus.subnet[1]);
				E2prom_Write_Byte(EEP_SUBNET + 1, Modbus.subnet[2]);
				E2prom_Write_Byte(EEP_SUBNET + 0, Modbus.subnet[3]);
#endif

#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI )
				E2prom_Write_Byte(EEP_SUBNET + 0, Modbus.subnet[0]);
				E2prom_Write_Byte(EEP_SUBNET + 1, Modbus.subnet[1]);
				E2prom_Write_Byte(EEP_SUBNET + 2, Modbus.subnet[2]);
				E2prom_Write_Byte(EEP_SUBNET + 3, Modbus.subnet[3]);
#endif

				//flag_reboot = 1;
			}
			if(memcmp(Modbus.getway,ptr->reg.getway,4) && !((ptr->reg.getway[0] == 0) && (ptr->reg.getway[1] == 0) \
				&& (ptr->reg.getway[2] == 0) && (ptr->reg.getway[3] == 0)))
			{

				memcpy(Modbus.getway,ptr->reg.getway,4);
#if (ASIX_MINI || ASIX_CM5)
				E2prom_Write_Byte(EEP_GETWAY + 3, Modbus.getway[0]);
				E2prom_Write_Byte(EEP_GETWAY + 2, Modbus.getway[1]);
				E2prom_Write_Byte(EEP_GETWAY + 1, Modbus.getway[2]);
				E2prom_Write_Byte(EEP_GETWAY + 0, Modbus.getway[3]);
#endif

#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI )
				E2prom_Write_Byte(EEP_GETWAY + 0, Modbus.getway[0]);
				E2prom_Write_Byte(EEP_GETWAY + 1, Modbus.getway[1]);
				E2prom_Write_Byte(EEP_GETWAY + 2, Modbus.getway[2]);
				E2prom_Write_Byte(EEP_GETWAY + 3, Modbus.getway[3]);
#endif
				//flag_reboot = 1;
			}
			if(Modbus.tcp_port != (ptr->reg.tcp_port) && (Modbus.tcp_port != 0))
			{
				Modbus.tcp_port = (ptr->reg.tcp_port);
				//E2prom_Write_Byte(EEP_PORT_LOW,Modbus.tcp_port);
				//E2prom_Write_Byte(EEP_PORT_HIGH,Modbus.tcp_port >> 8);
				//flag_reboot = 1;
			}
			if(Modbus.tcp_type != ptr->reg.tcp_type)
			{
				if(ptr->reg.tcp_type <= 1)
				{
				Modbus.tcp_type = ptr->reg.tcp_type;
				//E2prom_Write_Byte(EEP_TCP_TYPE,  Modbus.tcp_type );
				}
				//flag_reboot = 1;
			}

#if (ASIX_MINI || ASIX_CM5)
			flag_reboot = 1;
#endif

#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI )
			IP_Change = 1;
#endif
		}
#endif


#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI )
		if(MAX_MASTER != ptr->reg.MAX_MASTER)
		{
			MAX_MASTER = ptr->reg.MAX_MASTER;
			E2prom_Write_Byte(EEP_MAX_MASTER, ptr->reg.MAX_MASTER);
		}
#endif

#if ARM_TSTAT_WIFI
		if(memcmp(Modbus.display_lcd.lcddisplay,ptr->reg.display_lcd.lcddisplay,sizeof(lcdconfig)))
		{
			memcpy(Modbus.display_lcd.lcddisplay,ptr->reg.display_lcd.lcddisplay,sizeof(lcdconfig));
			// clear first screen
			disp_str(FORM15X30, 6,  32, "     ",SCH_COLOR,TSTAT8_BACK_COLOR);
			disp_ch(0,FIRST_CH_POS,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
			disp_ch(0,SECOND_CH_POS,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
			disp_ch(0,THIRD_CH_POS - 16,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
			disp_ch(0,THIRD_CH_POS - 16 + 48,THERM_METER_POS,' ',TSTAT8_CH_COLOR,TSTAT8_BACK_COLOR);
			// save it to flash memory
			write_page_en[25] = 1;
			Flash_Write_Mass();
		}
#endif

#if ARM_MINI
			if(Modbus.start_month != ptr->reg.start_month)
			{
				Modbus.start_month = ptr->reg.start_month;
				E2prom_Write_Byte(EEP_DLS_START_MON, ptr->reg.start_month);
				Calculate_DSL_Time();
			}
			if(Modbus.start_day != ptr->reg.start_day)
			{
				Modbus.start_day = ptr->reg.start_day;
				E2prom_Write_Byte(EEP_DLS_START_DAY, ptr->reg.start_day);
				Calculate_DSL_Time();
			}
			if(Modbus.end_month != ptr->reg.end_month)
			{
				Modbus.end_month = ptr->reg.end_month;
				E2prom_Write_Byte(EEP_DLS_END_MON, ptr->reg.end_month);
				Calculate_DSL_Time();
			}
			if(Modbus.end_day != ptr->reg.end_day)
			{
				Modbus.end_day = ptr->reg.end_day;
				E2prom_Write_Byte(EEP_DLS_END_DAY, ptr->reg.end_day);
				Calculate_DSL_Time();
			}
#endif
	 }
}

void responseCmd(uint8 type,uint8* pData)
{
	//uint16_t ptr16[10];
	//uint8_t ptr8[10];
	responseModbusCmd(BAC_TO_MODBUS, pData, 0,NULL,NULL,0);
}

