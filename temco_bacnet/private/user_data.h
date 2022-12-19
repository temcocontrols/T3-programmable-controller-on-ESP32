#ifndef USER_DATA_H
#define USER_DATA_H

#include "ud_str.h"
#include "monitor.h"
#include "esp_attr.h"
#include "bacnet.h"

#pragma pack(1)

typedef struct
{
	U8_T buf[600];
	U16_T len;
}STR_SEND_BUF;

typedef struct
{
	uint8 harware_rev;
	uint16 firmware_asix;	// ASIX
	uint8 frimware_pic;    // PIC
	uint8 firmware_rev;	// C8051
	uint8 hardware_rev;	// SM5964
	uint8 bootloader_rev;

	uint8 no_used[10];
}Str_Pro_Info;

#define READ_VARIABLES      3
#define WRITE_VARIABLES     6
#define MULTIPLE_WRITE		16
#define CHECKONLINE				0x19
#define CHECKONLINE_WIHTCOM	 0x18

#define TEMCO_MODBUS 		0xff

#define READ_COIL  			0X01
#define READ_DIS_INPUT 	0X02
#define READ_INPUT      0x04
#define WRITE_COIL 			0X05
#define WRITE_MULTI_COIL 0x0f 

#define READ_REMOTE_INPUT		0x09

//#define ID_SIZE 19
//typedef	union
//{
//	uint8 all[ID_SIZE];
//	struct
//	{
//		uint8 id;		
//		uint8 on_line; // 0: offline    1: online
//		uint8 schedule;	
//		uint8 flag;
//		char name[15];
///*  flag formate:
//  7   6  5  4  3   2   1  0
// a/m  output state1 -  -  -  -  -
// */
//	}Str;
//}UN_ID; /*config roution */


#define STORE_ID_LEN	15
typedef	union
{
 int8_t all[39];
 struct
 {
  uint8 id;
  uint8 schedule;
  uint8 flag;
	uint16_t reserved_reg[6];
	 
  uint8 on_line; // 0: offline    1: online
  char name[15];
  uint16_t daysetpoint;
  uint16_t nightsetpoint;
  uint16_t awaysetpoint;
  uint16_t sleepsetpoint;
  
  /*  flag formate:
  7   6  5  4  3   2   1  0
  a/m  output state1 -  -  -  -  -
  */
 }Str;
}UN_ID; /*config roution */

#if 0
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
        U8_T reserved[3];
    }NEW;
	}UN_Time;
#endif
	
typedef union
{
    uint8_t lcddisplay[7];
    struct
    {
        uint8_t display_type; // 0:?????????    1:modbus ??????????
        Point_Net npoint;     //?????input output var ???
    }lcd_mod_reg;

    struct
    {
        uint8_t display_type; // 0:?????????    2:Bacnet ?????????
        unsigned long obj_instance;
        uint8_t point_type;
        uint8_t point_number;
    }lcd_bac_reg;

}lcdconfig;
	
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
	 
	uint8_t dyndns_user[32]; // no used
	uint8_t dyndns_pass[32]; // no used
	uint8_t dyndns_domain[32]; // no used
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
	
	U8_T LCD_time_off_delay;//U8_T backlight;
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
	
	lcdconfig display_lcd;
	U8_T start_month;
	U8_T start_day;
	U8_T end_month;
	U8_T end_day;
	
	uint8_t network_number_hi;
	}reg;
}Str_Setting_Info;


typedef union
{
    uint8_t all[400];
    struct
    {
        unsigned char smtp_type;  //  0   ipaddress   // 1   domain
        unsigned char smtp_ip[4];
        char smtp_domain[40];
        unsigned short smtp_port;
        char email_address[60];
        char user_name[60];
        char password[20];
        char secure_connection_type;  //0 -NULL   1-SSL   2-TLS
    }reg;
}Str_Email_point;


typedef	union
{
	uint8_t all[400];
	struct 
	{ 
	 uint16_t flag; // 0x55ff
	 uint32_t monitor_block_num[24];
	 uint8_t operate_time[12][4];	
		
	 uint8_t flag1; // network health  0x55
	 uint32_t com_rx[3];
	 uint32_t com_tx[3];	
	 uint16_t collision[3];  // id collision
	 uint16_t packet_error[3];  // bautrate not match
	 uint16_t timeout[3];
	}reg;
}Str_MISC;

typedef union
{
		uint8_t all[30];
		struct
		{
			uint8_t product_id;
			uint8_t port;
			uint8_t modbus_id;
			uint32_t last_contact_time;
			uint8_t input_start;
			uint8_t input_end;
			uint8_t output_start;
			uint8_t output_end;
			uint32_t sn;
			S8_T reserved_reg[15];
		}reg;
}Str_Extio_point;



#define PIC_PACKET_STACK 50
#define PIC_PACKET_LEN  400
typedef struct
{
	U8_T flag;
	U8_T buf[PIC_PACKET_LEN];
	U8_T retry;
	U16_T index;
	U8_T file; // which picture
}STR_STORE_PIC;


typedef enum { WRITE_SD_OK = 0,WAIT_FOR_SD} E_OPERATE_SD;



#define USER_DATA_HEADER_LEN 7

#define MAX_RETRY_SEND_BIP 5

extern U32_T	timeCount1,timeCount2;
//extern uint8_t invokeid_bip;
extern uint8_t  invokeid_mstp;
extern uint8_t  flag_receive_netp;	// network points 
extern uint8_t  flag_receive_netp_temcovar;
//extern uint8_t  flag_receive_netp_temcoreg;
extern uint8_t  temcovar_panel;
//extern uint8_t  temcoreg[20];
extern uint8_t  temcovar_panel_invoke;
//extern uint8_t  temcoreg_panel_invoke;
extern uint8_t  flag_receive_netp_modbus;	// network points 
extern uint8_t  flag_receive_rmbp;  // remote bacnet points
extern U8_T Send_Whois_Flag;
extern U8_T Send_Time_Sync;
extern U8_T  count_send_bip;
//extern U8_T count_bip_connect;
//extern U8_T Send_bip_Flag;
//extern U8_T  Send_bip_address[6];
//extern U8_T  Send_bip_count;
//extern U8_T Send_bip_Flag2;
//extern U8_T  Send_bip_address2[6];
//extern U8_T  Send_bip_count2;
//extern U8_T flag_get_network_point;
extern U8_T remote_modbus_index;	
extern U8_T remote_bacnet_index;
extern U8_T  network_point_index;
extern U8_T remote_mstp_panel_index;

extern uint8_t Send_Private_Flag;
extern uint16_t count_Private;
extern U8_T Send_I_Am_Flag;
extern U8_T Send_Read_Property;
extern U8_T  MSTP_Send_buffer[600];
extern U16_T MSTP_buffer_len;
extern STR_MSTP_REV_HEADER  MSTP_Rec_Header;
extern U8_T  MSTP_Rec_buffer[600];
extern S8_T   just_load;

extern const uint8_t mon_table[12];
extern uint16_t 	start_day;
extern uint16_t		end_day;
void Calculate_DSL_Time(void);


extern FRAME_ENTRY 								 SendFrame[MAX_SEND_FRAMES];
//extern STR_PTP  Ptp_para;
//extern UNITDATA_PARAMETERS  		NL_PARAMETERS;
//extern Routing_Table                Routing_table[MAX_Routing_table];

extern U8_T flag_Updata_Clock;
extern U32_T  update_sntp_last_time;
extern uint8_t  BACnet_Port;
extern U32_T   	Instance;
extern U8_T  boot;
//extern U8_T  flag_E2_changed;
//extern U32_T  changed_index;
//extern U32_T  changed_index2;

extern EXT_RAM_ATTR Str_Remote_TstDB        Remote_tst_db;
extern EXT_RAM_ATTR Str_Panel_Info 	    		Panel_Info;
extern EXT_RAM_ATTR Str_Setting_Info     		Setting_Info;
extern EXT_RAM_ATTR Str_MISC  						 	MISC_Info;
extern EXT_RAM_ATTR Str_Special  Write_Special;

extern EXT_RAM_ATTR Str_in_point  inputs[MAX_INS];
extern EXT_RAM_ATTR Str_out_point  	outputs[MAX_OUTS];
//extern Str_out_point   	*outputs;
extern uint8_t				 				 no_outs;
//extern Str_in_point    	*inputs;
extern uint8_t				 				 no_ins;

extern Info_Table			 						 info[18];

extern U8_T  month_length[12];
extern U32_T 				timestart;	   /* seconds since the beginning of the year */
extern U32_T 				time_since_1970;   /* seconds since the beginning of 2010 */
extern In_aux					 						 in_aux[MAX_IO_POINTS];
extern Con_aux				 							 con_aux[MAX_CONS];
//extern Mon_aux                      mon_aux[MAX_MONITORS];
extern EXT_RAM_ATTR Monitor_Block		 			mon_block[2 * MAX_MONITORS];
//extern S8_T 				         mon_data_buf[sizeof(Monitor_Block) * 2 * MAX_MONITORS];
extern EXT_RAM_ATTR Mon_Data 			 		*Graphi_data;
extern S8_T 				 			Garphi_data_buf[sizeof(Mon_Data)];

extern EXT_RAM_ATTR S8_T   var_unit[MAX_VAR_UNIT][VAR_UNIT_SIZE];
extern EXT_RAM_ATTR Str_Extio_point  extio_points[MAX_EXTIO];

//extern S16_T                          MAX_MONITOR_BLOCKS;
//extern U8_T                         free_mon_blocks;

 
extern S8_T  panelname[20];
//extern U8_T 	client_ip[4];
//extern U8_T newsocket;

extern Alarm_point 		     				 alarms[MAX_ALARMS];
extern U8_T 			     							 ind_alarms;
extern Alarm_set_point 		    			 alarms_set[MAX_ALARMS_SET];
extern U8_T 			     							 ind_alarms_set;
extern U16_T                 alarm_id;
extern S8_T                         new_alarm_flag;

extern EXT_RAM_ATTR Units_element		     				 digi_units[MAX_DIG_UNIT];
extern U8_T 					 		ind_passwords;
extern EXT_RAM_ATTR Password_point			 			passwords[ MAX_PASSW ];

extern Str_Email_point Email_Setting;

extern EXT_RAM_ATTR Str_variable_point		 		 vars[MAX_VARS + 12];
extern EXT_RAM_ATTR Str_controller_point 	 			 controllers[MAX_CONS];
extern EXT_RAM_ATTR Str_totalizer_point           totalizers[MAX_TOTALIZERS];
extern EXT_RAM_ATTR Str_monitor_point		 				 monitors[MAX_MONITORS];   
extern EXT_RAM_ATTR Str_monitor_point		 			backup_monitors[MAX_MONITORS]/* _at_ 0x12800*/;	
//extern Aux_group_point        	 		 aux_groups[MAX_GRPS];
//extern S8_T                     		 Icon_names[MAX_ICONS][14];
extern EXT_RAM_ATTR Control_group_point  	 		 control_groups[MAX_GRPS];
extern EXT_RAM_ATTR Str_grp_element			 	    		 group_data[MAX_ELEMENTS];
extern S16_T 					 							 total_elements;
extern S16_T 					 							 group_data_length;

//extern U32_T 				 		SD_lenght[MAX_MONITORS * 2];
//extern U32_T 				 		SD_block_num[MAX_MONITORS * 2];

extern Str_mon_element          read_mon_point_buf[MAX_MON_POINT];
extern Str_mon_element          write_mon_point_buf[MAX_MONITORS * 2][MAX_MON_POINT];

extern EXT_RAM_ATTR Str_weekly_routine_point  		 weekly_routines[MAX_WR] ;
extern EXT_RAM_ATTR Wr_one_day				 		wr_times[MAX_WR][MAX_SCHEDULES_PER_WEEK];
extern EXT_RAM_ATTR Str_annual_routine_point	 	 annual_routines[MAX_AR];
extern EXT_RAM_ATTR U8_T                         ar_dates[MAX_AR][AR_DATES_SIZE];	
extern EXT_RAM_ATTR U8_T	  wr_time_on_off[MAX_WR][MAX_SCHEDULES_PER_WEEK][8];
 /* Assume bit0 from octet0 = Jan 1st */
extern EXT_RAM_ATTR Str_program_point	     			 programs[MAX_PRGS];
extern S8_T 			    	 			*program_address[MAX_PRGS]; /*pointer to code*/
extern EXT_RAM_ATTR U8_T    	    	 				prg_code[MAX_PRGS][MAX_CODE * CODE_ELEMENT];
extern U16_T			 	 			Code_len[MAX_PRGS];
extern U16_T 			 				Code_total_length;
//extern Str_array_point 	     			 arrays[MAX_ARRAYS];
extern S32_T  			    				*arrays_address[MAX_ARRAYS];
extern EXT_RAM_ATTR Str_table_point			 				 custom_tab[MAX_TBLS];
extern U16_T                         PRG_crc;
extern U8_T  *prog;
extern S32_T  stack[20];
extern S32_T  *index_stack;
extern U8_T  *time_buf;
extern U32_T  cond;
extern S32_T  v, value;
extern S32_T  op1,op2;
extern S32_T  n,*pn;

extern S8_T  message[ALARM_MESSAGE_SIZE+26+10];
extern U8_T alarm_flag;
extern S8_T alarm_at_all;
extern S8_T ind_alarm_panel;
extern S8_T alarm_panel[5];
extern U16_T alarm_index;
extern U32_T                       miliseclast_cur;
extern U32_T                       miliseclast;


extern POINTS_HEADER			      points_header[MAXREMOTEPOINTS];


extern EXT_RAM_ATTR NETWORK_POINTS          		 network_points_list_bacnet[MAXNETWORKPOINTS];	 /* points wanted by others */
extern Byte                  			 number_of_network_points_bacnet; 

extern EXT_RAM_ATTR NETWORK_POINTS          		 network_points_list_modbus[MAXNETWORKPOINTS];	 /* points wanted by others */
extern Byte                  			 number_of_network_points_modbus;
//extern U8_T  NT_bacnet_tb_func[MAXNETWORKPOINTS];
//extern STR_BAC_TB  NT_bacnet_tb[MAXNETWORKPOINTS];

extern EXT_RAM_ATTR REMOTE_POINTS		    		 remote_points_list_modbus[MAXREMOTEPOINTS];  /* points from other panels used localy */
//extern STR_SCAN_TB  RP_modbus_tb[MAXREMOTEPOINTS]; 
//extern U8_T  RP_modbus_tb_func[MAXREMOTEPOINTS];
extern Byte                 			 number_of_remote_points_modbus;

extern EXT_RAM_ATTR REMOTE_POINTS		    		 remote_points_list_bacnet[MAXREMOTEPOINTS];  /* points from other panels used localy */
//extern U8_T  RP_bacnet_tb_func[MAXREMOTEPOINTS];
//extern STR_BAC_TB  RP_bacnet_tb[MAXREMOTEPOINTS];
extern Byte                 			 number_of_remote_points_bacnet;


extern U16_T Last_Contact_Network_points_bacnet[MAXNETWORKPOINTS];
extern U16_T Last_Contact_Network_points_modbus[MAXNETWORKPOINTS];
extern U16_T Last_Contact_Remote_points_bacnet[MAXREMOTEPOINTS];
extern U16_T Last_Contact_Remote_points_modbus[MAXREMOTEPOINTS];


extern U8_T remote_panel_num;

#define MAX_REMOTE_PANEL_NUMBER 30
extern EXT_RAM_ATTR STR_REMOTE_PANEL_DB  remote_panel_db[MAX_REMOTE_PANEL_NUMBER];

//extern BACNET_DATE Local_Date;
//extern BACNET_TIME Local_Time;

extern Byte	 Station_NUM;
extern Byte  MAX_MASTER;
extern 	U8_T panel_number;


//extern U8_T  flag_Moniter_changed;
//extern U8_T count_monitor_changed;
//extern U8_T  table_bank[TABLE_BANK_LENGTH];
extern U8_T const table_bank[TABLE_BANK_LENGTH];

extern STR_STORE_PIC store_pic[PIC_PACKET_STACK];

void init_panel(void);



void update_timers( void );



#define DEFUATL_REMOTE_NETWORK_VALUE 0x55555555

#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI)
extern UN_ID  ID_Config[254];
extern U8_T  ID_Config_Sche[254];
#endif

extern U8_T  output_pri_live[MAX_OUTS];
extern float  output_priority[MAX_OUTS][16];
extern float  output_relinquish[MAX_OUTS];


U16_T convert_pointer_to_word( U8_T *iAddr );  //	 mGetPointWord
U32_T convert_pointer_to_double( U8_T *iAddr ) ;  // DoulbemGetPointWord

void Initial_Panel_Info(void);
void Sync_Panel_Info(void);

typedef union
{
    unsigned long ldata;
    float  fdata;
}FloatLongType;

#define FLOAT_TYPE_ABCD  1
#define FLOAT_TYPE_CDAB  2
#define FLOAT_TYPE_BADC  3
#define FLOAT_TYPE_DCBA  4


void Float_to_Byte(float f, unsigned char mybyte[],  unsigned char ntype);
//void Byte_to_Float(float *f, unsigned char mybyte[],unsigned char ntype);
void Byte_to_Float(float *f, S32_T val_ptr,unsigned char ntype);


U8_T check_remote_point_list(Point_Net *point,U8_T *index, U8_T protocal);

void put_remote_point_value( S16_T index, S32_T *val_ptr, S16_T prog_op , uint8_t protocal);
void add_remote_point(U8_T id,U8_T point_type,U8_T high_5bit,U8_T number,S32_T val_ptr,U8_T specail,U8_T float_type);
void put_network_point_value( S16_T index, S32_T *val_ptr, S16_T prog_op );
void add_network_point(U8_T panel,U8_T id,U8_T point_type,U8_T number,S32_T val_ptr,U8_T specail,U8_T float_type);

void change_panel_number_in_code(U8_T old, U8_T new_panel);

void check_graphic_element(void);
void check_weekly_routines(void);
void check_annual_routines(void);
void check_output_priority_array(U8_T i,U8_T HOA);
void check_output_priority_HOA(U8_T i);
void check_output_priority_array_without_AM(U8_T i);
void output_dead_master(void);
void clear_dead_master(void);


void push_expansion_out_stack(Str_out_point* ptr,uint8 point,uint8_t type);
void push_expansion_in_stack(Str_in_point* ptr);

U32_T get_current_time(void);


int GetPrivateBacnetToModbusData(uint32_t deviceid, uint16_t start_reg, int16_t readlength, uint16_t *data_out,uint8_t protocal);
int WritePrivateBacnetToModbusData(uint32_t deviceid, int16_t start_reg, uint16_t writelength, uint32_t data_in);
void Bacnet_Initial_Data(void);
#endif

