#ifndef	DEFINE_H
#define	DEFINE_H

#include "types.h"
#pragma pack(1)



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
	U8_T  	subnet[4];
	U8_T  	getway[4];
	U8_T  mac_addr[6];
	U8_T  ethernet_status;
	U16_T 	tcp_port;
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

	U8_T  usb_mode;
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
	U8_T dead_master;
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

}STR_MODBUS;


#endif
