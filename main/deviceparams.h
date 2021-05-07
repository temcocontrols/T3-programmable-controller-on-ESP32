#ifndef _DEVICE_PARAMS
#define _DEVICE_PARAMS

#include "stdio.h"

//#define A24_ARR_SIZE 24
#define SERIAL  0
#define TCP		1
#define USB		2
#define GSM		3
#define BAC_TO_MODBUS 4
#define WIFI  	5

#define UIP_HEAD 6

#define READ_VARIABLES      3
#define WRITE_VARIABLES     6
#define MULTIPLE_WRITE		16
#define CHECKONLINE			0x19
#define CHECKONLINE_WIHTCOM	 0x18

#define READ_COIL  			0X01
#define READ_DIS_INPUT 		0X02
#define READ_INPUT      	0x04
#define WRITE_COIL 			0X05
#define WRITE_MULTI_COIL 	0x0f

#define READ_REMOTE_INPUT		0x09

/* takes a byte out of a uint32 : var - uint32,  ByteNum - byte to take out (0 - 3) */
#define BREAK_UINT32( var, ByteNum ) \
          (uint8_t)((uint32_t)(((var) >>((ByteNum) * 8)) & 0x00FF))

#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((uint32_t)((uint32_t)((Byte0) & 0x00FF) \
          + ((uint32_t)((Byte1) & 0x00FF) << 8) \
          + ((uint32_t)((Byte2) & 0x00FF) << 16) \
          + ((uint32_t)((Byte3) & 0x00FF) << 24)))

#define BUILD_UINT16(loByte, hiByte) \
          ((uint16_t)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)

#define BUILD_UINT8(hiByte, loByte) \
          ((uint8_t)(((loByte) & 0x0F) + (((hiByte) & 0x0F) << 4)))

#define HI_UINT8(a) (((a) >> 4) & 0x0F)
#define LO_UINT8(a) ((a) & 0x0F)

typedef enum{
	PROJECT_SAUTER,
	PROJECT_FAN_MODULE,
}project_e;

#pragma pack(push, 1)
typedef struct
{
	unsigned short int cmd;   // low byte first
	unsigned short int len;   // low byte first
	unsigned short int own_sn[4]; // low byte first
	unsigned short int product;   // low byte first
	unsigned short int address;   // low byte first
	unsigned short int ipaddr[4]; // low byte first
	unsigned short int modbus_port; // low byte first
	unsigned short int firmwarerev; // low byte first
	unsigned short int hardwarerev;  // 28 29	// low byte first

	unsigned char master_sn[4];  // master's SN 30 31 32 33
	unsigned short int instance_low; // 34 35 hight byte first
	unsigned char panel_number; //  36
	char panelname[20]; // 37 - 56
	unsigned short int instance_hi; // 57 58 hight byte first

	unsigned char bootloader;  // 0 - app, 1 - bootloader, 2 - wrong bootloader , 3 - mstp device
	unsigned short int BAC_port;  //  hight byte first
	unsigned char zigbee_exist; // 0 - inexsit, 1 - exist

	unsigned char subnet_protocal; // 0 - modbus, 12 - bip to mstp

}STR_SCAN_CMD;
#pragma pack(pop)

//#pragma pack(push, 1)
typedef struct
{
	uint16_t trigger;
	uint16_t timer;
	uint8_t alarmOn;
	uint16_t count_down;
}trigger_t;
//#pragma pack(pop)
// This file defines structure of modbus parameters which reflect correspond modbus address space
// for each modbus register type (coils, discreet inputs, holding registers, input registers)
#pragma pack(push, 1)
typedef struct
{
    // Parameter: discrete_input0
    uint8_t discrete_input0:1;
    // Parameter: discrete_input1
    uint8_t discrete_input1:1;
    // Parameter: discrete_input2
    uint8_t discrete_input2:1;
    // Parameter: discrete_input3
    uint8_t discrete_input3:1;
    // Parameter: discrete_input4
    uint8_t discrete_input4:1;
    // Parameter: discrete_input5
    uint8_t discrete_input5:1;
    // Parameter: discrete_input6
    uint8_t discrete_input6:1;
    // Parameter: discrete_input7
    uint8_t discrete_input7:1;
    uint8_t discrete_input_port1:8;
} discrete_reg_params_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    // Parameter: Coil 0 : Coil0
    uint8_t coil0:1;
    // Parameter: Coil 1 : Coil1
    uint8_t coil1:1;
    // Parameter: Coil 2 : Coil2
    uint8_t coil2:1;
    // Parameter: Coil 3 : Coil3
    uint8_t coil3:1;
    // Parameter: Coil 4 : Coil4
    uint8_t coil4:1;
    // Parameter: Coil 5 : Coil5
    uint8_t coil5:1;
    // Parameter: Coil 6 : Coil6
    uint8_t coil6:1;
    // Parameter: Coil 7 : Coil7
    uint8_t coil7:1;
    // Coils port 1
    uint8_t coil_port1:8;
} coil_reg_params_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    // Parameter: Data channel 0 : data_chan0 : NV Address: 0
    float data_chan0;
    // Parameter: Data channel 1 : data_chan1 : NV Address: 0
    float data_chan1;
    // Parameter: Data channel 2 : data_chan2 : NV Address: 0
    float data_chan2;
    // Parameter: Data channel 3 : data_chan3 : NV Address: 0
    float data_chan3;
} input_reg_params_t;
#pragma pack(pop)

//See register map for more information.
#pragma pack(push, 1)
typedef struct
{
    // Parameter: Data channel 0 : DataChan0
    uint16_t serial_number_lo;
    // Parameter: Data channel 1 : DataChan1
    uint16_t serial_number_hi;
    // Parameter: Data channel 2 : DataChan2
    uint16_t version_number_lo;
    // Parameter: Data channel 3 : DataChan3
    uint16_t version_number_hi;
    // Parameter: Modbus Network Address : modbus_address
    uint8_t modbus_address;    //6
    // Parameter: Protocol version  : protocol_version
    uint8_t product_model;
    // Parameter: Hardware version  : hardware_version
    uint8_t hardware_version;   //8
    // Parameter: Software Version : software_version
    uint16_t isp_mode_indication;
    // Parameter: Modbus Baudrate : modbus_baud
    uint8_t baud_rate;
    uint16_t update_status;
    uint16_t ethernet_status;
    uint8_t modbus_bacnet_switch;

    uint16_t testBuf[20];

    uint8_t mac_addr[6];
    uint16_t ip_mode;
    uint8_t ip_addr[4];
    uint8_t ip_net_mask[4];
    uint8_t ip_gateway[4];

    uint8_t which_project;
    uint16_t fan_module_pwm1;
    uint16_t fan_module_pwm2;
    uint16_t led_rx485_tx;
    uint16_t led_rx485_rx;
    uint16_t fan_module_pulse;
    uint16_t register80;
    uint16_t register81;
    uint16_t register82;
    uint16_t register83;
    uint16_t register84;
    uint16_t register85;
    uint16_t register86;
    uint16_t register87;
    uint16_t register88;
    uint16_t register89;
    uint16_t register90;
    uint16_t register91;
    uint16_t register92;
    uint16_t register93;
    uint16_t register94;
    uint16_t register95;
    uint16_t register96;
    uint16_t register97;
    uint16_t register98;
    uint16_t register99;
    uint16_t register100;

    uint16_t coolheatmode;
    uint16_t pid1_mode_operation;


    // Parameter: Modbus parity  : modbus_parity
    //uint16_t modbus_parity;
    // Parameter: Modbus stopbit  : modbus_stop_bits
    //uint16_t modbus_stop_bits;
    // Parameter: Brace control  : modbus_brace_ctrl
    uint16_t modbus_brace_ctrl;
    // Parameter: Up time  : up_time
    uint32_t up_time;
    // Parameter: Device state  : device_state
    uint16_t device_state;
    // readyToUpdate set to 1, OTA will download bin file and update
    uint16_t readyToUpdate;
    // temperature
    uint16_t sht31temperature;
    // humidity
    uint16_t sht31humidity;

} holding_reg_params_t;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)

typedef struct
{
	int8_t description[21]; 	      /* (21 uint8_ts; string)*/
	int8_t label[9];		      	/* (9 uint8_ts; string)*/

	int32_t value;		     						/* (4 uint8_ts; int32_t)*/

	uint8_t  filter;  /* */
	int8_t decom;/* (1 bit; 0=ok, 1=point decommissioned)*/  // high 4 bits input type, low 4 bits for old decom status
	uint8_t sub_id;//int8_t sen_on;/* (1 bit)*/
	uint8_t sub_product;//int8_t sen_off;  /* (1 bit)*/
	int8_t control; /* (1 bit; 0=OFF, 1=ON)*/
	int8_t auto_manual; /* (1 bit; 0=auto, 1=manual)*/
	int8_t digital_analog ; /* (1 bit; 1=analog, 0=digital)*/
	int8_t calibration_sign; /* (1 bit; sign 0=positiv 1=negative )*/
	uint8_t sub_number;//int8_t calibration_increment; /* (1 bit;  0=0.1, 1=1.0)*/
	uint8_t calibration_hi; /* (5 bits - spare )*/
	uint8_t calibration_lo;  /* (8 bits; -25.6 to 25.6 / -256 to 256 )*/

	uint8_t range;	      /* (1 uint8_t ; input_range_equate)*/

} Str_in_point; /* 46 */
#pragma pack(pop)
#if 0
typedef struct
{
	int8_t description[21];	      /*  (21 uint8_ts; string)*/
	int8_t label[9];		      /*  (9 uint8_ts; string)*/

	int32_t value;		      /*  (4 uint8_ts; float)*/

	uint8_t auto_manual;  /*  (1 bit; 0=auto, 1=manual)*/
	uint8_t digital_analog;  /*  (1 bit; 1=analog, 0=digital)*/
	uint8_t control	;
	uint8_t unused	;  //low 4 bit for prog
	uint8_t range ; /*  (1 uint8_t ; variable_range_equate)*/


}	Str_variable_point; /*  39*/


typedef struct
{
	uint8_t harware_rev;
	uint16_t firmware_asix;	// ASIX
	uint8_t frimware_pic;    // PIC
	uint8_t firmware_rev;	// C8051
	uint8_t hardware_rev;	// SM5964
	uint8_t bootloader_rev;

	uint8_t no_used[10];
}Str_Pro_Info;

typedef	union
{
	uint8_t all[400];
	struct
	{
	 uint8_t ip_addr[4];
	 uint8_t subnet[4];
	 uint8_t getway[4];
	 uint8_t mac_addr[6];

	 uint8_t tcp_type;   /* 1 -- DHCP, 0 -- STATIC */

	 uint8_t mini_type;
	 uint8_t debug;

	 Str_Pro_Info pro_info;
	 uint8_t com_config[3];
	 uint8_t refresh_flash_timer;
	 uint8_t en_plug_n_play;

	 uint8_t reset_default;	  // write 88
	 uint8_t com_baudrate[3];

		uint8_t  en_username;  // 2-enalbe  1 - disable 0: unused
	 uint8_t  cus_unit;

	 uint8_t usb_mode;
	 uint8_t network_number;
	 uint8_t panel_type;

	 S8_T panel_name[20];
	 uint8_t en_panel_name;
	 uint8_t panel_number;

	uint8_t dyndns_user[MAX_USERNAME_SIZE];
	uint8_t dyndns_pass[MAX_PASSWORD_SIZE];
	uint8_t dyndns_domain[MAX_DOMAIN_SIZE];
	uint8_t en_dyndns;  			// 0 - no  1 - disable 2 - enable
	uint8_t dyndns_provider;  // 0- www.3322.org 1-www.dyndns.com  2 - www.no-ip.com 3 - temco server
	uint16_t dyndns_update_time;  // xx min
	uint8_t en_sntp;		// 0 - no  1 - disable 2 - timeserver1, 3 - timeserver2 , 4 - timeserver3 5 - customer timersverver  - 200 local PC
	S16_T time_zone;
	uint32_t sn;

	UN_Time update_dyndns;
	uint16_t mstp_network_number;
	uint8_t BBMD_EN;
	uint8_t sd_exist;

	uint16_t tcp_port;
	uint8_t modbus_id;
	uint32_t instance;

	uint32_t update_sntp_last_time;
	uint8_t Daylight_Saving_Time;

	S8_T sntp_server[30];
	U8_T zigbee_exist;  // 0x74 - exist

	U8_T backlight;
	U8_T update_time_sync_pc;   //  0 - finished  1 - ask updating
	U8_T en_time_sync_with_pc;  // 0 - time server 1 - pc
	U8_T sync_with_ntp_result; // 0 - fail , 1 - ok

//	U8_T network_ID[3];
	U8_T MSTP_ID;
	U16_T zigbee_module_id;
	U8_T MAX_MASTER;
	U8_T specila_flag;
	// bit0 -> support PT1K, 0 - NO PT1K , 1- PT1K
	// bit1 -> support PT100, 0 - NO PT100 , 1- PT100

	U8_T uart_parity[3];  // 2- Even  1 - Odd  0 - none
	U8_T uart_stopbit[3];
//	USART_StopBits_1        0
// 	USART_StopBits_0_5      1
// 	USART_StopBits_2        2
// 	USART_StopBits_1_5  		3

	}reg;
}Str_Setting_Info;
#endif

extern uint8_t CRClo;
extern uint8_t CRChi;
extern holding_reg_params_t holding_reg_params;
extern input_reg_params_t input_reg_params;
extern coil_reg_params_t coil_reg_params;
extern discrete_reg_params_t discrete_reg_params;

extern trigger_t light_trigger;
extern trigger_t sound_trigger;
extern trigger_t co2_trigger;
extern trigger_t occ_trigger;

extern void modbus_init(void);
extern void modbus_task(void *arg);

extern void init_crc16(void);
extern void crc16_byte(uint8_t ch);
extern uint16_t crc16(uint8_t *p, uint8_t length);
extern void responseCmd(uint8_t type, uint8_t *pData, uint16_t len);
extern uint16_t Filter(uint8_t channel,uint16_t input);

#endif // !defined(_DEVICE_PARAMS)
