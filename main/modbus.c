#include <stdio.h>
#include "esp_err.h"

#include "mbcontroller.h"
#include "deviceparams.h"       // for device parameters structures
#include "esp_log.h"            // for log_write
#include "driver/gpio.h"
#include "modbus.h"
#include "i2c_task.h"
#include "store.h"
#include "wifi.h"

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
    holding_reg_params.version_number_lo = 27;
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
	uint8_t stm32_counter = 0;
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
	//uint8_t *stm32_uart_send = (uint8_t*)malloc(STM32_UART_SEND_SIZE);
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
				holding_reg_params.stm32_uart_send[0] = 1;
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
		/*else
		{
			holding_reg_params.stm32_uart_send[0] = 0;
			holding_reg_params.stm32_uart_send[1] = 0;
		}*/

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

			//holding_reg_params.stm32_uart_send[0] ++;
			//holding_reg_params.stm32_uart_send[1] = 0xaa;
			//holding_reg_params.stm32_uart_send[5] = 0x2c;
			//holding_reg_params.stm32_uart_send[15] = 0x3e;
			//holding_reg_params.stm32_uart_send[10] = 0x78;
			uart_write_bytes(UART_NUM_1, (const char *)holding_reg_params.stm32_uart_send, STM32_UART_SEND_SIZE);
			/*if((holding_reg_params.stm32_uart_send[STM32_LED_485_TX]==1)||(holding_reg_params.stm32_uart_send[STM32_LED_485_RX]==1))
			{
				stm32_counter = 0;
			}*/
			if(stm32_counter++>20)
			{
				//memset(holding_reg_params.stm32_uart_send, 0, STM32_UART_SEND_SIZE);
				if(SSID_Info.IP_Wifi_Status == WIFI_CONNECTED)
					holding_reg_params.stm32_uart_send[STM32_LED_WIFI_ON] = 1;
				else
					holding_reg_params.stm32_uart_send[STM32_LED_WIFI_ON] = 0;
				holding_reg_params.stm32_uart_send[STM32_LED_485_TX] = 0;
				holding_reg_params.stm32_uart_send[STM32_LED_485_RX] = 0;
				stm32_counter = 0;
			}
		}
		//uart_write_bytes(uart_num, (const char *)uart_rsv, len);
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}

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

