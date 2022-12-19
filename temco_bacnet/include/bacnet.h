#ifndef __BACNET_H__
#define __BACNET_H__

#include <string.h>
#include "types.h"
#include "stdint.h"
#include "esp_attr.h"

#define BAC_MSTP 0
#define BAC_IP 1
#define BAC_IP_CLIENT 2  // for network points
//#define BAC_GSM 2
#define BAC_BVLC 3
#define BAC_IP_CLIENT2 4  // for COV


// select current chip
#define ASIX				0
#define ARM					1

/* Enable the Gateway (Routing) functionality here, if desired. */
#if !defined(MAX_NUM_DEVICES)
#ifdef BAC_ROUTING
#define MAX_NUM_DEVICES 3       /* Eg, Gateway + two remote devices */
#else
#define MAX_NUM_DEVICES 1       /* Just the one normal BACnet Device Object */
#endif
#endif


/* Define your processor architecture as
   Big Endian (PowerPC,68K,Sparc) or Little Endian (Intel,AVR)
   ARM and MIPS can be either - what is your setup? */
#if !defined(BIG_ENDIAN)
#define BIG_ENDIAN 1
#endif

/* Define your Vendor Identifier assigned by ASHRAE */
//#if !defined(BACNET_VENDOR_ID)
//#define BACNET_VENDOR_ID 148
//#endif
//#if !defined(BACNET_VENDOR_NAME)
//#define BACNET_VENDOR_NAME "Temco controls"
//#endif

//#define BACNET_PRODUCT_NAME "Netix Product"//"Temoc Product"
/* Max number of bytes in an APDU. */
/* Typical sizes are 50, 128, 206, 480, 1024, and 1476 octets */
/* This is used in constructing messages and to tell others our limits */
/* 50 is the minimum; adjust to your memory and physical layer constraints */
/* Lon=206, MS/TP=480, ARCNET=480, Ethernet=1476, BACnet/IP=1476 */
//#if !defined(MAX_APDU)
//    /* #define MAX_APDU 50 */
//    /* #define MAX_APDU 1476 */


//#define MAX_APDU 480//600//1476
///* #define MAX_APDU 128 enable this IP for testing readrange so you get the More Follows flag set */
//#elif defined (BACDL_ETHERNET)
//#define MAX_APDU 1476
//#else
//#define MAX_APDU 480
//#endif

#define MAX_APDU 600
	   

/* for confirmed messages, this is the number of transactions */
/* that we hold in a queue waiting for timeout. */
/* Configure to zero if you don't want any confirmed messages */
/* Configure from 1..255 for number of outstanding confirmed */
/* requests available. */
#if ASIX
#define MAX_TSM_TRANSACTIONS  20
#else
#define MAX_TSM_TRANSACTIONS  20//255
#endif
//#if !defined(MAX_TSM_TRANSACTIONS)
//#define MAX_TSM_TRANSACTIONS  255 //????????????? changed by chelsea
//#endif
/* The address cache is used for binding to BACnet devices */
/* The number of entries corresponds to the number of */
/* devices that might respond to an I-Am on the network. */
/* If your device is a simple server and does not need to bind, */
/* then you don't need to use this. */
#if !defined(MAX_ADDRESS_CACHE)
#define MAX_ADDRESS_CACHE 10//255
#endif

/* some modules have debugging enabled using PRINT_ENABLED */
#if !defined(PRINT_ENABLED)
#define PRINT_ENABLED 0
#endif

/* BACAPP decodes WriteProperty service requests
   Choose the datatypes that your application supports */


#define BACAPP_NULL
#define BACAPP_BOOLEAN
#define BACAPP_UNSIGNED
#define BACAPP_SIGNED
#define BACAPP_REAL
#define BACAPP_DOUBLE
#define BACAPP_OCTET_STRING
#define BACAPP_CHARACTER_STRING
#define BACAPP_BIT_STRING
#define BACAPP_ENUMERATED
#define BACAPP_DATE
#define BACAPP_TIME
#define BACAPP_OBJECT_ID


#define MAX_BITSTRING_BYTES (15)


#ifndef MAX_CHARACTER_STRING_BYTES
#define MAX_CHARACTER_STRING_BYTES (MAX_APDU-6)
#endif

#ifndef MAX_OCTET_STRING_BYTES
#define MAX_OCTET_STRING_BYTES (MAX_APDU-6)
#endif


#define BACNET_SVC_I_HAVE_A    1
#define BACNET_SVC_WP_A        1
#define BACNET_SVC_RP_A        1
#define BACNET_SVC_RPM_A       1
#define BACNET_SVC_DCC_A       1
#define BACNET_SVC_RD_A        1
#define BACNET_SVC_TS_A        1
#define BACNET_SVC_SERVER      0
#define BACNET_USE_OCTETSTRING 1
#define BACNET_USE_DOUBLE      1
#define BACNET_USE_SIGNED      1


#include "dlmstp.h"
#include "datalink.h"
#include "device.h"
#include "handlers.h"
#include "whois.h"
#include "address.h"
#include "client.h"




#ifndef MAX_AVS
#define MAX_AVS 128
#endif
#ifndef MAX_BVS
#define MAX_BVS  128
#endif
#ifndef MAX_AIS
#define MAX_AIS  64
#endif
#ifndef MAX_AOS
#define MAX_AOS  64
#endif
#ifndef MAX_BIS
#define MAX_BIS  64
#endif
#ifndef MAX_BOS
#define MAX_BOS  64
#endif
#ifndef MAX_SCHEDULES
#define MAX_SCHEDULES 8
#endif
#ifndef MAX_CALENDARS
#define MAX_CALENDARS 4
#endif
#ifndef MAX_TREND_LOGS
#define MAX_TREND_LOGS 8
#endif

#ifndef MAX_TEMCOVARS
#define MAX_TEMCOVARS  10
#endif


#pragma pack(1)

// for ASIX
void switch_to_modbus(void);   // receive modbus frame when current protocal is mstp,switch to modbus
//void UART_Init(U8_T port);

#define OBJECT_BASE 1

#define BAC_COMMON  1



uint8_t RS485_Get_Baudrate();
extern volatile uint16_t SilenceTime;





extern U8_T flag_send_get_panel_number;
//#include "bitmap.h"

#define T3_CON
//#define T3_IO
//#define T8
//#define CO2


#ifdef T3_CON
#define BAC_CALENDAR 		1
#define BAC_SCHEDULE 		1
#define BAC_PRIVATE 		1
#define BAC_TIMESYNC 		1
#define BAC_TRENDLOG		1
#define BAC_RANGE			1 // dont need range 
#define BAC_BI				1
#define BAC_BV				1
#define BAC_DCC				0
#define BAC_PROPRIETARY 1

#define BAC_MSV					1


#define BAC_MASTER 			1
#define EXTERNAL_IO			1

#define COV							1
#define BAC_FILE				0//1



#define BIP   // TSTAT dont have it
 

/*#define BACNET_VENDOR_NETIX 	"NETIX Controls"
#define BACNET_VENDOR_JET			"JetControls"
#define BACNET_VENDOR_TEMCO	 	"TemcoControls"
#define BACNET_VENDOR_NEWRON	"Newron Solutions"		

#define BACNET_PRODUCT_NETIX 	"NCCB"
#define BACNET_PRODUCT_JET		"Jet Product"
#define BACNET_PRODUCT_TEMCO	"Temco Product"
#define BACNET_PRODUCT_NEWRON	"NewroNode"		
		
#define BACNET_VENDOR_ID_NETIX 1007
#define BACNET_VENDOR_ID_JET 997
#define BACNET_VENDOR_ID_TEMCO 148
#define BACNET_VENDOR_ID_NEWRON	1206*/

#define BACNET_PRODUCT_TEMCO	"Temco Product"
#define BACNET_VENDOR_TEMCO	 	"TemcoControls"

#define BACDL_ALL
#define BACDL_MSTP

#ifdef BIP
#include "bip.h"
//#include "tcpip.h" 
#define BBMD_ENABLED 1
#define BACDL_BIP
#endif



void uart1_init(U32_T bound);


#define far  
#define xdata 

int Get_Number_by_Bacnet_Index(U8_T type,U8_T index);
int Get_Bacnet_Index_by_Number(U8_T type,U8_T number);

//#define TXEN		PAout(8)

//#define TXEN0_MINI PCout(2)
//#define TXEN2_MINI PEout(11)

extern U16_T far Test[50];
//extern u8 uart_send[512] ;
//extern uint16_t send_count;

extern uint16_t uip_len;

extern bool Send_bip_Flag2;

extern U8_T far Send_bip_address2[6];
extern U8_T far Send_bip_count2;
extern U8_T count_send_bip2;


U8_T 	UART_Get_SendCount(void);

#endif

void Set_TXEN(U8_T dir);
char get_current_mstp_port(void);
void uart_send_string(U8_T *p, U16_T length,U8_T port);
void Timer_Silence_Reset( void);
void uart_send_mstp(U8_T *p, U16_T length);



extern uint8_t TransmitPacket[MAX_PDU];
extern uint8_t TransmitPacket_panel;
extern uint16_t TransmitPacketLen;
extern uint8_t Send_Private_Flag;
extern uint8_t MSTP_Transfer_OK;
char* get_label(uint8_t type,uint8_t num);
char* get_description(uint8_t type,uint8_t num);
char* Get_temcovars_string_from_buf(uint8_t num);
char Write_temcovars_string_to_buf(uint8_t num,char * str);
char get_range(uint8_t type,uint8_t num);
char /*get_AM_Status*/Get_Out_Of_Service(uint8_t type,uint8_t num);
float Get_bacnet_value_from_buf(uint8_t type,uint8_t priority,uint8_t i);
char* Get_Object_Name(void);

void Set_Vendor_ID(uint16_t vendor_id);
void Set_Vendor_Name(char* name);
void Set_Vendor_Product(char* product);
void Set_Object_Name(char* name);


void write_Output_Relinguish(uint8_t type,uint8_t i,float value);
float Get_Output_Relinguish(uint8_t type,uint8_t i);

bool Analog_Input_Change_Of_Value(unsigned int instance);
bool Analog_Value_Change_Of_Value(unsigned int instance);
// for TIMESYNC
#if BAC_TIMESYNC
#include "timesync.h"
//#include "user_data.h"
#if 1

int add_Trend_Log(uint8_t type,uint8_t instance);


typedef	union
	{
		U8_T all[10];
		struct 
		{
			U8_T sec;				/* 0-59	*/
			U8_T min;    		/* 0-59	*/
			U8_T hour;      		/* 0-23	*/
			U8_T day;       		/* 1-31	*/
			U8_T week;  		/* 0-6, 0=Sunday	*/
			U8_T mon;     		/* 1-12	*/
			U8_T year;      		/* 0-99	*/
			U16_T day_of_year; 	/* 0-365	*/
			S8_T is_dst;        /* daylight saving time on / off */		
				
		}Clk;
		struct
    {
        U32_T timestamp;
        S8_T time_zone;
        U8_T daylight_saving_time;
        U8_T reserved[4];
    }NEW;
}UN_Time;
extern UN_Time Rtc;//时钟结构体 
#endif	
//U32_T Rtc_Set(U16_T syear, U8_T smon, U8_T sday, U8_T hour, U8_T min, U8_T sec, U8_T flag);
uint32_t Rtc_Set(uint16_t syear, uint8_t smon, uint8_t sday, uint8_t hour, uint8_t min, uint8_t sec, uint8_t flag);
extern  BACNET_DATE Local_Date;
extern  BACNET_TIME Local_Time;
void Set_Daylight_Saving_Status(bool);
bool Get_Daylight_Savings_Status();
BACNET_DATE * Get_Local_Date();
BACNET_TIME * Get_Local_Time();
S16_T Get_UTC_Offset(void);
void Set_UTC_OFFset(void);  //???????
bool write_Local_Date(BACNET_DATE* date);
bool write_Local_Time(BACNET_TIME* date);
#endif

#if BAC_SCHEDULE
#include "schedule.h"
#include "bactimevalue.h"
	
extern uint8_t  SCHEDULES;
extern U8_T far wr_time_on_off[8][9][8];

BACNET_TIME_VALUE Get_Time_Value(uint8_t object_index,uint8_t day,uint8_t i);
uint8_t Get_TV_count(uint8_t object_index,uint8_t day);
BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE * Get_Object_Property_References(uint8_t i);
void Clear_Time_Value(uint8_t index,uint8_t day);
#if ARM
void write_Time_Value(uint8_t index,uint8_t day,uint8_t i,uint8_t , uint8_t /*BACNET_TIME_VALUE time_value*/,uint8_t);
#else
void write_Time_Value(uint8_t index,uint8_t day,uint8_t i,BACNET_TIME_VALUE time_value,uint8_t);
#endif

U8_T Get_WR_ON_OFF(uint8_t index,uint8_t day,uint8_t i);
//void Set_WR_ON_OFF(uint8_t index,uint8_t day,uint8_t i,uint8_t value);
void Check_wr_time_on_off(uint8_t i,uint8_t j,uint8_t mode);


#endif

#if BAC_CALENDAR
#include "calendar.h"


extern uint8_t  CALENDARS;
uint8_t Get_CALENDAR_count(uint8_t object_index);
BACNET_DATE Get_Calendar_Date(uint8_t object_index,uint8_t i);
U32_T get_current_time(void);
//U32_T my_mktime(UN_Time* t);
void clear_calendar_data(uint8 index);
void write_annual_date(uint8_t index,BACNET_DATE date);
#endif


#if BAC_TRENDLOG
#include "trendlog.h"

extern uint8_t  TRENDLOGS;
U32_T my_mktime(UN_Time* t);
#endif


#if BAC_MSV
#include "msv.h"

extern uint8_t MSVS;

char * Get_State_Text(uint8_t i,uint8_t j);
void write_value_state(uint32_t object_instance,
    uint32_t state_index,
    char *new_name);
uint8 Get_State_Text_Len(uint8_t i);

#endif


void wirte_bacnet_value_to_buf(uint8_t type,uint8_t priority,uint8_t i,float unit);
void write_bacnet_unit_to_buf(uint8_t type,uint8_t priority,uint8_t i,uint8_t unit);
void write_bacnet_name_to_buf(uint8_t type,uint8_t priority,uint8_t i,char* str);
void write_bacnet_description_to_buf(uint8_t type,uint8_t priority,uint8_t i,char* str);
void write_Out_Of_Service(uint8_t type,uint8_t i,uint8_t am);
void Set_Object_Name(char * name);

void Send_whois_to_mstp(void);


U16_T Get_Vendor_ID(void);
//extern char* bacnet_vendor_name;
//extern char* bacnet_vendor_product;
const char*  Get_Vendor_Name(void);
const char * Get_Vendor_Product(void);

typedef enum
{
 AV,AI,AO,BI,BO,SCHEDULE,CALENDAR,TRENDLOG,TRENDLOG_MUL,BV,TEMCOAV,MSV,
}BACNET_type;

extern uint8_t MSTP_Transfer_Len;





//#include "user_data.h"
extern uint8_t far MSTP_Rec_buffer[600];
extern uint8_t MSTP_Write_OK;
extern uint8_t MSTP_Transfer_OK;
extern uint8_t remote_panel_num;
extern uint8_t count_hold_on_bip_to_mstp; // 当yabe或者T3000软件正在访问时，不要读写下面的设备

U8_T Get_current_panel(void);
void chech_mstp_collision(void);
void check_mstp_packet_error(void);
void check_mstp_timeout(void);

void Send_SUB_I_Am(uint8_t index);
void add_remote_panel_db(uint32_t device_id,BACNET_ADDRESS* src,uint8_t panel,uint8 * pdu,uint8_t pdu_len,uint8_t protocal,uint8_t temcoproduct);

void Transfer_Bip_To_Mstp_pdu( uint8_t * pdu,uint16_t pdu_len);
void Transfer_Mstp_To_Bip_pdu( uint8_t src, uint8_t * pdu,uint16_t pdu_len);

void push_bac_buf(U8_T *mtu,U8_T len);
uint16_t get_network_number(void);

void handler_conf_private_trans_ack(
    uint8_t * service_request,
    uint16_t service_len,
    uint8_t * apdu,
    int apdu_len,
			uint8_t protocal);

	void Handler_Complex_Ack(
    uint8_t * apdu,
    int apdu_len,      /* total length of the apdu */
		uint8_t protocal
    );
		
uint8_t Send_Mstp(uint8_t flag,uint8_t *type);		






extern uint8_t  AVS;
extern uint8_t  AIS;
extern uint8_t  AOS;
extern uint8_t  BIS;
extern uint8_t  BOS;
extern uint8_t  BVS;
extern uint8_t  TRENDLOGS;
extern uint8_t  TemcoVarS;

extern uint8_t AI_Index_To_Instance[MAX_AIS];
extern uint8_t BI_Index_To_Instance[MAX_AIS];
extern uint8_t AO_Index_To_Instance[MAX_AOS];
extern uint8_t BO_Index_To_Instance[MAX_AOS];
extern uint8_t AV_Index_To_Instance[MAX_AVS];
extern uint8_t BV_Index_To_Instance[MAX_AVS];


extern uint8_t AI_Instance_To_Index[MAX_AIS];
extern uint8_t BI_Instance_To_Index[MAX_AIS];
extern uint8_t AO_Instance_To_Index[MAX_AOS];
extern uint8_t BO_Instance_To_Index[MAX_AOS];
extern uint8_t AV_Instance_To_Index[MAX_AVS];
extern uint8_t BV_Instance_To_Index[MAX_AVS];


void Count_IN_Object_Number(void);
void Count_OUT_Object_Number(void);
void Count_VAR_Object_Number(void);





extern unsigned char far Temp_Buf[MAX_APDU];

#endif












