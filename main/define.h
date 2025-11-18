#ifndef	DEFINE_H
#define	DEFINE_H

#include "types.h"
#include "esp_attr.h"

#pragma pack(1)

#define SOFTREV     6602


#define		SW_OFF 	 0
#define 	SW_HAND	 2
#define		SW_AUTO	 1

typedef	struct
{
	u16 count[20];
	u16 old_count[20];
	u8  enable[20];
	u16 inactive_count[20];
}STR_Task_Test;
extern STR_Task_Test task_test;



#define UIP_HEAD 6
// must change library if change it
typedef struct
{
	U8_T serialNum[4];
	U8_T address; 	
	U8_T protocal;
	U8_T product_model;
	U8_T hardRev;
	U8_T baudrate[3];
	//U8_T unit;
//	U8_T switch_tstat_val;
	U8_T IspVer;
	U8_T PicVer;
	U8_T update_status;
	U8_T  base_addr;
	U8_T  tcp_type;   /* 0 -- DHCP, 1-- STATIC */
	U8_T  ip_addr[4];
	U8_T  subnet[4];
	U8_T  getway[4];
	U8_T  mac_addr[6];
	U8_T  ethernet_status;
	U16_T tcp_port;
	U8_T  mini_type;
	U8_T  sub_port;
//	U8_T zigbee_or_gsm;
	U8_T point_sequence;
	U8_T main_port;
	U8_T external_nodes_plug_and_play;
	U8_T com_config[3];
	U16_T start_adc[11];
	U8_T refresh_flash_timer;

	U16_T network_number;
	U8_T  en_username;
	U8_T  cus_unit;

	U8_T usb_mode;
	U8_T en_dyndns;
	U8_T en_sntp;	
	
	U16_T Bip_port;
	U16_T vcc_adc; // 
	U8_T network_master;
	
	
	U8_T fix_com_config;
//	U8_T backlight;
	U8_T LCD_time_off_delay;
	U8_T en_time_sync_with_pc;
	
	U8_T uart_parity[3];
	U8_T uart_stopbit[3];
//	U8_T network_ID[3]; // 3 RS485 port
	U16_T zigbee_module_id;
	U8_T dead_master_for_PLC;;
	U8_T disable_tstat10_display;  // display icons and scrolling string
	//lcdconfig display_lcd;
	U8_T start_month;
	U8_T start_day;
	U8_T end_month;
	U8_T end_day;

	U8_T led_rx485_tx;
	U8_T led_rx485_rx;

	U8_T enable_debug;
	U16_T mstp_network;

	U8_T icon_config;
	U8_T mstp_master;
	U16_T write_flash;


}STR_MODBUS;

typedef struct
{
	U16_T cmd;   // low byte first
	U16_T len;   // low byte first
	U16_T own_sn[4]; // low byte first
	U16_T product;   // low byte first
	U16_T address;   // low byte first
	U16_T ipaddr[4]; // low byte first
	U16_T modbus_port; // low byte first
	U16_T firmwarerev; // low byte first
	U16_T hardwarerev;  // 28 29	// low byte first

	U8_T master_sn[4];  // master's SN 30 31 32 33
	U16_T instance_low; // 34 35 hight byte first
	U8_T panel_number; //  36
	U8_T panelname[20]; // 37 - 56
	U16_T instance_hi; // 57 58 hight byte first

	U8_T bootloader;  // 0 - app, 1 - bootloader, 2 - wrong bootloader , 3 - mstp device
	U16_T BAC_port;  //  hight byte first
	U8_T zigbee_exist; // BIT0: 0 - inexsit, 1 - exist
										 // BIT1: 0 - NO WIFI, 1 - WIFI

	U8_T subnet_protocal; // 0 - modbus, 12 - bip to mstp

	U8_T  command_version; //65 version number
	U8_T  subnet_port;  //1- MainPort      2-ZigbeePort      3-SubPort
	U8_T  subnet_baudrate;   //
	 U8_T mini_type;
}STR_SCAN_CMD;


typedef	enum
{
	NOUSE = 0,
	BACNET_SLAVE,
	MODBUS_SLAVE,
	PTP_RS232_GSM,
	SUB_GSM,
	MAIN_ZIG,
	SUB_ZIG,
	MODBUS_MASTER,
	RS232_METER,
	BACNET_MASTER,
	MAX_COM_TYPE
}E_UART_TYPE;


#define SERIAL  0
#define TCP		1
#define USB		2
#define GSM		3
#define BAC_TO_MODBUS 4
#define WIFI  	5

/*#define MINI_CM5  0
#define MINI_BIG	 1
#define MINI_SMALL  2
#define MINI_TINY	 3			// ASIX CORE
#define MINI_NEW_TINY	 4  // ARM CORE*/
#define MINI_BIG_ARM	 	5
#define MINI_SMALL_ARM  	6
#define MINI_TINY_ARM		7
#define MINI_NANO   		8
#define MINI_TSTAT10 		9
#define MINI_T10P	 		11
#define MINI_VAV	 		10   // no used
#define MINI_TINY_11I		12
#define	PROJECT_FAN_MODULE 	13
#define	PROJECT_POWER_METER 14
#define	PROJECT_AIRLAB   	15
#define PROJECT_TRANSDUCER 	16
#define PROJECT_TSTAT9		17 		// 基本上不用了
#define PROJECT_SAUTER		18
#define PROJECT_RMC1216		19  	// old NG2 = RMC1216
#define PROJECT_MPPT		20
#define PROJECT_LSW_BTN		21
#define PROJECT_NG2_NEW		22
#define PROJECT_MULTIMETER	23
#define PROJECT_LIGHT_PWM	24
#define PROJECT_MULTIMETER_NEW	25
#define PROJECT_CO2 		26
#define PROJECT_LSW_SENSOR	27

#define MAX_MINI_TYPE 		28

uint16 READ_POINT_TIMER;
uint16 READ_POINT_TIMER_FROM_EEP;
extern uint8 uart0_config;
extern STR_MODBUS Modbus;
extern U16_T Test[50];
//extern uint8_t modbus_wifi_buf[500];
//extern uint16_t modbus_wifi_len;
extern uint8 reg_num;

extern uint8 led_sub_tx;
extern uint8 led_sub_rx;
extern uint8 led_main_tx;
extern uint8 led_main_rx;

extern uint8 flagLED_ether_tx;
extern uint8 flagLED_ether_rx;
extern uint8 flagLED_sub_rx;
extern uint8 flagLED_sub_tx;
extern uint8 flagLED_main_rx;
extern uint8 flagLED_main_tx;

extern uint8 com_config_back[3];
extern uint8 flag_change_uart0;
extern uint8 flag_change_uart2;
extern uint8 count_change_uart0;
extern uint8 count_change_uart2;

void modbus_task0(void *arg);
void modbus_task2(void *arg);
void start_fw_update(void);
void esp_retboot(void);
void internalDeal(uint8  *bufadd,uint8 type);


#if 1// for UDP DEBUG
extern char udp_debug_str[100];
#define DEBUG_PORT 33333
extern uint8 flag_debug_rx;
extern uint16 debug_rx_len;
#endif

extern uint32 multicast_addr;
uint32 Get_multicast_addr(uint8 *ip_addr);
void delay_ms(unsigned int t);

#endif
