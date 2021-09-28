#ifndef __MODBUS_H
#define	__MODBUS_H

#include <string.h>
#include "ud_str.h"
//#include "crc.h"


#define TXEN		PAout(8)
#define SEND			1			//1
#define	RECEIVE		0
//#define	READ_VARIABLES				0x03
//#define	WRITE_VARIABLES				0x06
#define	MULTIPLE_WRITE_VARIABLES				0x10
#define	CHECKONLINE					0x19
#define DATABUFLEN					200
#define DATABUFLEN_SCAN				12
#define SENDPOOLLEN         		8
#define SERIAL_COM_IDLE				0
#define INVALID_PACKET				1
#define VALID_PACKET				2
#define USART_REC_LEN  			512
#define USART_SEND_LEN			512



#define READ_VARIABLES      3
#define WRITE_VARIABLES     6
#define MULTIPLE_WRITE		16
#define CHECKONLINE				0x19


// *******************modbus.h***********************************
// Header file containing all of the register information for modbus
// serial communications.
// V.24 first release of modbus.h file.
// V.25
//caution:the tstat response should have a pause between two characrers,but the maximum allowed pause is 1.5 character times or .83 ms * 1.5 or 1.245 ms at 9600 baud.
//  REGISTER ADDRESSES TO BE USED IN MODBUS SERIAL COMMUNICATION


enum {
	SERIALNUMBER_LOWORD =0   ,          // -	-	Lower 2 bytes of the serial number
	SERIALNUMBER_HIWORD  = 2  ,		// -	-	Upper 2 bytes of teh serial number
	VERSION_NUMBER_LO   = 4  ,		// -	-	Software version
	VERSION_NUMBER_HI,				// -	-	Software version
	MODBUS_ADDRESS,							// 1	254	Device modbus address
	PRODUCT_MODEL,					// -	-	Temco Product Model	1=Tstat5B, 2=Tstat5A, 4=Tstat5C, 12=Tstat5D,
	HARDWARE_REV,					// -	-	Hardware Revision
	PIC882VERSION,						// -	-	PIC firmware version
	PLUG_N_PLAY,				// -	-	Temporary address for plug-n-play addressing scheme
	ISP_MODE_INDICATION,
	BASE_ADDRESS 	= 14,			// base address select
	BAUDRATE,//15						// 0	1	Baudrate 0 = 9.6kb/s, 1 = 19.2kb/s
	UPDATE_STATUS	= 16,			// reg 16 status for update_flash											// writing 0x7F means jump to ISP routine											// writing 0x8F means completely erase eeprom

	HARDWARE_INFORMATION,     // 20 indicate if clock is present
	MODBUS_BACNET_SWITCH,



	MODBUS_ETHERNET_STATUS	= 22,

	MODBUS_INSTANCE_HI = 32,
	MODBUS_MINI_TYPE = 34,
	MODBUS_INSTANCE_LO = 35,  //
	MODBUS_PANEL_NUMBER = 36,
	MODBUS_STATION_NUM = 42,  // MSTP ID

	MAC_ADDR_1 = 60,
	MAC_ADDR_2,
	MAC_ADDR_3,
	MAC_ADDR_4,
	MAC_ADDR_5,
	MAC_ADDR_6, //65
	IP_MODE, //66
	IP_ADDR_1,
	IP_ADDR_2,
	IP_ADDR_3,
	IP_ADDR_4,
	IP_SUB_MASK_1,//71
	IP_SUB_MASK_2,
	IP_SUB_MASK_3,
	IP_SUB_MASK_4,
	IP_GATE_WAY_1,
	IP_GATE_WAY_2,
	IP_GATE_WAY_3,
	IP_GATE_WAY_4,
	WIFI_FAC,//79
	WIFI_RSSI, //80



/******** WIFI START ************************/
	MODBUS_WIFI_START = 2000,
	MODBUS_WIFI_SSID_MANUAL_EN = 2000,  // 2001 ~ 2009 reserved
	MODBUS_WIFI_MODE, // 0-AUTO 1-STATIC
	MODBUS_WIFI_STATUS,
	MODBUS_WIFI_RESTORE,
	MODBUS_WIFI_MODBUS_PORT,
	MODBUS_WIFI_BACNET_PORT,
	MODBUS_WIFI_REV, // current is 2
	MODBUS_WIIF_START_SMART,
	MODBUS_WIFI_WRITE_MAC,

	MODBUS_WIFI_SSID_START = 2010,	// 2010 ~ 2041 user name 64 bytes
	MODBUS_WIFI_SSID_END = 2041,
	MODBUS_WIFI_PASS_START = 2042,	// 2042 ~ 2057 password 32 bytes
	MODBUS_WIFI_PASS_END = 2057,

	MODBUS_WIFI_IP1 = 2058 ,
	MODBUS_WIFI_NETMASK = 2062,
	MODBUS_WIFI_GETWAY = 2066,
	MDOBUS_WIFI_MACADDR = 2070,

	MODBUS_WIFI_END = 2100,
/******** WIFI END ************************/


    MODBUS_TEST_1 = 7000,
	MODBUS_TEST_50 = 7049,
/*********FOR CALIBRATE HUM SENSOR******************/
	MODBUS_SETTING_BLOCK_FIRST = 9800,
	MODBUS_USER_BLOCK_FIRST = MODBUS_SETTING_BLOCK_FIRST,

	MODBUS_SETTING_BLOCK_LAST = 9999,
	MODBUS_OUTPUT_BLOCK_FIRST = 10000,     // 10000 -  11471  45
//	MODBUS_OUTPUT_BLOCK_LAST = MODBUS_OUTPUT_BLOCK_FIRST + MAX_OUTS * ((sizeof(Str_out_point) + 1)/ 2) - 1,

//	MODBUS_INPUT_BLOCK_FIRST, // 11472 - 12943   46
//	MODBUS_INPUT_BLOCK_LAST = MODBUS_INPUT_BLOCK_FIRST + MAX_INS * ((sizeof(Str_in_point) + 1) / 2) - 1,

	MODBUS_INPUT_BLOCK_FIRST =11472,
	MODBUS_INPUT_BLOCK_LAST = MODBUS_INPUT_BLOCK_FIRST + MAX_INS * 23/* ((sizeof(Str_in_point) + 1) / 2) - 1*/,

//	MODBUS_VAR_BLOCK_FIRST,  // 12944 - 15503  39
//	MODBUS_VAR_BLOCK_LAST = MODBUS_VAR_BLOCK_FIRST + MAX_VARS * ((sizeof(Str_variable_point) + 1) / 2) - 1,

	MODBUS_EX_MOUDLE_EN = 65000,
	MODBUS_EX_MOUDLE_FLAG12 = 65001,
	MODBUS_EX_MOUDLE_FLAG34 = 65002,
};
//extern uint8_t isWifiExist;
//extern uint8_t serial_no_activity;
//extern u8 USART_RX_BUFA[USART_REC_LEN];     //
//extern u8 USART_RX_BUFB[USART_REC_LEN];


//extern uint8_t dealwithTag ;

//void serial_restart(void);

//void modbus_data_cope(u8 XDATA* pData, u16 length, u8 conn_id) ;
extern void modbus_init(void) ;
extern void stm32_uart_init(void);
//uint8_t checkCrc(void);


#endif
void responseModbusCmd(uint8_t type, uint8_t *pData, uint16_t len,uint8_t *resData ,uint16_t *modbus_send_len);
