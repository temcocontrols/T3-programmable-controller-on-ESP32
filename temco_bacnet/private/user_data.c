#include "bacnet.h"
#include "string.h"
#include "types.h"
#include "bac_point.h"
#include "user_data.h"
//#include "clock.h"
//#include "alarm.h"
//#include "controls.h"
#include "define.h"
//#include "sntpc.h"
//#include "tcpip.h"
//#include "main.h"
#include "datetime.h"
//#include "wifi.h"
#include "esp_timer.h"

void write_parameters_to_nodes(uint8_t func,uint8 id, uint16 reg, uint16* value,uint8_t len);
int write_NP_Modbus_to_nodes(U8_T ip,U8_T func,U8_T sub_id, U16_T reg, U16_T * value, U8_T len);

/*U8_T rec_mstp_index;  // for i-am
U8_T send_mstp_index;
EXT_RAM_ATTR STR_SEND_BUF mstp_bac_buf[10];

U8_T rec_mstp_index1; // response packets form
U8_T send_mstp_index1;
EXT_RAM_ATTR STR_SEND_BUF mstp_bac_buf1[10];*/
void Send_MSTP_to_BIPsocket(uint8_t * buf,uint16_t len);

extern S16_T	 timezone;
extern U8_T  input_type[32];
extern U8_T  input_type1[32];
extern STR_MODBUS Modbus;
U8_T Daylight_Saving_Time;
U16_T SW_REV;
U8_T PRODUCT;
U8_T uart0_baudrate;
U8_T uart1_baudrate;
U8_T uart2_baudrate;


//Str_points_ptr 	point_str[3];

U8_T SD_exist;
#define DEFAULT_FILTER 5

U8_T E2prom_Read_Byte(U16_T addr,U8_T *value);
U8_T E2prom_Write_Byte(U16_T addr,U16_T dat);

U32_T 				timestart;	   /* seconds since the beginning of the year */
U32_T 				time_since_1970;

extern U8_T  SD_exist;
uint16_t 	start_day;
uint16_t	end_day;

extern uint8_t flag_start_scan_network;
extern uint8_t start_scan_network_count;
extern uint16_t scan_network_bacnet_count;

BACNET_DATE Local_Date;
BACNET_TIME Local_Time;

/*EXT_RAM_ATTR*/ UN_Time  update_dyndns_time;
//U32_T test111,test222;
 U8_T max_dos;
 U8_T max_aos;

 U16_T  input_raw[MAX_INS];
 U16_T  input_raw_back[MAX_INS];
 U16_T  output_raw[MAX_OUTS];
 U16_T  output_raw_back[MAX_OUTS];
 U16_T  chip_info[6];
 U32_T  Instance;

//U8_T  control_auto[MAX_OUTS];
//U8_T  switch_status_back[MAX_OUTS];

U8_T  boot;

uint16 max_inputs;
uint16 max_outputs;
uint16 max_vars;

U32_T  update_sntp_last_time;
EXT_RAM_ATTR Str_Extio_point  extio_points[MAX_EXTIO];

EXT_RAM_ATTR Str_in_point  inputs[MAX_INS];// _at_ 0x20000;
EXT_RAM_ATTR Str_out_point outputs[MAX_OUTS];//_at_ 0x22000;
U8_T  month_length[12];// = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
//U8_T  table_week[12];// = {0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5};	//���������ݱ�	  

Info_Table									 	info[18];// _at_ 0x41000;
EXT_RAM_ATTR S8_T   							var_unit[MAX_VAR_UNIT][VAR_UNIT_SIZE];


EXT_RAM_ATTR Str_Special  						Write_Special;
EXT_RAM_ATTR Str_MISC   						MISC_Info;
EXT_RAM_ATTR Str_Remote_TstDB             		Remote_tst_db;// _at_ 0x10000;
EXT_RAM_ATTR Str_Panel_Info 		 			Panel_Info;
EXT_RAM_ATTR Str_Setting_Info     				Setting_Info;
//In_aux				far			in_aux[MAX_IO_POINTS];//_at_ 0x17500; 
//Con_aux				far			con_aux[MAX_CONS];//_at_ 0x26000; 
//Mon_aux           	         mon_aux[MAX_MONITORS];// _at_ 0x27000; 
//U32_T 				 		SD_lenght[MAX_MONITORS * 2];
//U32_T 				 		SD_block_num[MAX_MONITORS * 2];

//Monitor_Block		far			mon_block[2 * MAX_MONITORS];

EXT_RAM_ATTR Mon_Data 			 				*Graphi_data;
EXT_RAM_ATTR S8_T 								Garphi_data_buf[sizeof(Mon_Data)];//  _at_ 0x41100;
EXT_RAM_ATTR Alarm_point 						alarms[MAX_ALARMS];
EXT_RAM_ATTR U8_T 			    				ind_alarms;
EXT_RAM_ATTR Alarm_set_point 	    			alarms_set[MAX_ALARMS_SET];
U8_T 			    							ind_alarms_set;
U16_T                							alarm_id;
S8_T                         					new_alarm_flag;
EXT_RAM_ATTR Units_element		    			digi_units[MAX_DIG_UNIT];
U8_T 					 						ind_passwords;
EXT_RAM_ATTR Password_point						passwords[ MAX_PASSW ];
EXT_RAM_ATTR Str_program_point	    			programs[MAX_PRGS];// _at_ 0x24000;
EXT_RAM_ATTR Str_variable_point					vars[MAX_VARS];// _at_ 0x18000;
EXT_RAM_ATTR Str_controller_point 				controllers[MAX_CONS];// _at_ 0x25000;
EXT_RAM_ATTR Str_totalizer_point          		totalizers[MAX_TOTALIZERS];// _at_ 0x12500;


EXT_RAM_ATTR Str_monitor_point					monitors[MAX_MONITORS];// _at_ 0x12800;
EXT_RAM_ATTR Str_monitor_point					backup_monitors[MAX_MONITORS];// _at_ 0x2e000;

EXT_RAM_ATTR multiple_struct 					msv_data[MAX_MSV][STR_MSV_MULTIPLE_COUNT];

//Str_Email_point Email_Setting;
//U32_T  count_max_time[MAX_MONITORS];
//U32_T  max_monitor_time[MAX_MONITORS];

//Str_mon_element          mon_point[MAX_MONITORS * 2][MAX_MON_POINT];

//Aux_group_point          		aux_groups[MAX_GRPS];// _at_ 0x13500;
//S8_T                    far		Icon_names[MAX_ICONS][14];
EXT_RAM_ATTR Control_group_point  	 		control_groups[MAX_GRPS];// _at_ 0x14000;
//Str_grp_element		    far	    	group_data[MAX_ELEMENTS];// _at_ 0x14500;
EXT_RAM_ATTR Str_grp_element			   	  group_data[MAX_ELEMENTS];

S16_T 					far							 total_elements;
S16_T 					far							 group_data_length;


EXT_RAM_ATTR Str_weekly_routine_point 	far		weekly_routines[MAX_WR];//_at_ 0x28000; // _at_ 0x15200;
EXT_RAM_ATTR Wr_one_day					far		wr_times[MAX_WR][MAX_SCHEDULES_PER_WEEK];//_at_ 0x29000 ;//_at_ 0x16000;
EXT_RAM_ATTR U8_T  wr_time_on_off[MAX_WR][MAX_SCHEDULES_PER_WEEK][8];
EXT_RAM_ATTR Str_annual_routine_point	far	 	annual_routines[MAX_AR];// _at_ 0x2a000;//_at_ 0x16500;
EXT_RAM_ATTR U8_T                   		     ar_dates[MAX_AR][AR_DATES_SIZE];//_at_ 0x2b000 ;//_at_ 0x17000;

//UN_ID  ID_Config[254];
//U8_T  ID_Config_Sche[254];

float  output_priority[MAX_OUTS][16];
float  output_relinquish[MAX_OUTS];

//Date_block	ora_current;
 /* Assume bit0 from octet0 = Jan 1st */
//S8_T 			    	far			*program_address[MAX_PRGS]; /*pointer to code*/
EXT_RAM_ATTR U8_T    	    					prg_code[MAX_PRGS][CODE_ELEMENT * MAX_CODE];// _at_ 0x8000; 
//U16_T				far	 			Code_len[MAX_PRGS];
U16_T 			    				Code_total_length;
//Str_array_point 	    			 arrays[MAX_ARRAYS];
//S32_T  			    				*arrays_address[MAX_ARRAYS];
Str_table_point							 custom_tab[MAX_TBLS];  // defined in io_control.lib
U16_T                   		PRG_crc;
//U8_T                         free_mon_blocks;
//U16_T  total_send_length;
//Byte Station_NUM;
//U8_T MAX_MASTER;
U8_T panel_number;

S8_T  panelname[20];



EXT_RAM_ATTR NETWORK_POINTS      		  network_points_list_bacnet[MAXNETWORKPOINTS];	 /* points wanted by others */
Byte              			  number_of_network_points_bacnet; 

EXT_RAM_ATTR NETWORK_POINTS      		  network_points_list_modbus[MAXNETWORKPOINTS];	 /* points wanted by others */
Byte              			  number_of_network_points_modbus; 

EXT_RAM_ATTR REMOTE_POINTS		    		  remote_points_list_bacnet[MAXREMOTEPOINTS];  /* points from other panels used localy */
Byte              			  number_of_remote_points_bacnet;


EXT_RAM_ATTR REMOTE_POINTS		    		  remote_points_list_modbus[MAXREMOTEPOINTS];  /* points from other panels used localy */
Byte              			  number_of_remote_points_modbus;

U16_T Last_Contact_Network_points_bacnet[MAXNETWORKPOINTS];
U16_T Last_Contact_Network_points_modbus[MAXNETWORKPOINTS];
U16_T Last_Contact_Remote_points_bacnet[MAXREMOTEPOINTS];
U16_T Last_Contact_Remote_points_modbus[MAXREMOTEPOINTS];


U8_T remote_panel_num;
// include remote mstp device and network bip device
EXT_RAM_ATTR STR_REMOTE_PANEL_DB  remote_panel_db[MAX_REMOTE_PANEL_NUMBER];


//U8_T 	client_ip[4];
//U8_T newsocket = 0;

U8_T  *prog;
S32_T  stack[20];
S32_T  *index_stack;
U8_T  *time_buf;
U32_T  cond;
S32_T  v, value;
S32_T  op1,op2;
S32_T  n,*pn;
S8_T  message[ALARM_MESSAGE_SIZE + 26 + 10];
U8_T alarm_flag;
S8_T alarm_at_all;
S8_T ind_alarm_panel;
S8_T alarm_panel[5];
U16_T alarm_index;

//U8_T  flag_Moniter_changed;
//U8_T count_monitor_changed;
//extern STR_flag_flash 	 bac_flash;

U8_T const table_bank[TABLE_BANK_LENGTH] =
{
	 MAX_OUTS,     	/*OUT*/
	 MAX_INS,     	/*IN*/
	 MAX_VARS,      /*VAR*/
	 MAX_CONS,     	/*CON*/
	 MAX_WR,        /*WR*/
	 MAX_AR,        /*AR*/
	 MAX_PRGS,     	/*PRG*/
	 MAX_TBLS,      /*TBL*/
	 MAX_TOTALIZERS, /*TOTAL*/
	 MAX_MONITORS,	/*AMON*/
	 MAX_GRPS,      /*GRP*/
	 MAX_ARRAYS,    /*AY*/
	 MAX_ALARMS,
	 MAX_DIG_UNIT,
	 MAX_PASSW
};



void init_info_table( void )
{
	S16_T i;
	Info_Table *inf;
	Point *pt1 = NULL;

	inf = &info[0];
	for( i=0; i< MAX_INFO_TYPE; i++, inf++ )
	{
		switch( i )
		{
			case OUT:
				inf->address = (S8_T *)outputs;
				inf->size = sizeof( Str_out_point );
				inf->max_points =  MAX_OUTS;
				break;
			case IN:
				inf->address = (S8_T *)inputs;
				inf->size = sizeof( Str_in_point );
				inf->max_points =  MAX_INS;
				break;
			case VAR:
				inf->address = (S8_T *)vars;
				inf->size = sizeof( Str_variable_point );
				inf->max_points =  MAX_VARS;
				break;
			case CON:
				inf->address = (S8_T *)controllers;
				inf->size = sizeof( Str_controller_point );
				inf->max_points = MAX_CONS;
				break;
			case WRT:
				inf->address = (S8_T *)weekly_routines;
				inf->size = sizeof( Str_weekly_routine_point );
				inf->max_points = MAX_WR;
				break;
			case AR:
				inf->address = (S8_T *)annual_routines;
				inf->size = sizeof( Str_annual_routine_point );
				inf->max_points = MAX_AR;
				break;
			case PRG:
				inf->address = (S8_T *)programs;
				inf->size = sizeof( Str_program_point );
				inf->max_points = MAX_PRGS;
				break;
			case TBL:
				inf->address = (S8_T *)custom_tab;
				inf->size = sizeof( Str_table_point );
				inf->max_points = MAX_TBLS;
				break; 
			case TZ:
				inf->address = (S8_T *)totalizers;
				inf->size = sizeof( Str_totalizer_point );
				inf->max_points = MAX_TOTALIZERS;
				break;
			case AMON:
				inf->address = (S8_T *)monitors;
				inf->size = sizeof( Str_monitor_point );
				inf->max_points = MAX_MONITORS;
				break;
			case GRP:
				inf->address = (S8_T *)control_groups;
				inf->size = sizeof( Control_group_point );
				inf->max_points = MAX_GRPS;
				break;
		/*	case ARRAY:
				inf->address = (S8_T *)arrays;
				inf->size = sizeof( Str_array_point );
				inf->max_points = MAX_ARRAYS;
				break;	*/
			case ALARMM:          // 12
				inf->address = (S8_T *)alarms;
				inf->size = sizeof( Alarm_point );
				inf->max_points = MAX_ALARMS;
				break;
			case ALARM_SET:         //15
				inf->address = (S8_T *)alarms_set;
				inf->size = sizeof( Alarm_set_point );
				inf->max_points = MAX_ALARMS_SET;
				break;
			case UNIT:
				inf->address = (S8_T *)digi_units;
				inf->size = sizeof( Units_element );
				inf->max_points = MAX_DIG_UNIT;
				break;										  
			case USER_NAME:
				inf->address = (S8_T *)&passwords;
				inf->size = sizeof( Password_point );
				inf->max_points = MAX_PASSW;
				break;	  
			case WR_TIME:
				inf->address = (S8_T *)wr_times;
				inf->size = 9*sizeof( Wr_one_day );
				inf->max_points = MAX_SCHEDULES_PER_WEEK;
				break;
			case AR_DATA:               // 17 ar_dates[MAX_AR][AR_DATES_SIZE];
				inf->address = (S8_T *)ar_dates;
				inf->size = AR_DATES_SIZE;
				inf->max_points = MAX_AR;
				break;
			default:
				break;	
		}
	}
}

uint16_t get_network_number(void)
{
	return Modbus.network_number;
}

void init_panel(void)
{	 	
	uint16_t i,j;
	uint8 temp[2];
	Str_points_ptr ptr;
	
/*	Point *pt1 = NULL;
	S32_T value = 0;
	get_point_value1(pt1,&value);*/

	just_load = 1;
	miliseclast_cur = 0;
	miliseclast = 0;
	
	Test[40]++;
	#if 0
	if(Test[45] == 100)
	{Test[43]++;
	/*if(max_inputs == 0)*/ 	max_inputs = MAX_INS;
	/*if(max_outputs == 0)*/ 	max_outputs = MAX_OUTS;
	/*if(max_vars == 0)*/ 		max_vars = MAX_VARS;
	point_str[OUT].pout = (uint8_t*)malloc(max_outputs * sizeof(Str_out_point));
	point_str[IN].pin = (uint8_t*)malloc(max_inputs * sizeof(Str_in_point));
	point_str[VAR].pvar = (uint8_t*)malloc(max_vars * sizeof(Str_variable_point));
	

	for( i=0; i< max_inputs; i++, point_str[IN].pin++ )
	{
		point_str[IN].pin->value = 0;  
		sprintf((S8_T *)&point_str[IN].pin->description,"IN %d",(U16_T)(i + 1));
		point_str[IN].pin->filter = DEFAULT_FILTER;  /* (3 bits; 0=1,1=2,2=4,3=8,4=16,5=32, 6=64,7=128,)*/
		point_str[IN].pin->decom = 0;	   /* (1 bit; 0=ok, 1=point decommissioned)*/
//		point_str[IN].pin->sen_on = 1;/* (1 bit)*/
//		point_str[IN].pin->sen_off = 1;  /* (1 bit)*/
		point_str[IN].pin->control = 1; /*  (1 bit; 0=OFF, 1=ON)*/
		point_str[IN].pin->auto_manual = 0; /* (1 bit; 0=auto, 1=manual)*/
		point_str[IN].pin->digital_analog = 1; /* (1 bit; 1=analog, 0=digital)*/
		point_str[IN].pin->calibration_sign = 1;; /* (1 bit; sign 0=positiv 1=negative )*/
//		point_str[IN].pin->calibration_increment = 1;; /* (1 bit;  0=0.1, 1=1.0)*/
		point_str[IN].pin->calibration_hi = 0;  /* (8 bits; -25.6 to 25.6 / -256 to 256 )*/
		point_str[IN].pin->calibration_lo = 0; 
//		memcpy(point_str[IN].pin->label,'\0',9);	

		sprintf((S8_T *)&point_str[IN].pin->label,"IN%d",(U16_T)(i + 1));
	}

	for( i=0; i < max_outputs; i++, point_str[OUT].pout++ )
	{
		point_str[OUT].pout->value = 0; 		
		point_str[OUT].pout->range = 0;
		point_str[OUT].pout->digital_analog = 0;

		if((i >= 0) && (i < 12))
		{
			point_str[OUT].pout->range = 1; // off-on
			point_str[OUT].pout->digital_analog = 0;
		}
		else if((i >= 12) && (i < 24))
		{
			point_str[OUT].pout->range = 4;  // 0-100%
			point_str[OUT].pout->digital_analog = 1;
		}			
		
		
		sprintf((S8_T *)&point_str[OUT].pout->description,"OUT%d",(U16_T)(i + 1));
		sprintf((S8_T *)&point_str[OUT].pout->label,"OUT%d",(U16_T)(i + 1));
		point_str[OUT].pout->auto_manual = 0;
	} 

	for( i=0; i < max_vars; i++, point_str[VAR].pvar++ )
	{
		point_str[VAR].pvar->value = 0;
		point_str[VAR].pvar->auto_manual = 0;
		point_str[VAR].pvar->digital_analog = 1; //analog point 
		point_str[VAR].pvar->unused = 2; 
		point_str[VAR].pvar->range = 0;
	}

	}
	else
#endif
	{
	memset(inputs, 0, MAX_INS *sizeof(Str_in_point) );

	ptr.pin = inputs;
	for( i=0; i< MAX_INS; i++, ptr.pin++ )
	{
		ptr.pin->value = 0;  
		sprintf((S8_T *)&ptr.pin->description,"IN %d",(U16_T)(i + 1));
		ptr.pin->filter = DEFAULT_FILTER;  /* (3 bits; 0=1,1=2,2=4,3=8,4=16,5=32, 6=64,7=128,)*/
		ptr.pin->decom = 0;	   /* (1 bit; 0=ok, 1=point decommissioned)*/
//		ptr.pin->sen_on = 1;/* (1 bit)*/
//		ptr.pin->sen_off = 1;  /* (1 bit)*/
		ptr.pin->control = 1; /*  (1 bit; 0=OFF, 1=ON)*/
		ptr.pin->auto_manual = 0; /* (1 bit; 0=auto, 1=manual)*/
		ptr.pin->digital_analog = 1; /* (1 bit; 1=analog, 0=digital)*/
		ptr.pin->calibration_sign = 1;; /* (1 bit; sign 0=positiv 1=negative )*/
//		ptr.pin->calibration_increment = 1;; /* (1 bit;  0=0.1, 1=1.0)*/
		ptr.pin->calibration_hi = 0;  /* (8 bits; -25.6 to 25.6 / -256 to 256 )*/
		ptr.pin->calibration_lo = 0; 
//		memcpy(ptr.pin->label,'\0',9);	

		sprintf((S8_T *)&ptr.pin->label,"IN%d",(U16_T)(i + 1));
	}

	memset(outputs,0, MAX_OUTS *sizeof(Str_out_point) );
	ptr.pout = outputs;
	for( i=0; i<MAX_OUTS; i++, ptr.pout++ )
	{
		ptr.pout->value = 0; 		
		ptr.pout->range = 0;
		ptr.pout->digital_analog = 0;

		if((i >= 0) && (i < 12))
		{
			ptr.pout->range = 1; // off-on
			ptr.pout->digital_analog = 0;
		}
		else if((i >= 12) && (i < 24))
		{
			ptr.pout->range = 4;  // 0-100%
			ptr.pout->digital_analog = 1;
		}			
		
		
		sprintf((S8_T *)&ptr.pout->description,"OUT%d",(U16_T)(i + 1));
		sprintf((S8_T *)&ptr.pout->label,"OUT%d",(U16_T)(i + 1));
		ptr.pout->auto_manual = 0;
	} 
	
	memset(vars,0,MAX_VARS*sizeof(Str_variable_point));
	ptr.pvar = vars;

	for( i=0; i < MAX_VARS; i++, ptr.pvar++ )
	{
		ptr.pvar->value = 0;
		ptr.pvar->auto_manual = 0;
		ptr.pvar->digital_analog = 1; //analog point 
		ptr.pvar->unused = 2; 
		ptr.pvar->range = 0;
	}
	}
	memset(controllers,'\0',MAX_CONS*sizeof(Str_controller_point));
	ptr.pcon = controllers;
	for( i = 0; i < MAX_CONS; i++, ptr.pcon++ )
	{
		ptr.pcon->repeats_per_min = 1;
	}
	memset(programs,'\0',MAX_PRGS *sizeof(Str_program_point));
	ptr.pprg = programs;
	for( i = 0; i < MAX_PRGS; i++, ptr.pprg++ )
	{
		ptr.pprg->on_off = 1;  
		ptr.pprg->auto_manual = 0;
		ptr.pprg->bytes = 0;
//		memcpy(ptr.pprg->description,'\0',21);	
	} //test by chelsea	
	Code_total_length = 0;
	memset(prg_code, '\0', MAX_PRGS * CODE_ELEMENT * MAX_CODE);
 	for(i = 0;i < MAX_PRGS;i++)	
	{	for(j = 0;j < CODE_ELEMENT * MAX_CODE;j++)	
		 prg_code[i][j] = 0;
	}	
//	total_length = 0;
	
	
	memset(points_header,0,MAXREMOTEPOINTS * sizeof(POINTS_HEADER));
	memset(network_points_list_modbus,0,MAXNETWORKPOINTS * sizeof(NETWORK_POINTS));

	ptr.pnp = network_points_list_modbus;
	number_of_network_points_modbus = 0;

	for(i = 0; i < MAXNETWORKPOINTS; i++,ptr.pnp++ )
	{
		ptr.pnp->point_value = DEFUATL_REMOTE_NETWORK_VALUE;
		ptr.pnp->decomisioned = 0;

	}
#if 1//TEST
	memset( control_groups,'\0', MAX_GRPS * sizeof( Control_group_point) );
//	memset( aux_groups,'\0', MAX_GRPS * sizeof( Aux_group_point) );
	memset( group_data, '\0', MAX_ELEMENTS * sizeof( Str_grp_element) );
	total_elements = 0;
	group_data_length = 0;
	ptr.pgrp = control_groups;		
	for(i = 0;i < MAX_GRPS; i++,ptr.pgrp++ )
	{
		ptr.pgrp->element_count = 0;
	}

	memset(con_aux,'\0',MAX_CONS*sizeof(Con_aux));		
	memset(&passwords,'\0',sizeof(Password_point) * MAX_PASSW);

	memset(custom_tab,0,MAX_TBLS * sizeof(Str_table_point));	

	
#if TEST
	memset(network_points_list_bacnet,0,MAXNETWORKPOINTS * sizeof(NETWORK_POINTS));
	
	ptr.pnp = network_points_list_bacnet;
	number_of_network_points_bacnet = 0;
	
	for(i = 0; i < MAXNETWORKPOINTS; i++,ptr.pnp++ )
	{
		ptr.pnp->point_value = DEFUATL_REMOTE_NETWORK_VALUE;
		ptr.pnp->decomisioned = 0;
	}
	

	for(i = 0; i < MAXREMOTEPOINTS; i++ )
	{
		memset(&remote_points_list_bacnet[i],0,sizeof(REMOTE_POINTS));
		remote_points_list_bacnet[i].point_value = DEFUATL_REMOTE_NETWORK_VALUE;
		remote_points_list_bacnet[i].decomisioned = 0;
	}
	number_of_remote_points_bacnet = 0;
	remote_bacnet_index	= 0;




	for(i = 0; i < MAXREMOTEPOINTS; i++ )
	{
		memset(&remote_points_list_modbus[i],0,sizeof(REMOTE_POINTS));
		remote_points_list_modbus[i].point_value = DEFUATL_REMOTE_NETWORK_VALUE;
		remote_points_list_modbus[i].decomisioned = 0;
	}	
	number_of_remote_points_modbus = 0;
	remote_modbus_index = 0;
#endif
	memset( weekly_routines,'\0',MAX_WR*sizeof(Str_weekly_routine_point));
	ptr.pwr = weekly_routines;
	for(i = 0; i < MAX_WR; i++, ptr.pwr++)
	{
		ptr.pwr->value = 0;
		ptr.pwr->auto_manual = 0;
	}

	memset(wr_times,'\0',MAX_WR*9*sizeof(Wr_one_day ));
//	memset(wr_time_on_off,0xff,MAX_WR*9*sizeof(Wr_one_day ));
	memset( annual_routines,'\0',MAX_AR*sizeof(Str_annual_routine_point));
	for( i = 0; i < MAX_AR; i++, ptr.panr++ )
	{
		ptr.panr->value = 0;
		ptr.panr->auto_manual = 0;
	}
	memset( ar_dates,'\0',MAX_AR*46*sizeof(S8_T));
//	memset( ID_Config, 0, 254 * sizeof(UN_ID));
//	memset( ID_Config_Sche, 0, 254);
	
	memset( output_priority, 0, MAX_OUTS * 16 * 4);
	for( i=0; i<MAX_OUTS; i++)
	{
		for(j = 0;j < 16;j++)
			output_priority[i][j] = 0xff;
		output_pri_live[i] = 10;
	}
	memset( totalizers,'\0',MAX_TOTALIZERS*sizeof(Str_totalizer_point));

//	memset(arrays,'\0',MAX_ARRAYS*sizeof(Str_array_point));
//	memset(arrays_data,'\0',MAX_ARRAYS*sizeof(S32_T *));
	memset(digi_units,'\0',MAX_DIG_UNIT*sizeof(Units_element));


	memset( monitors,'\0',MAX_MONITORS*sizeof(Str_monitor_point));		
	memset( backup_monitors,'\0',MAX_MONITORS*sizeof(Str_monitor_point));
//	memset( mon_aux,'\0',MAX_MONITORS*sizeof(Mon_aux) );   
//	mon_block = mon_data_buf;  // tbd: changed by chelsea
	memset( mon_block,'\0',2 * MAX_MONITORS * sizeof(Monitor_Block) );

	memset( read_mon_point_buf,'\0',MAX_MON_POINT * sizeof(Str_mon_element) );
	memset( write_mon_point_buf,'\0',MAX_MON_POINT  * 2 * MAX_MONITORS * sizeof(Str_mon_element) );


	ptr.pmon = monitors;

	for( i=0; i<MAX_MONITORS; i++, ptr.pmon++ )
	{
		ptr.pmon->minute_interval_time = 15;
	}
	
	Graphi_data = (Mon_Data *)Garphi_data_buf;
	memset( Graphi_data,'\0',sizeof(Mon_Data));
	
	ind_alarms_set = 0;
	ind_alarms = 0;
	alarm_id = 0;	
	memset(alarms,0,MAX_ALARMS * sizeof(Alarm_point));
	ptr.palrm = alarms;
	for( i=0; i<MAX_ALARMS; i++, ptr.palrm++ )
	{
		ptr.palrm->alarm = 0;
		ptr.palrm->ddelete = 0;
		ptr.palrm->alarm_id = 0;
	}
	memset(alarms_set,'\0',MAX_ALARMS_SET*sizeof(Alarm_set_point));

	month_length[0] = 31;
	month_length[1] = 28;
	month_length[2] = 31;
	month_length[3] = 30;
	month_length[4] = 31;
	month_length[5] = 30;
	month_length[6] = 31;
	month_length[7] = 31;
	month_length[8] = 30;
	month_length[9] = 31;
	month_length[10] = 30;
	month_length[11] = 31;

	
	memset(input_raw,1000,2 * MAX_INS);
	memset(input_raw_back,1000,2 * MAX_INS);
	memset(output_raw,0,2 * MAX_OUTS);
	memset(output_raw_back,0,2 * MAX_OUTS);
//	memset(chip_info,0,12);
	init_info_table();

	memset(&Remote_tst_db,0,sizeof(Str_Remote_TstDB) );
//	memset(panelname,0,20);

	MISC_Info.reg.flag = 0x55ff;
	memset(MISC_Info.reg.monitor_block_num,0,MAX_MONITORS * 2 * 4);
	MISC_Info.reg.flag1 = 0x55;
	memset(MISC_Info.reg.com_rx,0,12);
	memset(MISC_Info.reg.com_tx,0,12);	
	memset(MISC_Info.reg.collision,0,6);
	memset(MISC_Info.reg.packet_error,0,6);
	memset(MISC_Info.reg.timeout,0,6);
	
	Write_Special.reg.clear_health_rx_tx = 0;
	
	//memcpy(update_dyndns_time.all,0,sizeof(UN_Time));
	
	memset(remote_panel_db,0,sizeof(STR_REMOTE_PANEL_DB) * MAX_REMOTE_PANEL_NUMBER);
	remote_panel_num = 0;
	
	memset(MSTP_Send_buffer,0,600);
	MSTP_buffer_len = 0;
	memset(&MSTP_Rec_Header,0,sizeof(STR_MSTP_REV_HEADER));
	memset(MSTP_Rec_buffer,0,600);
	//	flag_Moniter_changed = 0;
//	count_monitor_changed = 0;
	alarm_index = 0;
	
	memset(input_type,0,32);
	memset(input_type1,0,32);
	
//	memset(control_auto,SW_AUTO,MAX_OUTS);
//	memset(switch_status_back,SW_AUTO,MAX_OUTS);
	
	memset(var_unit,0,MAX_VAR_UNIT * VAR_UNIT_SIZE);
//	memset(extio_points,0,MAX_EXTIO * sizeof(Str_Extio_point));
	

//	memset(panelname,0,20);
//	memset(&Email_Setting, 0, sizeof(Str_Email_point));

	//memset(&SSID_Info,0,sizeof(STR_SSID));
#endif
}

void Initial_Panel_Info(void)
{
	memset(&Panel_Info,0,sizeof(Str_Panel_Info));
	memset(&Setting_Info,0,sizeof(Str_Setting_Info));

}
void Sync_Panel_Info(void)
{
	uint8 temp[2];

	Panel_Info.reg.mac[0] = Modbus.ip_addr[0];
	Panel_Info.reg.mac[1] = Modbus.ip_addr[1];
	Panel_Info.reg.mac[2] = Modbus.ip_addr[2];
	Panel_Info.reg.mac[3] = Modbus.ip_addr[3];
	Panel_Info.reg.mac[4] = 0xBA;
	Panel_Info.reg.mac[5] = 0xC0;
		

	Panel_Info.reg.serial_num[0] = Modbus.serialNum[0];
	Panel_Info.reg.serial_num[1] = Modbus.serialNum[1];
	Panel_Info.reg.serial_num[2] = Modbus.serialNum[2];
	Panel_Info.reg.serial_num[3] = Modbus.serialNum[3];

	Panel_Info.reg.modbus_addr = Modbus.address;
	
	Setting_Info.reg.Daylight_Saving_Time = Daylight_Saving_Time;
	//Setting_Info.reg.zigbee_exist = zigbee_exist;
	Panel_Info.reg.instance = Instance;
	Panel_Info.reg.instance_hi = Instance >> 16;
	Panel_Info.reg.modbus_port = Modbus.tcp_port;
	Panel_Info.reg.hw = Modbus.hardRev;

 //   E2prom_Read_Byte(EEP_TIME_ZONE_LO, &temp[0]); //fandu add timezone ��Ҫ��EEP���ȡ
 //   E2prom_Read_Byte(EEP_TIME_ZONE_HI, &temp[1]);
 //   timezone = temp[1] * 256 + temp[0];

	Setting_Info.reg.time_zone = timezone;
	Setting_Info.reg.tcp_port = Modbus.tcp_port;	
	Setting_Info.reg.update_sntp_last_time =  (update_sntp_last_time);

	
	Panel_Info.reg.sw = SW_REV / 100 + (U16_T)((SW_REV % 100) << 8);

	Setting_Info.reg.pro_info.firmware_asix = SW_REV / 100 + (U16_T)((SW_REV % 100) << 8);
	Setting_Info.reg.sn = Modbus.serialNum[0] + (U16_T)(Modbus.serialNum[1] << 8)
	 + ((U32_T)Modbus.serialNum[2] << 16) + ((U32_T)Modbus.serialNum[3] << 24);




//	Setting_Info.reg.backlight = Modbus.backlight;
	Setting_Info.reg.LCD_time_off_delay = Modbus.LCD_time_off_delay;
	
	Setting_Info.reg.mac_addr[0] = Modbus.mac_addr[0];
	Setting_Info.reg.mac_addr[1] = Modbus.mac_addr[1];
	Setting_Info.reg.mac_addr[2] = Modbus.mac_addr[2];
	Setting_Info.reg.mac_addr[3] = Modbus.mac_addr[3];
	Setting_Info.reg.mac_addr[4] = Modbus.mac_addr[4];
	Setting_Info.reg.mac_addr[5] = Modbus.mac_addr[5];
	
	Setting_Info.reg.modbus_id = Modbus.address;	

	Panel_Info.reg.panel_number	= panel_number;

	Setting_Info.reg.ip_addr[0] = Modbus.ip_addr[0];
	Setting_Info.reg.ip_addr[1] = Modbus.ip_addr[1];
	Setting_Info.reg.ip_addr[2] = Modbus.ip_addr[2];
	Setting_Info.reg.ip_addr[3] = Modbus.ip_addr[3];
	
	Setting_Info.reg.subnet[0] = Modbus.subnet[0];
	Setting_Info.reg.subnet[1] = Modbus.subnet[1];
	Setting_Info.reg.subnet[2] = Modbus.subnet[2];
	Setting_Info.reg.subnet[3] = Modbus.subnet[3];
	
	Setting_Info.reg.getway[0] = Modbus.getway[0];
	Setting_Info.reg.getway[1] = Modbus.getway[1];
	Setting_Info.reg.getway[2] = Modbus.getway[2];
	Setting_Info.reg.getway[3] = Modbus.getway[3];
	

	Setting_Info.reg.mini_type = Modbus.mini_type;

	Setting_Info.reg.en_username = Modbus.en_username;
	Setting_Info.reg.cus_unit = Modbus.cus_unit;
	Setting_Info.reg.usb_mode = Modbus.usb_mode;

	Setting_Info.reg.en_dyndns = Modbus.en_dyndns;

	Setting_Info.reg.pro_info.firmware_rev = chip_info[1]; // firmware
	Setting_Info.reg.pro_info.hardware_rev = chip_info[2];  // hardware
	if(chip_info[1] >= 42 && chip_info[2] == 1)
		Setting_Info.reg.specila_flag |= 0x01;
	else
		Setting_Info.reg.specila_flag &= 0xfe;

	Setting_Info.reg.en_sntp = Modbus.en_sntp;
	Setting_Info.reg.en_time_sync_with_pc = Modbus.en_time_sync_with_pc;
	if(Setting_Info.reg.en_time_sync_with_pc == 1)
		Setting_Info.reg.update_time_sync_pc = 1;
	else
		Setting_Info.reg.update_time_sync_pc = 0;
	
	Setting_Info.reg.network_number = Modbus.network_number;
	Setting_Info.reg.network_number_hi = Modbus.network_number >> 8;
	
	Setting_Info.reg.panel_type = 88;

	Setting_Info.reg.pro_info.harware_rev = Modbus.hardRev;
	

	Setting_Info.reg.pro_info.bootloader_rev = Modbus.IspVer;
	Setting_Info.reg.pro_info.frimware_pic = Modbus.PicVer;

	Setting_Info.reg.com_config[0] = Modbus.com_config[0];
	Setting_Info.reg.com_config[1] = Modbus.com_config[1];
	Setting_Info.reg.com_config[2] = Modbus.com_config[2];


	Setting_Info.reg.uart_parity[0] = Modbus.uart_parity[0];
	Setting_Info.reg.uart_parity[1] = Modbus.uart_parity[1];
	Setting_Info.reg.uart_parity[2] = Modbus.uart_parity[2];
	
	Setting_Info.reg.uart_stopbit[0] = Modbus.uart_stopbit[0];
	Setting_Info.reg.uart_stopbit[1] = Modbus.uart_stopbit[1];
	Setting_Info.reg.uart_stopbit[2] = Modbus.uart_stopbit[2];

//	Setting_Info.reg.refresh_flash_timer = Modbus.refresh_flash_timer;
	Setting_Info.reg.en_plug_n_play = Modbus.external_nodes_plug_and_play;

//	Setting_Info.reg.clear_tstat_db = Modbus.clear_tstat_db;
	Setting_Info.reg.com_baudrate[0] = Modbus.baudrate[0];
	Setting_Info.reg.com_baudrate[1] = Modbus.baudrate[1];
	Setting_Info.reg.com_baudrate[2] = Modbus.baudrate[2];

	Setting_Info.reg.panel_number	= panel_number;
	
	Setting_Info.reg.instance = Instance;	
	Setting_Info.reg.en_panel_name = 1;
	memcpy(Setting_Info.reg.panel_name,panelname,20);

	Setting_Info.reg.sd_exist = SD_exist;

	Panel_Info.reg.product_type = 88;


	
//	Setting_Info.reg.network_ID[0] = Modbus.network_ID[0];
//	Setting_Info.reg.network_ID[1] = Modbus.network_ID[1];
//	Setting_Info.reg.network_ID[2] = Modbus.network_ID[2];
	

	Setting_Info.reg.MAX_MASTER = MAX_MASTER;


	
	Setting_Info.reg.start_month = Modbus.start_month;
	Setting_Info.reg.start_day = Modbus.start_day;
	Setting_Info.reg.end_month = Modbus.end_month;
	Setting_Info.reg.end_day = Modbus.end_day;
	
}


	


extern U32_T system_timer;

/*U32_T get_current_time(void)
{
	return time_since_1970 + system_timer / 1000;//timezone ?????????????
}*/

#if BAC_TRENDLOG
U32_T my_mktime(UN_Time* t)
{
	return Rtc_Set(t->Clk.year,t->Clk.mon,t->Clk.day,t->Clk.hour,t->Clk.min,t->Clk.sec,1);
}



#endif


void Bacnet_Initial_Data(void)
{
	init_panel(); // for test now

}

U32_T response_bacnet_ip = 0;
U32_T response_bacnet_port = 0;
void Response_bacnet_Start(void);

extern U16_T count_send_whois;
//extern xSemaphoreHandle sem_mstp;
void Send_whois_to_mstp(uint32_t instance)
{

	U8_T count;
	if((Modbus.mini_type >= 5/*MINI_BIG_ARM*/) && (Modbus.mini_type <= 8/*MINI_NANO*/))
	{
	if(Modbus.com_config[2] == 9/*BACNET_MASTER*/ || Modbus.com_config[0] == 9/*BACNET_MASTER*/)
	{
// avoid sending out who_is frequently, interval is at least 30s
// count_send_whois is increased per 5ms
//		if(count_send_whois > 6000)  
		{
//			if(cSemaphoreTake(sem_mstp, 500) == 1)
//			{
//				Send_Whois_Flag = 1;	
//				count_send_whois = 0;			
//			}
//			cSemaphoreGive(sem_mstp);	
		}
		// wait send who is
		if(remote_panel_num > 0)
		{
			U8_T i = 0;
			if(instance == 0)  // send all
			{
				for(i = 0;i < remote_panel_num;i++)
				{
					if(remote_panel_db[i].protocal == BAC_MSTP)
					{
						Send_SUB_I_Am(i);  // mstp device
					}					
				}
			}
			else
			{
				for(i = 0;i < remote_panel_num;i++)
				{
					if((remote_panel_db[i].protocal == BAC_MSTP)
						&& (remote_panel_db[i].device_id == instance))
					{
						Send_SUB_I_Am(i);  // mstp device
					}					
				}
			}
			
		}
	}
	}

}


extern uint8_t Master_Scan_Network_Count;
void add_remote_panel_db(uint32_t device_id,BACNET_ADDRESS* src,uint8_t panel,uint8_t * pdu,uint8_t pdu_len,uint8_t protocal,uint8_t temcoproduct)
{

	U8_T i;	
	
	if(device_id == 0)
	{
		return;
	}
	if(panel == 0) 
	{
		if(protocal == BAC_IP)
		{// it is customer device or old version product of temco
			if(temcoproduct != 1)
				panel = src->mac[3];
		}
		else
			return;
	}	
	
//	if(panel == 255)
//	{	
//		if(protocal == BAC_IP && temcoproduct == 1)
////after adding preperiatary object,default panel is 255, need get panel number from preperialtary object1 
//			;
//		else
//			return;
//	}
	if(remote_panel_num > MAX_REMOTE_PANEL_NUMBER) 
		return ;
#if (ARM_MINI || ARM_CM5)	
	if((panel < panel_number) && (protocal == BAC_IP))
	{
		Modbus.network_master = 0;
		Master_Scan_Network_Count = 0;
	}
#endif
	//if(Master_node)
	{
		for(i = 0;i < remote_panel_num;i++)
		{
			if(device_id == remote_panel_db[i].device_id)
			{ // already in database.
				remote_panel_db[i].time_to_live = RMP_TIME_TO_LIVE;
				break;							
			}
		}	
		
		// it is a new device
		if(i == remote_panel_num)  // add new panel into DB
		{
			remote_panel_db[remote_panel_num].device_id = device_id;
			remote_panel_db[remote_panel_num].protocal = protocal;	
			remote_panel_db[remote_panel_num].product_model = 8;
			memcpy(&remote_panel_db[remote_panel_num].address,src->mac,6);

			remote_panel_db[remote_panel_num].time_to_live = RMP_TIME_TO_LIVE;

			if(pdu_len < 20)
				memcpy(remote_panel_db[remote_panel_num].remote_iam_buf,pdu,pdu_len);	

			remote_panel_db[remote_panel_num].remote_iam_buf_len = pdu_len;
			if(protocal == BAC_IP)
			{	Test[31]++;
				remote_panel_db[remote_panel_num].panel = panel;					
				if(src->len == 1)  //  sub device,mstp device
				{
					remote_panel_db[remote_panel_num].sub_id = src->adr[0];
				}
				else if(src->len == 6)  // wifi-tstat for cusomter
				{
					remote_panel_db[remote_panel_num].sub_id = src->adr[3];//?????????
				}
				else  // src->len == 0
					remote_panel_db[remote_panel_num].sub_id = panel;				
				
			}
			else // BAC_MSTP
			{ 	Test[32]++;
				remote_panel_db[remote_panel_num].panel = panel_number;//Modbus.network_ID[2];
				remote_panel_db[remote_panel_num].sub_id = panel;
				remote_panel_db[remote_panel_num].product_model = 0;
			}
			
			if(protocal == BAC_IP && src->len == 0 && temcoproduct == 1)
				// current device is temco device, and it is not subdevice
			{
				if(panel == 255)
				{
					remote_panel_db[remote_panel_num].sub_id = src->mac[3];
					remote_panel_db[remote_panel_num].panel = src->mac[3];
					remote_panel_db[remote_panel_num].product_model = 1;  // temco product
					
				}
				
			}
			remote_panel_db[remote_panel_num].retry_reading_panel = 0;
			remote_panel_num++;
			Test[30] = remote_panel_num;			
		}
	}

}

#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI )
void Check_Remote_Panel_Table(void)
{
	STR_REMOTE_PANEL_DB * ptr;
	U8_T i;
	static U8_T count = 0;
	if(count++ >= 180)
	{
		count = 0;
		ptr = &remote_panel_db[0];
		for(i = 0;i < remote_panel_num;i++,ptr++)
		{			
			ptr->time_to_live--;
			if(ptr->time_to_live < 0)
			{
				if(remote_panel_num > 0)
				{

				memcpy(ptr,ptr+1,(remote_panel_num - i - 1) * sizeof(STR_REMOTE_PANEL_DB));
				memset(&remote_panel_db[remote_panel_num - 1],0,sizeof(STR_REMOTE_PANEL_DB));
				remote_panel_num--;
				}
			}
		}
	}
}
#endif

U8_T Get_address_by_panel(uint8 panel,U8_T *addr)
{
	U8_T i;
	for(i = 0;i < remote_panel_num;i++)
	{
		if(panel == remote_panel_db[i].panel)
		{
			memcpy(addr,remote_panel_db[i].address,6);
			return TRUE;
		}
	}
// if device is 0, need GET_PANEL_INFO to get panel first
	return FALSE;
}

U8_T Get_address_by_instacne(uint32_t instnace,U8_T *addr)
{
	U8_T i;
	for(i = 0;i < remote_panel_num;i++)
	{
		if(instnace == remote_panel_db[i].device_id)
		{
			memcpy(addr,remote_panel_db[i].address,6);
			return TRUE;
		}
	}
// if device is 0, need GET_PANEL_INFO to get panel first
	return FALSE;
}

U32_T Get_device_id_by_panel(uint8 panel,uint8 sub_id,uint8_t protocal)
{
	U8_T i;
	if(sub_id == 0)
	{
		if(protocal == BAC_MSTP)			
		{
			
			return 0;
		}
		else if((protocal == BAC_IP_CLIENT) && (panel != panel_number))
		{  // network points, not currnet panel
			
			sub_id = panel;
		}			
	}
  

	for(i = 0;i < remote_panel_num;i++)
	{
		if((panel == remote_panel_db[i].panel) && (sub_id == remote_panel_db[i].sub_id))
		{
			return remote_panel_db[i].device_id;
		}
	}
	// if device is 0, need GET_PANEL_INFO to get panel first

	return 0;
}

S8_T Get_remote_index_by_device_id(U32_T device_id,U8_T *index)
{
	U8_T i;
	for(i = 0;i < remote_panel_num;i++)
	{
		if(device_id == remote_panel_db[i].device_id)
		{
			*index = i;
			return 1;
		}
	}

	return -1;
}

S8_T Get_rmp_index_by_panel(U8_T panel,U8_T sub_id,U8_T *index,U8_T protocal)
{
	U8_T i = 0;
	if(sub_id == 0)
	{
		if(protocal == BAC_MSTP)
			return -1;
		else if((protocal == BAC_IP_CLIENT) && (panel != panel_number))
		{  // network points, not currnet panel
			sub_id = panel;
		}			
	}

	for(i = 0;i < remote_panel_num;i++)
	{
		if((panel == remote_panel_db[i].panel) && (sub_id == remote_panel_db[i].sub_id))
		{
			*index = i;
			return i;
		}
	}
	
// if device is 0, need GET_PANEL_INFO to get panel first
	return -1;
}


// 1 - modbus points
// 2 - bacnet points
U8_T check_point_type(Point_Net * point)
{
	U8_T point_type;
	point_type = (point->point_type & 0x1f) + (point->network_number & 0x60);

	if(point_type == VAR + 1) // point type of T3-IO is VAR
		return 1;
	if((point_type <= MB_REG + 1) 
		&& (point_type >= MB_COIL_REG + 1))
		return 1;	
	if((point_type >= BAC_FLOAT_ABCD + 1) && (point_type <= BAC_FLOAT_DCBA + 1))
		return 1;

	return 0;
}

S8_T get_point_info_by_instacne(Point_Net * point)
{
	U8_T i;
	STR_REMOTE_PANEL_DB *ptr;
	U32_T instance;
	// modbus points do not have instance

	if(check_point_type(point) == 1) return -1;

	if(point->network_number & 0x80)  // first bit is 1, new structure, it is for instance
	{
		instance = (U32_T)((point->network_number & 0x7f) << 16) + (U16_T)(point->panel << 8) + point->sub_id;
		// find panel , sub id and protocal by instance
		if(instance == 0)
			return 0;

		ptr = remote_panel_db;
		for(i = 0;i < remote_panel_num;i++,ptr++)
		{
			if(ptr->device_id == instance)
			{
				point->panel = ptr->panel;
				point->sub_id = ptr->sub_id;
				return 0;
			}
		}
		// if not in current database, try to scan network
#if (ARM_MINI || ARM_CM5 )
			flag_start_scan_network = 1;
			start_scan_network_count = 0;
#endif
		point->panel = 0;
	}
	return -1;	
}

// 81 0b 00 18 
// 01 20 ff ff 00 ff 10 00 c4 02 01 90 de 00 02 58 19 03 21 94

void Send_SUB_I_Am(uint8_t index)
{
	uint16_t mstp_network = 1;

	uint8_t pos;
	uint8_t mtu[50];
	uint8_t len;
	mtu[0] = 0x81;
	mtu[1] = 0x0b;
	mtu[2] = 0x00;
	mtu[3] = 0x1c;
	mtu[4] = 0x01;
	mtu[5] = 0x28;
	mtu[6] = 0xff;
	mtu[7] = 0xff;
	mtu[8] = 0x00;
	mtu[9] = (U8_T)(mstp_network >> 8);
	
	mtu[10] = mstp_network;  // network address  10
	mtu[11] = 0x01;  // BACNET_PROTOCOL_VERSION
	mtu[12] = remote_panel_db[index].sub_id;  // mac id
	mtu[13] = 0xfe; // hop count
	
	mtu[14] = 0x10; // 10 00 i am
	mtu[15] = 0x00;

// pdu
// c4 02 xx xx xx -> instacne
// 21 xx / 22 xx xx -> max apdu len
// 91 03 -> no segment
// 21 xx / 22 xx xx -> vendor id
	len = 16;

// instance
		memcpy(&mtu[len],remote_panel_db[index].remote_iam_buf,5);
		len += 5;	
// max apdu len			
		mtu[len++] = 0x22;
		mtu[len++] = 0x01;		
		mtu[len++] = 0xe0;
// no segment
		mtu[len++] = 0x91;
		mtu[len++] = 0x03;
// vendor id	

		if(remote_panel_db[index].remote_iam_buf[5] == 0x21)
		{
			pos = 1;
		}
		else
		{
			pos = 0;
		}
		
		if(remote_panel_db[index].remote_iam_buf[len - 16 - pos] == 0x21)
		{
			memcpy(&mtu[len],&remote_panel_db[index].remote_iam_buf[10 - pos],2);
			len += 2;
		}
		else if(remote_panel_db[index].remote_iam_buf[len - 16 - pos] == 0x22)
		{
			memcpy(&mtu[len],&remote_panel_db[index].remote_iam_buf[10 - pos],3);
			len += 3;
		}
		Send_MSTP_to_BIPsocket(mtu,len);

}


void chech_mstp_collision(void)
{
	//Test[0]++;//collision[2]++;
}

void check_mstp_packet_error(void)
{
	//Test[1]++;//packet_error[2]++;
}

void check_mstp_timeout(void)
{
	//Test[2]++;//timeout[2]++;
}

#if 1 //BAC_POINT
extern void set_output_raw(uint8_t point,uint16_t value);
extern U8_T  output_pri_live[MAX_OUTS];
S8_T get_point_info_by_instacne(Point_Net * point);
uint16_t get_reg_from_list(uint8_t type,uint8_t index,uint8_t * len);

U8_T Get_product_by_id(U8_T id);
extern U8_T current_online[32];
uint8_t get_max_internal_output(void);


/*
 * ----------------------------------------------------------------------------
 * Function Name: rtrim
 * Purpose: retrim the text
 * Params:
 * Returns:
 * Note: this function is called in search_point funciton
 * ----------------------------------------------------------------------------
 */
S8_T *rtrim(S8_T *text)
{
	S16_T n,i;
	n = strlen(text);
	for (i=n-1;i>=0;i--)
			if (text[i]!=' ')
					break;
	text[i+1]='\0';
	return text;
}



S16_T find_network_point( Point_Net *point )
{
	S16_T i;
	U8_T flag = 0;
	NETWORK_POINTS *ptr;



	if(check_point_type(point) == 1)
	{
		ptr = network_points_list_modbus;
	}
	else

	{
		ptr = network_points_list_bacnet;
	}


//	if( point->network_number == 0x0FFFF )
//	{
//		point->network_number = Setting_Info.reg.network_number;//panel_net_info.network_number;
//		flag = 1;
//	}
//	else
//		flag = 0;

	for( i = 0; i < MAXNETWORKPOINTS; i++, ptr++ )
	{
		if(!memcmp( (void*)point, (void*)&ptr->point, sizeof(Point_Net) ) )
		{
			break;
		}
	}

//	if(flag)
//		point->network_number = 0x0FFFF;
	if( i < MAXNETWORKPOINTS )
	{
	// add to scan tabel
		return i; // returns the index in the list
	}
	else
	{
		return -1;
	}
}


S16_T find_remote_point( Point_Net *point )
{
	S16_T i;
	U8_T flag;
	REMOTE_POINTS *ptr;

	if(check_point_type(point) == 1)
	{
		ptr = remote_points_list_modbus;
	}
	else
	{
		ptr = remote_points_list_bacnet;
	}


	for( i=0; i<MAXREMOTEPOINTS; i++, ptr++ )
	{
		if( ptr->count )
		{
			// change size to 4, dont compare network number
			if( !memcmp( (void*)point, (void*)&ptr->point, sizeof(Point_Net)) )
			{
				break;
			}
		}
	}
//	if( flag )
//		point->network_number = 0xFF;
	if( i < MAXREMOTEPOINTS )
	{
	// add to scan tabel

		return i; // returns the index in the list
	}
	else
		return -1;
}


U8_T check_network_point_list(Point_Net *point,U8_T *index, U8_T protocal)
{
	U16_T i;
	NETWORK_POINTS *ptr;

#if 1//NETWORK_MODBUS
	if(protocal == 0)
		ptr = network_points_list_modbus;
	else
#endif
		ptr = network_points_list_bacnet;


	for(i = 0;i < MAXNETWORKPOINTS;i++,ptr++)
	{
		if(/*(point->network_number == remote_points_list[i].point.network_number) &&*/
		(point->panel == ptr->point.panel) &&
		(point->sub_id == ptr->point.sub_id) &&
		(point->point_type == ptr->point.point_type) &&
		(point->number == ptr->point.number) )
		{
			*index = i;
			return 1;
		}
	}
	return 0;
}


U8_T check_remote_point_list(Point_Net *point,U8_T *index, U8_T protocal)
{
	U16_T i;
	REMOTE_POINTS *ptr;
	if(protocal == 0)
		ptr = remote_points_list_modbus;
	else
		ptr = remote_points_list_bacnet;


	for(i = 0;i < MAXREMOTEPOINTS;i++,ptr++)
	{
		if(/*(point->network_number == remote_points_list[i].point.network_number) &&*/
		(point->panel == ptr->point.panel) &&
		(point->sub_id == ptr->point.sub_id) &&
		(point->point_type == ptr->point.point_type) &&
		(point->number == ptr->point.number) )
		{
			*index = i;
			return 1;
		}
	}
	return 0;
}

// special -- 2: bacnet points, 0/1 - modbus points
void add_remote_point(U8_T id,U8_T point_type,U8_T high_5bit, U8_T number,S32_T val_ptr,U8_T specail,U8_T float_type)
{
	REMOTE_POINTS ptr;//	Point_Net *point;
	U8_T index;
	U8_T type,number_high3bit;
	U8_T protocal;

	ptr.point.panel = panel_number;//Modbus.network_ID[2];
	ptr.point.sub_id = id;

	if(specail == 2)
	{
		protocal = 1;
		type = point_type & 0x1f;
		number_high3bit = point_type & 0xe0;

		if(type == OBJECT_ANALOG_VALUE || type == BAC_AV)
		{
			ptr.point.point_type = BAC_AV + 1 + number_high3bit;
		}
		else if(type == OBJECT_ANALOG_INPUT || type == BAC_AI)
		{
			ptr.point.point_type = BAC_AI+ 1 + number_high3bit;
		}
		else if(type == OBJECT_ANALOG_OUTPUT || type == BAC_AO)
		{
			ptr.point.point_type = BAC_AO + 1 + number_high3bit;
		}
		else if(type == OBJECT_BINARY_OUTPUT || type == BAC_BO)
		{
			ptr.point.point_type = BAC_BO + 1 + number_high3bit;
		}
		else if(type == OBJECT_BINARY_VALUE || type == BAC_BV)
		{
			ptr.point.point_type = BAC_BV + 1 + number_high3bit;
		}
		else if(type == OBJECT_BINARY_INPUT || type == BAC_BI)
		{
			ptr.point.point_type = BAC_BI + 1 + number_high3bit;
		}

	}
	else
	{
		protocal = 0;
		type = point_type & 0x1f;
		number_high3bit = point_type & 0xe0;
		if(high_5bit > 0)
		{
			ptr.point.network_number = 0x80 + high_5bit;//Setting_Info.reg.network_number;
		}
		else
			ptr.point.network_number = 0;
		if(type == READ_COIL)
			ptr.point.point_type = MB_COIL_REG + 1 + number_high3bit;
		else if(type == READ_DIS_INPUT)
			ptr.point.point_type = MB_DIS_REG + 1 + number_high3bit;
		else if(type == READ_INPUT)
			ptr.point.point_type = MB_IN_REG + 1 + number_high3bit;
		else if(type == READ_VARIABLES)
		{
			if(specail == 1)
			{
				ptr.point.point_type = MB_REG + 1 + number_high3bit;
			}
			else   // MB_REG & REG
			{
				ptr.point.point_type = VAR + 1 + number_high3bit;
			}
			if(float_type > 0)
			{
				ptr.point.point_type = ((BAC_FLOAT_ABCD + float_type) & 0x1f) + number_high3bit;
				ptr.point.network_number |= ((BAC_FLOAT_ABCD + float_type) & 0x60);
			}
		}
	}

	ptr.point.number = number;
	ptr.auto_manual = 0;
	if(check_remote_point_list(&ptr.point,&index,protocal))
	{
		if(protocal == 0) // modbus
		{
			if(float_type == 0)
				remote_points_list_modbus[index].point_value = val_ptr * 1000;
			else
			{
				float f;
				Byte_to_Float(&f,val_ptr,float_type);
				remote_points_list_modbus[index].point_value = f * 1000;
			}
		}
		else
		{ // mstp
			remote_points_list_bacnet[index].point_value = val_ptr;
		}

	}
}

#if 1//(ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI )
// special -- 2: bacnet points, 0/1 - modbus points

void add_network_point(U8_T panel,U8_T id,U8_T point_type,U8_T number,S32_T val_ptr,U8_T specail,U8_T float_type)
{
	NETWORK_POINTS ptr;//	Point_Net *point; 
	U8_T index;
	U8_T protocal;
	U8_T type,number_high3bit;
	ptr.point.network_number = 0;//Setting_Info.reg.network_number;
	ptr.point.panel = panel;
	ptr.point.sub_id = id;
	ptr.point.point_type = point_type + 1;
//  ptr->auto_manual = auto_manual;
//	ptr->digital_analog = digital_analog;
//	type = point_type & 0x1f;
	ptr.point.number = number - 1;
	ptr.auto_manual = 0;  
	
	if(specail == 2)
	{
		protocal = 1;
		ptr.point.point_type = point_type + 1;
	}
	else
	{
		type = point_type & 0x1f;
		number_high3bit = point_type & 0xe0;
		
		if(type == READ_COIL)
			ptr.point.point_type = MB_COIL_REG + 1 + number_high3bit;
		else if(type == READ_DIS_INPUT)
			ptr.point.point_type = MB_DIS_REG + 1 + number_high3bit;
		else if(type == READ_INPUT)
		{
			ptr.point.point_type = MB_IN_REG + 1 + number_high3bit;
			float_type = 1;			// read input float 32bit
			protocal = 0;			
		}
		else if(type == READ_VARIABLES)
		{
			if(specail == 1) 
				ptr.point.point_type = MB_REG + 1 + number_high3bit;
			else   // MB_REG & REG
				ptr.point.point_type = VAR + 1 + number_high3bit;	
			
			if(float_type > 0)
			{
				ptr.point.point_type = ((BAC_FLOAT_ABCD + float_type) & 0x1f) + number_high3bit;
				ptr.point.network_number |= ((BAC_FLOAT_ABCD + float_type) & 0x60);
			}
		}
		protocal = 0;
	}
	// network panel

	if(check_network_point_list(&ptr.point,&index,protocal))
	{
#if 1//NETWORK_MODBUS	
		if(protocal == 0) // modbus
		{
			if(float_type == 0)
				network_points_list_modbus[index].point_value = val_ptr*1000;
			else 
			{
				float f;
				Byte_to_Float(&f,val_ptr,float_type);
				network_points_list_modbus[index].point_value = f*1000;
			}			
		}
		else
#endif
		{
			network_points_list_bacnet[index].point_value = val_ptr;
		}			
	}
}
#endif

S16_T insert_remote_point( Point_Net *point, S16_T index )
{
	S16_T i;
	REMOTE_POINTS *ptr;
	U8_T point_type;
	
	if(check_point_type(point) == 1)
	{
		ptr = &remote_points_list_modbus[0];
	}
	else
	{
		ptr = &remote_points_list_bacnet[0];
	}

	if( index < 0 )
	{ /* index < 0 means that the index is unknown */
		if(check_point_type(point) == 1)
		{
			if( number_of_remote_points_modbus >= MAXREMOTEPOINTS )
				return -1;
		}
		else
		{
			if( number_of_remote_points_bacnet >= MAXREMOTEPOINTS )
				return -1;
		}

		if( ( i = find_remote_point( point ) ) >= 0 )
		{ /* the point is in the list */
			(ptr+i)->count++;
			return i;
		}
		else
		{
			for( i=0; i < MAXREMOTEPOINTS; i++, ptr++ )
			{
				if( !ptr->count )
				{
					U8_T *addr;
				// check current id is modbus device or mstp device
					// add modbus command
					if(check_point_type(point) == 1)
					{
						point_type = (point->point_type & 0x1f) + (point->network_number & 0x60);
						remote_points_list_modbus[i].tb.RP_modbus.id = point->sub_id;

						if(point->network_number & 0x80)	// new way, modbus reg is 0-65535
							remote_points_list_modbus[i].tb.RP_modbus.reg = (U16_T)((point->point_type & 0xe0) << 3) + point->number
								+ (U16_T)((point->network_number & 0x1f) << 11);
						else // old way, modbus reg is 0-2047
							remote_points_list_modbus[i].tb.RP_modbus.reg = (U16_T)((point->point_type & 0xe0) << 3) + point->number;

						if(point_type == VAR + 1)
							remote_points_list_modbus[i].tb.RP_modbus.func = READ_VARIABLES;
						else if(point_type == MB_REG + 1)
							remote_points_list_modbus[i].tb.RP_modbus.func = READ_VARIABLES + 0x80;
						else if(point_type == MB_COIL_REG + 1)
							remote_points_list_modbus[i].tb.RP_modbus.func = READ_COIL;
						else if(point_type == MB_DIS_REG + 1)
							remote_points_list_modbus[i].tb.RP_modbus.func = READ_DIS_INPUT;
						else if(point_type == MB_IN_REG + 1)
							remote_points_list_modbus[i].tb.RP_modbus.func = READ_INPUT;
						else if((point_type >= BAC_FLOAT_ABCD + 1) &&( point_type <= BAC_FLOAT_DCBA + 1))
						{
							remote_points_list_modbus[i].tb.RP_modbus.func = READ_VARIABLES
							+ ((point_type - BAC_FLOAT_ABCD) << 8);
						}
						else	// other things
							remote_points_list_modbus[i].tb.RP_modbus.func = READ_VARIABLES;
						number_of_remote_points_modbus++;
					}
					else
					{
						point_type = (point->point_type & 0x1f) + (point->network_number & 0x60);
					// add AV,AI,AO,BO ... mstp command
						remote_points_list_bacnet[i].tb.RP_bacnet.panel = point->sub_id;
						remote_points_list_bacnet[i].tb.RP_bacnet.instance =
							(U16_T)((point->point_type & 0xe0) << 3) + point->number;
						if(point_type == BAC_AV + 1)
							remote_points_list_bacnet[i].tb.RP_bacnet.object = OBJECT_ANALOG_VALUE;
						else if(point_type == BAC_AI + 1)
							remote_points_list_bacnet[i].tb.RP_bacnet.object = OBJECT_ANALOG_INPUT;
						else if(point_type == BAC_AO + 1)
							remote_points_list_bacnet[i].tb.RP_bacnet.object = OBJECT_ANALOG_OUTPUT;
						else if(point_type == BAC_BO + 1)
							remote_points_list_bacnet[i].tb.RP_bacnet.object = OBJECT_BINARY_OUTPUT;
						else if(point_type == BAC_BV + 1)
							remote_points_list_bacnet[i].tb.RP_bacnet.object = OBJECT_BINARY_VALUE;
						else if(point_type == BAC_BI + 1)
							remote_points_list_bacnet[i].tb.RP_bacnet.object = OBJECT_BINARY_INPUT;
						number_of_remote_points_bacnet++;
					}

					memcpy( &ptr->point, point, sizeof(Point_Net) );
					ptr->point_value = DEFUATL_REMOTE_NETWORK_VALUE;
					//if( point->network_number == 0x0FFFF)
					//ptr->point.network_number = Setting_Info.reg.network_number;
					ptr->count = 1;
//					ptr->change = 0;


					return i;
				}
			}
			return -1;
		}
	}
	else
	{
		ptr += index;
		ptr->count++;
		return index;
	}
}

#if 1//(ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI )
S16_T insert_network_point( Point_Net *point, S16_T index )
{
	S16_T i;
	NETWORK_POINTS *ptr;
	U8_T point_type;
#if 1//NETWORK_MODBUS
	if(check_point_type(point) == 1)
	{ // only support MB_COIL_REG to MB_REG
		ptr = &network_points_list_modbus[0];
	}
	else
#endif
	{
		ptr = &network_points_list_bacnet[0];
	}

	if( index < 0 )
	{ /* index < 0 means that the index is unknown */

#if 1//NETWORK_MODBUS
		if(check_point_type(point) == 1)
		{ // only support MB_COIL_REG to MB_REG
			if( number_of_network_points_modbus >= MAXNETWORKPOINTS )
				return -1;
		}
		else
#endif
		{
			if( number_of_network_points_bacnet >= MAXNETWORKPOINTS )
				return -1;
		}

		if( ( i = find_network_point( point ) ) >= 0 )
		{ /* the point is in the list */
			return i;
		}
		else
		{
			for( i = 0; i < MAXNETWORKPOINTS; i++, ptr++ )
			{
				if( !ptr->count )
				{
#if 1//NETWORK_MODBUS
				// add modbus command
					if(check_point_type(point) == 1)
					{ // only support MB_COIL_REG to MB_REG
						point_type = (point->point_type & 0x1f) + (point->network_number & 0x60);
							network_points_list_modbus[i].point.panel = point->panel;
							if(point->network_number & 0x80)  // modbus range is 0-65535
									network_points_list_modbus[i].tb.NT_modbus.reg = (U16_T)((point->point_type & 0xe0) << 3) + point->number
									+ (U16_T)((point->network_number & 0x1f) << 11);
							else	// modbus range is 0-2047
								network_points_list_modbus[i].tb.NT_modbus.reg = (U16_T)((point->point_type & 0xe0) << 3) + point->number;

							if(point->sub_id == 0)
								network_points_list_modbus[i].tb.NT_modbus.id = 255;//point->sub_id;
							else
								network_points_list_modbus[i].tb.NT_modbus.id = point->sub_id;

							if((point_type & 0x1f) == VAR + 1)
								network_points_list_modbus[i].tb.NT_modbus.func = READ_VARIABLES;
							else if((point_type & 0x1f) == MB_REG + 1)
								network_points_list_modbus[i].tb.NT_modbus.func = READ_VARIABLES + 0x80;
							else if((point_type & 0x1f) == MB_COIL_REG + 1)
								network_points_list_modbus[i].tb.NT_modbus.func = READ_COIL;
							else if((point_type & 0x1f) == MB_DIS_REG + 1)
								network_points_list_modbus[i].tb.NT_modbus.func = READ_DIS_INPUT;
							else if((point_type & 0x1f) == MB_IN_REG + 1)
								network_points_list_modbus[i].tb.NT_modbus.func = READ_INPUT;
							else if((point_type >= BAC_FLOAT_ABCD + 1) &&( point_type <= BAC_FLOAT_DCBA + 1))
							{
								network_points_list_modbus[i].tb.NT_modbus.func = READ_VARIABLES
								+ ((point_type - BAC_FLOAT_ABCD) << 8);
							}
							else	// other things
								network_points_list_modbus[i].tb.NT_modbus.func = READ_VARIABLES;
					}
					else
					{

					}
#endif
					memcpy( &ptr->point, point, sizeof(Point_Net) );
					ptr->point_value = DEFUATL_REMOTE_NETWORK_VALUE;
					//if( point->network_number == 0x0FFFF )
					//ptr->point.network_number = Setting_Info.reg.network_number;
#if 1//NETWORK_MODBUS
					// add modbus command
					if(check_point_type(point) == 1)
					 // only support MB point
						number_of_network_points_modbus++;
					else
#endif
						number_of_network_points_bacnet++;

					ptr->count = 1;
					return i;
				}
			}
			return -1;
		}
	}
	else
	{
		ptr += index;
		ptr->count++;
		return index;
	}
}
#endif

S32_T my_honts_arm(S32_T val)
{
	U8_T temp[4];
	S32_T temp_32;
	temp[0] = (val & 0xff000000) >> 24;
	temp[1] = (val & 0x00ff0000) >> 16;
	temp[2] = (val & 0x0000ff00) >> 8;
	temp[3] = (val & 0x000000ff);
	temp_32 = temp[0] + (U16_T)(temp[1] << 8) + ((U32_T)temp[2] << 16) + ((U32_T)temp[3] << 24);
	return temp_32;
}

POINTS_HEADER			      points_header[MAXREMOTEPOINTS];

// check if remote bacent points and network points online or not
void Check_bacnet_points_online(void)
{

}

void Update_RM_NT_points_table(void)
{

	U8_T i,j;

	j = 0;

	for(i = 0;i < number_of_remote_points_modbus;i++)
	{
		if(j < MAXREMOTEPOINTS - 1)
		{
			memcpy(&points_header[j],&remote_points_list_modbus[i],sizeof(POINTS_HEADER));
			points_header[j].point_value = my_honts_arm((remote_points_list_modbus[i].point_value));
			points_header[j].instance = my_honts_arm((float)remote_points_list_modbus[i].instance);
			points_header[j].time_to_live = Last_Contact_Remote_points_modbus[j] / 60;
			j++;
		}
	}


	for(i = 0;i < number_of_remote_points_bacnet;i++)
	{
		if(j < MAXREMOTEPOINTS - 1)
		{
			memcpy(&points_header[j],&remote_points_list_bacnet[i],sizeof(POINTS_HEADER));

			points_header[j].point_value = my_honts_arm((float)remote_points_list_bacnet[i].point_value );
			points_header[j].instance = my_honts_arm((float)remote_points_list_bacnet[i].instance);
			points_header[j].time_to_live = Last_Contact_Remote_points_bacnet[j] / 60;
			j++;
		}
	}
#if 1//NETWORK_MODBUS
	for(i = 0;i < number_of_network_points_modbus;i++)
	{
		if(j < MAXNETWORKPOINTS - 1)
		{
			memcpy(&points_header[j],&network_points_list_modbus[i],sizeof(POINTS_HEADER));

			points_header[j].point_value = my_honts_arm((float)network_points_list_modbus[i].point_value);
			points_header[j].instance = my_honts_arm((float)network_points_list_modbus[i].instance);
			points_header[j].time_to_live = Last_Contact_Network_points_modbus[j] / 60;
			j++;
		}
	}
#endif
	for(i = 0;i < number_of_network_points_bacnet;i++)
	{
		if(j < MAXNETWORKPOINTS - 1)
		{
			memcpy(&points_header[j],&network_points_list_bacnet[i],sizeof(POINTS_HEADER));

			points_header[j].point_value = my_honts_arm((float)network_points_list_bacnet[i].point_value);
			points_header[j].instance = my_honts_arm((float)network_points_list_bacnet[i].instance);
			points_header[j].time_to_live = Last_Contact_Network_points_bacnet[j] / 60;
			j++;
		}
	}


	memset(&points_header[j],0,(MAXREMOTEPOINTS - j) * sizeof(POINTS_HEADER));

}


void Check_Net_Point_Table(void)
{
	REMOTE_POINTS *ptr;
	NETWORK_POINTS *ptr1 = NULL;
	U8_T i;
	U8_T product = 0;

	ptr = &remote_points_list_modbus[0];
	if(number_of_remote_points_modbus < MAXREMOTEPOINTS)
	{
		for(i = 0;i < number_of_remote_points_modbus;i++,ptr++)
		{
			// check whether remote point is T3 module
			// if it is T3, dont count live time
			if(ptr->point.panel)
				product = Get_product_by_id(ptr->point.sub_id);

			/*if((product == PM_T34AO) || (product == PM_T3IOA) || (product == PM_T38I13O) || (product == PM_T332AI)
				|| (product == PM_T322AI) || (product == PM_T38AI8AO6DO) || (product == PM_T36CTA) || (product == PM_T3LC)
			|| (product == PM_T3PT12))
			{
				// do not count live time
			}
			else*/
			{
				// check whether device is on line
				if(((current_online[remote_points_list_modbus[i].tb.RP_modbus.id / 8] & (1 << (remote_points_list_modbus[i].tb.RP_modbus.id % 8))))
					|| (product == 0))  // customer device
				{
					Last_Contact_Remote_points_modbus[i]++;
					if(remote_points_list_modbus[i].time_to_live != PERMANENT_LIVE
						&& remote_points_list_modbus[i].time_to_live > 0)
						remote_points_list_modbus[i].time_to_live--;

					//if(ptr->decomisioned == 1)
					{
						if(ptr->time_to_live <= 0)
						{
							if(number_of_remote_points_modbus > 0)
							{
							// delete this point
							//memset(ptr,0,sizeof(REMOTE_POINTS));
								ptr->decomisioned = 0;
								memcpy(ptr,ptr+1,(number_of_remote_points_modbus - i - 1) * sizeof(REMOTE_POINTS));
								//memset(ptr + number_of_remote_points_modbus - i - 1,0,sizeof(REMOTE_POINTS));
								memset(&remote_points_list_modbus[number_of_remote_points_modbus - 1],0,sizeof(REMOTE_POINTS));
								number_of_remote_points_modbus--;

							}
						}
					}
				}
				else
				{
					ptr->time_to_live = 0;
					if(number_of_remote_points_modbus > 0)
					{
					// delete this point
					//memset(ptr,0,sizeof(REMOTE_POINTS));
						ptr->decomisioned = 0;
						memcpy(ptr,ptr+1,(number_of_remote_points_modbus - i - 1) * sizeof(REMOTE_POINTS));
						//memset(ptr + number_of_remote_points_modbus - i - 1,0,sizeof(REMOTE_POINTS));
						memset(&remote_points_list_modbus[number_of_remote_points_modbus - 1],0,sizeof(REMOTE_POINTS));
						number_of_remote_points_modbus--;
					}

				}
			}
		}
	}
	/* else
error
*/

#if 1//(ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI )
	ptr = &remote_points_list_bacnet[0];
	for(i = 0;i < number_of_remote_points_bacnet;i++,ptr++)
	{
		if(remote_points_list_bacnet[i].time_to_live != PERMANENT_LIVE
			&& remote_points_list_bacnet[i].time_to_live > 0)
			remote_points_list_bacnet[i].time_to_live--;
		Last_Contact_Remote_points_bacnet[i]++;
		if(ptr->decomisioned == 1)
		{
			if(ptr->time_to_live <= 0)
			{
				if(number_of_remote_points_bacnet > 0)
				{
				// delete this point
				ptr->decomisioned = 0;
				// move up, clear the last line
				memcpy(ptr,ptr + 1,(number_of_remote_points_bacnet - i - 1) * sizeof(REMOTE_POINTS));
				//memset(ptr + number_of_remote_points_bacnet - i - 1,0,sizeof(REMOTE_POINTS));
				memset(&remote_points_list_bacnet[number_of_remote_points_bacnet - 1],0,sizeof(REMOTE_POINTS));
				number_of_remote_points_bacnet--;
				}

			}
		}
	}
#if 1//NETWORK_MODBUS
	ptr1 = &network_points_list_modbus[0];
	for(i = 0;i < number_of_network_points_modbus;i++,ptr1++)
	{
		if(network_points_list_modbus[i].time_to_live != PERMANENT_LIVE
			&& network_points_list_modbus[i].time_to_live > 0)
			network_points_list_modbus[i].time_to_live--;
		Last_Contact_Network_points_modbus[i]++;
		if(ptr1->decomisioned == 1)
		{
			if(ptr1->time_to_live <= 0)
			{
				// delete this point
				//memset(ptr1,0,sizeof(NETWORK_POINTS));
				ptr1->decomisioned = 0;
				// move up, clear the last line
				memcpy(ptr1,ptr1+1,(number_of_network_points_modbus - i - 1) * sizeof(NETWORK_POINTS));
				//memset(ptr1 + number_of_network_points - i - 1,0,sizeof(NETWORK_POINTS));
				memset(&network_points_list_modbus[number_of_network_points_modbus - 1],0,sizeof(NETWORK_POINTS));
				network_points_list_modbus[number_of_network_points_modbus - 1].point_value = DEFUATL_REMOTE_NETWORK_VALUE;
				number_of_network_points_modbus--;
			}
		}
	}
#endif
	ptr1 = &network_points_list_bacnet[0];
	for(i = 0;i < number_of_network_points_bacnet;i++,ptr1++)
	{
		if(network_points_list_bacnet[i].time_to_live != PERMANENT_LIVE
			&& network_points_list_bacnet[i].time_to_live > 0)
			network_points_list_bacnet[i].time_to_live--;
		Last_Contact_Network_points_bacnet[i]++;
		if(ptr1->decomisioned == 1)
		{
			if(ptr1->time_to_live <= 0)
			{
				// delete this point
				//memset(ptr1,0,sizeof(NETWORK_POINTS));
				ptr1->decomisioned = 0;
				// move up, clear the last line
				memcpy(ptr1,ptr1+1,(number_of_network_points_bacnet - i - 1) * sizeof(NETWORK_POINTS));
				//memset(ptr1 + number_of_network_points - i - 1,0,sizeof(NETWORK_POINTS));
				memset(&network_points_list_bacnet[number_of_network_points_bacnet - 1],0,sizeof(NETWORK_POINTS));
				network_points_list_bacnet[number_of_network_points_bacnet - 1].point_value = DEFUATL_REMOTE_NETWORK_VALUE;
				number_of_network_points_bacnet--;
			}
		}
	}
#endif

	Update_RM_NT_points_table();
}


S16_T get_point_value( Point *point, S32_T *val_ptr )
{

 	Str_points_ptr  sptr;
	if( ( OUTPUT <= point->point_type ) &&( point->point_type <= ARRAYS ))
 	{
  	//if( point->number < table_bank[point->point_type-1] )
		// VAR add 1 int_vars
			if(((point->point_type != VAR + 1) && (point->number < table_bank[point->point_type-1]) )
				|| ((point->point_type == VAR + 1) && (point->number < table_bank[point->point_type-1] + 12))
				)
  		{
				switch( point->point_type-1 )
				{
				case OUT:
					sptr.pout = &outputs[point->number];
					if((!sptr.pout->digital_analog) && (sptr.pout->range != UNUSED)) /* DIGITAL */
					{
					//	*val_ptr = (sptr.pout->control? 1000L:0L);
						if(( sptr.pout->range >= ON_OFF  && sptr.pout->range <= HIGH_LOW )
							||(sptr.pout->range >= custom_digital1 // customer digital unit
							&& sptr.pout->range <= custom_digital8
							&& digi_units[sptr.pout->range - custom_digital1].direct == 1))
						{// inverse
							*val_ptr = (sptr.pout->control ? 0L : 1000L);
						}
						else
							*val_ptr = (sptr.pout->control ? 1000L : 0L);
					}
					else
					{
						*val_ptr = sptr.pout->value;
					}

					break;
				case IN:
					sptr.pin = &inputs[point->number];

					if( (!sptr.pin->digital_analog) && (sptr.pin->range != UNUSED))
					{
						if(( sptr.pin->range >= ON_OFF  && sptr.pin->range <= HIGH_LOW )
					||(sptr.pin->range >= custom_digital1 // customer digital unit
					&& sptr.pin->range <= custom_digital8
					&& digi_units[sptr.pin->range - custom_digital1].direct == 1))
						{// inverse
							*val_ptr = (sptr.pin->control ? 0L : 1000L);
						}
						else
							*val_ptr = (sptr.pin->control ? 1000L : 0L);
					}
					else
					{
						*val_ptr = sptr.pin->value;
					}
					break;
			 	case VAR:
					sptr.pvar = &vars[point->number];

					if( (!sptr.pvar->digital_analog) && (sptr.pvar->range != UNUSED))
					{
						if(( sptr.pvar->range >= ON_OFF  && sptr.pvar->range <= HIGH_LOW )
							||(sptr.pvar->range >= custom_digital1 // customer digital unit
							&& sptr.pvar->range <= custom_digital8
							&& digi_units[sptr.pvar->range - custom_digital1].direct == 1))
						{// inverse
							*val_ptr = (sptr.pvar->control ? 0L : 1000L);
						}
						else
						{
							*val_ptr = (sptr.pvar->control ? 1000L : 0L);
						}
					}
					else
					{
						*val_ptr = sptr.pvar->value;
					}
					break;
			 	case CON:
					*val_ptr = controllers[point->number].value;
					break;
				case WRT:
					*val_ptr = weekly_routines[point->number].value?1000L:0;
					break;
				case AR:
					*val_ptr = annual_routines[point->number].value?1000L:0;
					break;
				case PRG:
					*val_ptr = programs[point->number].on_off?1000L:0;
					break;
				case AMON:
					*val_ptr = monitors[point->number].status ? 1000L : 0;
					break;

				/*case TZ:
					*val_ptr = totalizers[point->number].active?1000L:0;
					break; */

			}

		return point->number;
  		}
 	}
/*
  point->point_type = 0;
  point->number = 0;
*/
	*val_ptr = 0;

	return -1;
}


/*
 * ----------------------------------------------------------------------------
 * Function Name: put_point_value
 * Purpose: according to the type of point, write the value to the Str_points_ptr
 * Params:
 * Returns:
 * Note:
 * ----------------------------------------------------------------------------
 */

S16_T put_point_value( Point *point, S32_T *val_ptr, S16_T aux, S16_T prog_op )
{
 	Str_points_ptr sptr;
	U32_T temp;
	S32_T old_value;
/* write value to point	*/

	if( ( OUTPUT <= point->point_type ) &&	( point->point_type <= ARRAYS ) )
	{
		if( point->number < table_bank[point->point_type-1] )
		{
		 	switch( point->point_type-1 )
		 	{
			case OUT:
				sptr.pout = &outputs[point->number];
				output_pri_live[point->number] = 10;
				if( ((sptr.pout->auto_manual == 0) && (prog_op == 0)) || ((sptr.pout->auto_manual == 1) && (prog_op == 1)) )
				{
					if( !sptr.pout->digital_analog ) 	// DIGITAL
					{
						if(point->number < get_max_internal_output())
						{
								if(( outputs[point->number].range >= ON_OFF && outputs[point->number].range <= HIGH_LOW )
									||(outputs[point->number].range >= custom_digital1 // customer digital unit
									&& outputs[point->number].range <= custom_digital8
									&& digi_units[outputs[point->number].range - custom_digital1].direct == 1))
								{// inverse
									output_priority[point->number][9] = *val_ptr ? 0 : 1;
#if TEST
									outputs[point->number].control = Binary_Output_Present_Value(point->number) ? 0 : 1;
#endif
								}
								else
								{
									output_priority[point->number][9] = *val_ptr ? 1 : 0;
#if Test
									outputs[point->number].control = Binary_Output_Present_Value(point->number) ? 1 : 0;
#endif
								}
						}
						else
						{
							old_value = outputs[point->number].control;

							if(( outputs[point->number].range >= ON_OFF && outputs[point->number].range <= HIGH_LOW )
								||(outputs[point->number].range >= custom_digital1 // customer digital unit
									&& outputs[point->number].range <= custom_digital8
									&& digi_units[outputs[point->number].range - custom_digital1].direct == 1))
							{ // inverse
								outputs[point->number].control = *val_ptr ? 0 : 1;
								outputs[point->number].value = *val_ptr ? 0 : 1000;
							}
							else
							{
								outputs[point->number].control = *val_ptr ? 1 : 0;
								outputs[point->number].value = *val_ptr ? 1000 : 0;
							}
						}

						if(outputs[point->number].control)
							set_output_raw(point->number,1000);
						else
							set_output_raw(point->number,0);

#if  T3_MAP
				if(point->number >= get_max_internal_output())
				{
						if(old_value != outputs[point->number].control)
						{
						vTaskSuspend(Handle_Scan);	// dont not read expansion io
#if (ARM_MINI || ASIX_MINI)
						vTaskSuspend(xHandler_Output);  // do not control local io
#endif
						push_expansion_out_stack(&outputs[point->number],point->number,0);
							// resume output task
#if (ARM_MINI || ASIX_MINI)
						vTaskResume(xHandler_Output);
#endif
						vTaskResume(Handle_Scan);
						}
				}
#endif

					}
					else
					{
						if(point->number < get_max_internal_output())
							output_priority[point->number][9] = (float)(*val_ptr) / 1000;
						// if external io
#if  T3_MAP
						if(point->number >= get_max_internal_output())
						{
							old_value = outputs[point->number].value;
							output_raw[point->number] = *val_ptr;
							if(old_value != *val_ptr)
							{
								vTaskSuspend(Handle_Scan);	// dont not read expansion io
#if (ARM_MINI || ASIX_MINI)
								vTaskSuspend(xHandler_Output);  // do not control local io
#endif
								push_expansion_out_stack(&outputs[point->number],point->number,0);
#if (ARM_MINI || ASIX_MINI)
									// resume output task
								vTaskResume(xHandler_Output);
#endif
								vTaskResume(Handle_Scan);
							}
						}

#endif
					}
					check_output_priority_array(point->number,0);
//					clear_dead_master();
					temp = *val_ptr;
					sptr.pout->value = (temp);

				}
				break;
		  	case IN:
				sptr.pin = &inputs[point->number];
				if( ((sptr.pin->auto_manual == 0) && (prog_op == 0)) || ((sptr.pin->auto_manual == 1) && (prog_op == 1)) )
				{
					if( !sptr.pin->digital_analog )
					{
						 sptr.pin->control = *val_ptr ? 1 : 0;
					}
				//	sptr.pin->value = convert_pointer_to_double(*val_ptr);
					temp = *val_ptr;
					sptr.pin->value = (temp);
				// add clear counter in program
#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI)

					if((inputs[point->number].range == HI_spd_count) || (inputs[point->number].range == N0_2_32counts)
						|| (inputs[point->number].range == RPM)
					)
					{
						if((inputs[point->number].value) == 0)
						{
							high_spd_counter[point->number] = 0; // clear high spd count
							clear_high_spd[point->number] = 1;

						}
					}

#endif
				}
				break;
		  	case VAR:
				sptr.pvar = &vars[point->number];
				if(((sptr.pvar->auto_manual == 0) && (prog_op == 0)) || ((sptr.pvar->auto_manual == 1) && (prog_op == 1)) )
				{
					if(sptr.pvar->digital_analog == 0)
					{
						sptr.pvar->control = *val_ptr ? 1 : 0;
					}
					temp = *val_ptr;
					sptr.pvar->value = (temp);
				}
				break;
		  	case CON:
				sptr.pcon = &controllers[point->number];
				if(( (sptr.pcon->auto_manual == 0) && (prog_op == 0)) || ((sptr.pcon->auto_manual == 1) && (prog_op == 1)) )
				{
				//	sptr.pcon->value = *val_ptr;
					temp = *val_ptr ? 1 : 0;
#if (ASIX_MINI || ASIX_CM5)
					sptr.pcon->value = convert_pointer_to_double(temp);
#endif
				}
				break;
		  	case WRT:
				sptr.pwr = &weekly_routines[point->number];
				if( ((sptr.pwr->auto_manual == 0) && (prog_op == 0)) || ((sptr.pwr->auto_manual== 1) && (prog_op == 1)) )
				{
					temp = *val_ptr ? 1 : 0;
#if (ASIX_MINI || ASIX_CM5)
					sptr.pwr->value = convert_pointer_to_double(temp);
#endif
				}
				break;
		  	case AR:
				sptr.panr = &annual_routines[point->number];
				if( (!sptr.panr->auto_manual && !prog_op) || (sptr.panr->auto_manual && prog_op) )
				{
				//	sptr.panr->value = *val_ptr ? 1 : 0;
					temp = *val_ptr ? 1 : 0;
#if (ASIX_MINI || ASIX_CM5)
					sptr.pwr->value = convert_pointer_to_double(temp);
#endif
				}
				break;
		  	case PRG:
				sptr.pprg = &programs[point->number];
				if( ((sptr.pprg->auto_manual == 0) && (prog_op == 0)) ||( (sptr.pprg->auto_manual == 1) && (prog_op == 1)) )
				{
				//	sptr.pprg->on_off = *val_ptr ? 1 : 0;
					temp = *val_ptr ? 1 : 0;
#if (ASIX_MINI || ASIX_CM5)
					sptr.pprg->on_off = convert_pointer_to_double(temp);
#endif
				}
				break;
		 /*	case ARRAY:
				sptr.pary = &arrays[point->number];
				if( aux >= sptr.pary->length || aux < 0 )				aux = 0;
				arrays_address[point->number][aux] = *val_ptr;
				break;*/

			case AMON:
				sptr.pmon = &monitors[point->number];
				sptr.pmon->status = *val_ptr ? 1 : 0;
				break;

		  /*case TOTAL:
				sptr.ptot = &totalizers[point->number];
				sptr.ptot->active = *val_ptr ? 1 : 0;
				break;*/

	/*			sptr.ptot->time_on = ptr->point_value; */
		 	}
	  	}

		return point->number;
 	}

	point->point_type = 0;
	point->number = 0;
	*val_ptr = 0;

	return -1;
}


/*
 * ----------------------------------------------------------------------------
 * Function Name: put_net_point_value
 * Purpose:
 * Params:
 * Returns:
 * Note:  write remote points and network points,put them into read and write buffer, wait for dealing
 * ----------------------------------------------------------------------------
 */
S16_T put_net_point_value( Point_Net *p, S32_T *val_ptr, S16_T aux, S16_T prog_op , U8_T mode )
{

	S16_T index;
	Point_Net point;
	REMOTE_POINTS * ptr = NULL;
	NETWORK_POINTS * ptr1 = NULL;
	U16_T reg = 0;
	S32_T value = 0;
//	U8_T  temp[4];

	U8_T high_3bit;
	U8_T high_5bit;
	high_3bit = 0;
	high_5bit = 0;
	memcpy( &point, p, sizeof(Point_Net));
/*	point.network_number = Setting_Info.reg.network_number;

	if( point.network_number != Setting_Info.reg.network_number)
		return 0;*/  // change structure, network is used for instance

	
	get_point_info_by_instacne(&point);

	if(point.panel == 0)
		return -1;

	if(/*( point.network_number != Setting_Info.reg.network_number) ||*/( point.panel != panel_number)
		|| ((point.sub_id != panel_number) && (point.sub_id != 0)))
	{
		if(point.panel == panel_number)  // remote points
		{
			if( ( index = find_remote_point( &point ) ) < 0 )
			{
				if( ( index = insert_remote_point( &point, -1 ) ) < 0 )
				{
					return -1;
				}
			}
			// modbus remote point
			if(check_point_type(&point) == 1)
			{
				U8_T point_type;
				U8_T flag;
				ptr = (REMOTE_POINTS *)(&remote_points_list_modbus[0]) + index;

				point_type = (ptr->point.point_type & 0x1f) + (ptr->point.network_number & 0x60);

				high_3bit = ptr->point.point_type >> 5;


				if((point_type == (VAR + 1)) \
				|| (point_type == (MB_IN_REG + 1))\
				|| (point_type == (MB_REG + 1)) \
				|| ((point_type >= BAC_FLOAT_ABCD + 1) &&( point_type <= BAC_FLOAT_DCBA + 1))
				)
				{
					if(ptr->point.network_number & 0x80)
						high_5bit = ptr->point.network_number & 0x1f;
					else
						high_5bit = 0;
					if(ptr->point_value != *val_ptr)
					{
						if((point_type >= BAC_FLOAT_ABCD + 1) &&( point_type <= BAC_FLOAT_DCBA + 1))
						{
							Float_to_Byte((float)(*val_ptr) / 1000,(U8_T *)&value,point_type - BAC_FLOAT_ABCD);
							write_parameters_to_nodes(0x10,remote_points_list_modbus[index].tb.RP_modbus.id,ptr->point.number + 256 * high_3bit + 2048 * high_5bit,(U16_T*)&value,4);
						}
						else
						{	value = *val_ptr / 1000;
							write_parameters_to_nodes(0x06,remote_points_list_modbus[index].tb.RP_modbus.id,ptr->point.number + 256 * high_3bit + 2048 * high_5bit,(U16_T*)&value,1);
						}
					}
				}

				if((point_type == (MB_COIL_REG + 1)) || (point_type == (MB_DIS_REG + 1)))
				{
					if(ptr->point_value != *val_ptr)
					{
						value = (float)*val_ptr / 1000; // write coil
						write_parameters_to_nodes(0x05,remote_points_list_modbus[index].tb.RP_modbus.id,ptr->point.number + 256 * high_3bit + 2048 * high_5bit,(U16_T*)&value,1);
					}
				}
				if(mode == 0)
					ptr->time_to_live = RM_TIME_TO_LIVE;
				else if(mode == 2)
					ptr->time_to_live = 60;
				else
					ptr->time_to_live = PERMANENT_LIVE;
//				ptr->decomisioned = 1;

				Last_Contact_Remote_points_modbus[index] = 0;

				ptr->instance = 0;
			}
			else
			{  // write remote bacnet points
				ptr = (REMOTE_POINTS *)(&remote_points_list_bacnet[0]) + index;
				// write MSTP remote point
		//value  = ptr->point_value;//temp[0] + (U16_T)(temp[1] << 8) + ((U32_T)temp[2] << 16) + ((U32_T)temp[3] << 24);
				if(ptr->point.point_type == BAC_AV + 1)
				{
					if(ptr->point_value != *val_ptr /*/ 1000*/)
					{

						WriteRemotePoint(OBJECT_ANALOG_VALUE,
									point.number,
									point.panel,
									point.sub_id ,
									(float)*val_ptr / 1000,
									BAC_MSTP);
						ptr->point_value = (*val_ptr);
					}
				}
				else if(ptr->point.point_type == BAC_AI + 1)
				{
					if(ptr->point_value != *val_ptr /*/ 1000*/)
					{
						//value = (float)*val_ptr / 1000;
						WriteRemotePoint(OBJECT_ANALOG_INPUT,
									point.number,
									point.panel,
									point.sub_id ,
									(float)*val_ptr / 1000,
									BAC_MSTP);
						ptr->point_value = (*val_ptr);
					}
				}
				else if(((ptr->point.point_type == OUT + 1) && (point.number >= max_dos))
					|| (ptr->point.point_type == BAC_AO + 1))
				{		// ptransfer output
					if(ptr->point_value != *val_ptr /*/ 1000*/)
					{
						if(ptr->point.point_type == BAC_AO + 1)
						{
							WriteRemotePoint(OBJECT_ANALOG_OUTPUT,
								point.number,
								point.panel,
								point.sub_id ,
								(float)*val_ptr / 1000,
								BAC_MSTP);
						}
						else
						{
							WriteRemotePoint(OBJECT_ANALOG_OUTPUT,
								point.number - max_dos,
								point.panel,
								point.sub_id ,
								(float)*val_ptr / 1000,
								BAC_MSTP);
						}
						ptr->point_value = (*val_ptr);
					}
				}
				else if(((ptr->point.point_type == OUT + 1) && (point.number < max_dos))
					|| (ptr->point.point_type == BAC_BO + 1))
				{		// ptransfer output
					if(ptr->point_value != *val_ptr)
					{
						WriteRemotePoint(OBJECT_BINARY_OUTPUT,
									point.number,
									point.panel,
									point.sub_id ,
									(float)*val_ptr / 1000,
									BAC_MSTP);
						ptr->point_value = (*val_ptr);
					}

				}
				if(ptr->point.point_type == BAC_BV + 1)
				{
					if(ptr->point_value != *val_ptr)
					{
						WriteRemotePoint(OBJECT_BINARY_VALUE,
									point.number,
									point.panel,
									point.sub_id ,
									(*val_ptr ? 1 : 0),
									BAC_MSTP);
						ptr->point_value = *val_ptr ;
					}
				}
				if(ptr->point.point_type == BAC_BI + 1)
				{
					if(ptr->point_value != *val_ptr)
					{

						WriteRemotePoint(OBJECT_BINARY_INPUT,
									point.number,
									point.panel,
									point.sub_id ,
									(*val_ptr ? 1 : 0),
									BAC_MSTP);
						ptr->point_value = (*val_ptr);
					}
				}
				// ...

				ptr->instance = Get_device_id_by_panel(point.panel,point.sub_id,BAC_MSTP);
			}
//			ptr->change = 1;
			if(mode == 0)
				ptr->time_to_live = RB_TIME_TO_LIVE;
			else
				ptr->time_to_live = PERMANENT_LIVE;

			Last_Contact_Remote_points_bacnet[index] = 0;
		}

		else  // network points
		{
// write network  points
			if( ( index = find_network_point( &point ) ) < 0 )
			{
				if( ( index = insert_network_point( &point, -1 ) ) < 0 ) return -1;
			}
#if  1//NETWORK_MODBUS

			if(check_point_type(&point) == 1)
			{// network modbus point
				U8_T point_type;
				ptr1 = (NETWORK_POINTS *)(&network_points_list_modbus[0]) + index;
				point_type = (ptr1->point.point_type & 0x1f) + (ptr1->point.network_number & 0x60);
				high_3bit = ptr1->point.point_type >> 5;

				if((point_type == (VAR + 1)) \
				|| (point_type == (MB_IN_REG + 1))\
				|| (point_type == (MB_REG + 1))
				|| (point_type == (MB_COIL_REG + 1))
				|| ((point_type >= BAC_FLOAT_ABCD + 1) &&( point_type <= BAC_FLOAT_DCBA + 1)))
				{
					if(ptr1->point.network_number & 0x80)
						high_5bit = ptr1->point.network_number & 0x1f;
					else
						high_5bit = 0;

					if(ptr1->point_value != *val_ptr)
					{
						if((point_type >= BAC_FLOAT_ABCD + 1) &&( point_type <= BAC_FLOAT_DCBA + 1))
						{
								Float_to_Byte(*val_ptr,(U8_T *)&value,point_type - BAC_FLOAT_ABCD);
								write_NP_Modbus_to_nodes(ptr1->point.panel,0x10,ptr1->point.sub_id,ptr1->point.number + 256 * high_3bit + 2048 * high_5bit,(U16_T*)&value,2);
						}
						else
						{
							value = *val_ptr / 1000;
							if(point_type == (MB_COIL_REG + 1))  // WRITE COIL
							{
								//network_points_list_modbus[index].invoked_id =
									write_NP_Modbus_to_nodes(ptr1->point.panel,0x05,ptr1->point.sub_id,ptr1->point.number + 256 * high_3bit + 2048 * high_5bit,(U16_T*)&value,1);
							}
							else
								//network_points_list_modbus[index].invoked_id =
									write_NP_Modbus_to_nodes(ptr1->point.panel,0x06,ptr1->point.sub_id,ptr1->point.number + 256 * high_3bit + 2048 * high_5bit,(U16_T*)&value,1);

						}
						// tbd:
						//network_points_list_modbus[index].invoked_id =
						//	write_NP_Modbus_to_nodes(ptr1->point.panel,0x06,ptr1->point.sub_id,ptr1->point.number + 256 * high_3bit + 2048 * high_5bit,value,1);
						ptr1->point_value = *val_ptr;
					}

				}
//				if(((ptr1->point.point_type & 0x1f) == (MB_COIL_REG + 1)) || ((ptr1->point.point_type & 0x1f) == (MB_DIS_REG + 1)))
//				{
//					if(ptr1->point_value != *val_ptr)
//					{
//						value = (float)*val_ptr / 1000; // write coil
//						//write_parameters_to_nodes(0x05,remote_points_list_modbus[index].tb.RP_modbus.id,ptr->point.number + high_3bit * 256,(U16_T*)&value,1);
//					}
//				}
				if(mode == 0)
					ptr1->time_to_live = RM_TIME_TO_LIVE;
				else if(mode == 2)
					ptr1->time_to_live = 60;
				else
					ptr1->time_to_live = PERMANENT_LIVE;

				Last_Contact_Network_points_modbus[index] = 0;
				ptr1->instance = 0;
			}
			else
#endif
			{
				ptr1 = (NETWORK_POINTS *)(&network_points_list_bacnet[0]) + index;

				if(point.sub_id == 0) // network point
					point.sub_id = point.panel;

				value = ptr1->point_value;

// write MSTP remote point
				if(ptr1->point.point_type == VAR + 1
					|| ptr1->point.point_type == IN + 1
					|| ptr1->point.point_type == OUT + 1)
				{
					if(value != *val_ptr)
					{
						uint16_t reg;
						uint8_t len;
						uint32 deviceid = Get_device_id_by_panel(point.panel,point.sub_id,BAC_IP_CLIENT);
						if(deviceid != 0)
						{
							reg = get_reg_from_list(ptr1->point.point_type,point.number,&len);
							WritePrivateBacnetToModbusData(deviceid,reg,len,(float)*val_ptr);
							ptr1->point_value = *val_ptr;
						}
					}

				}
				else 	if(ptr1->point.point_type == BAC_AV + 1)
				{
					if(value != *val_ptr)
					{
						WriteRemotePoint(OBJECT_ANALOG_VALUE,
									point.number/* + 1*/,
									point.panel,
									point.sub_id ,
									(float)*val_ptr / 1000,
									BAC_IP_CLIENT);
						ptr1->point_value = *val_ptr;
					}

				}
				else if(ptr1->point.point_type == BAC_AI + 1)
				{

				}
				else if(((ptr1->point.point_type == OUT + 1) && (point.number >= max_dos))
					|| (ptr1->point.point_type == BAC_AO + 1))
				{		// ptransfer output
					if(value != *val_ptr)
					{
						if(ptr1->point.point_type == BAC_AO + 1)
						{
							WriteRemotePoint(OBJECT_ANALOG_OUTPUT,
								point.number/* + 1*/,
								point.panel,
								point.sub_id ,
								(float)*val_ptr / 1000,
								BAC_IP_CLIENT);

						}
						else
						{
							WriteRemotePoint(OBJECT_ANALOG_OUTPUT,
								point.number - max_dos /* + 1*/,
								point.panel,
								point.sub_id ,
								(float)*val_ptr / 1000,
								BAC_IP_CLIENT);
						}
						ptr1->point_value = *val_ptr;
					}
				}
				else if(((ptr1->point.point_type == OUT + 1) && (point.number < max_dos))
					|| (ptr1->point.point_type == BAC_BO + 1))
				{		// ptransfer output
					if(value != *val_ptr)
					{
						WriteRemotePoint(OBJECT_BINARY_OUTPUT,
									point.number/* + 1*/,
									point.panel,
									point.sub_id ,
									(float)*val_ptr / 1000,
									BAC_IP_CLIENT);
						ptr1->point_value = *val_ptr;
					}

				}
				else if(ptr1->point.point_type == IN + 1)
				{  	// ptransfer input

				}

				ptr1->instance = Get_device_id_by_panel(point.panel,point.sub_id,BAC_IP_CLIENT);

				//vTaskDelay( 100 / portTICK_RATE_MS);

				if(mode == 0)
					ptr1->time_to_live = NP_TIME_TO_LIVE;
				else
					ptr1->time_to_live = PERMANENT_LIVE;

				Last_Contact_Network_points_bacnet[index] = 0;
			}
		}

	}
	else
	{
// local points
		if(prog_op == 1)
		{
			put_point_value( (Point *)p, val_ptr, aux, prog_op );
		}
	}

	return 1;
}


/*
 * ----------------------------------------------------------------------------
 * Function Name: get_net_point_value
 * Purpose: according to the network number of the networt point, get point value of the remote points,
* Params: mode : 0 - not permanent, 1 - permanent
 * Returns:
 * Note: read remote points and network points
 * ----------------------------------------------------------------------------
 */
// mode -- 0 is RM_TIME_TO_LIVE, 1 is PERMANENT_LIVE
// flag -- 1 is grahpic, 0 is other

S16_T get_net_point_value( Point_Net *p, S32_T *val_ptr , U8_T mode,U8_T flag)
{

	S16_T index;
	Point_Net point;
	S32_T tempval = 0;

	memcpy( &point, p, sizeof(Point_Net));

	get_point_info_by_instacne(&point);

	if(point.panel == 0)
	{
		return -1;
	}
	if(/*( point.network_number != Setting_Info.reg.network_number) ||*/( point.panel != panel_number)
		|| ((point.sub_id != panel_number) && (point.sub_id != 0)))
	{
		if( point.panel == panel_number)  // remote points
		{
		 // remote points
			
			if( ( index = find_remote_point( &point ) ) < 0 )
			{
				if( ( index = insert_remote_point( &point, -1 ) ) < 0 )
					return -1;
			}
			if(index < MAXREMOTEPOINTS)
			{
				if(check_point_type(&point) == 1)
				{
					remote_points_list_modbus[index].instance = 0;
					if(remote_points_list_modbus[index].point_value != DEFUATL_REMOTE_NETWORK_VALUE)
					{
						*val_ptr = (remote_points_list_modbus[index].point_value);

						if(mode == 0)
							remote_points_list_modbus[index].time_to_live = RM_TIME_TO_LIVE;
						else
							remote_points_list_modbus[index].time_to_live = PERMANENT_LIVE;

						Last_Contact_Remote_points_modbus[index] = 0;
					}
					else
					{
						if(mode == 0)
							remote_points_list_modbus[index].time_to_live = RM_TIME_TO_LIVE;
						else
							remote_points_list_modbus[index].time_to_live = PERMANENT_LIVE;

						Last_Contact_Remote_points_modbus[index] = 0;
						*val_ptr = 0;
						return 0;
					}
				}

				else		// remote bacnet points
				{	
					remote_points_list_bacnet[index].instance =
					Get_device_id_by_panel(remote_points_list_bacnet[index].point.panel,remote_points_list_bacnet[index].point.sub_id,BAC_MSTP);
					if(remote_points_list_bacnet[index].point_value != DEFUATL_REMOTE_NETWORK_VALUE)
					{						
						*val_ptr = remote_points_list_bacnet[index].point_value;						
						if(mode == 0)
							remote_points_list_bacnet[index].time_to_live = RB_TIME_TO_LIVE;
						else
							remote_points_list_bacnet[index].time_to_live = PERMANENT_LIVE;
//						remote_points_list_bacnet[index].decomisioned = 1;
						Last_Contact_Remote_points_bacnet[index] = 0;
					}
					else
					{
						*val_ptr = 0;
						return 0;
					}
				}

			}

		}
#if 1//(ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI )

		else  // network points
		{
			if(flag == 1)  // enter is graphic, ingore network bacnet points
				return -1;
			if( ( index = find_network_point( &point ) ) < 0 )
			{
				if( ( index = insert_network_point( &point, -1 ) ) < 0 )
					return -1;
			}
			if(index < MAXNETWORKPOINTS)
			{
#if 1//NETWORK_MODBUS
				if(check_point_type(&point) == 1)  // modbus points
				{
					network_points_list_modbus[index].instance = 0;

					if(network_points_list_modbus[index].point_value != DEFUATL_REMOTE_NETWORK_VALUE)
					{
						*val_ptr = network_points_list_modbus[index].point_value;

						if(mode == 0)
							network_points_list_modbus[index].time_to_live = NP_TIME_TO_LIVE;
						else
							network_points_list_modbus[index].time_to_live = PERMANENT_LIVE;

						Last_Contact_Network_points_modbus[index] = 0;
					}
					else
					{
						network_points_list_modbus[index].time_to_live = NP_TIME_TO_LIVE;
						Last_Contact_Network_points_modbus[index] = 0;
						*val_ptr = 0;
						return 0;
					}
				}
				else
#endif
				{
					//*val_ptr = network_points_list_bacnet[index].point_value;
					network_points_list_bacnet[index].instance =
					Get_device_id_by_panel(network_points_list_bacnet[index].point.panel,network_points_list_bacnet[index].point.sub_id,BAC_IP_CLIENT);

					if(mode == 0)
						network_points_list_bacnet[index].time_to_live = NP_TIME_TO_LIVE;
					else
						network_points_list_bacnet[index].time_to_live = PERMANENT_LIVE;

					Last_Contact_Network_points_bacnet[index] = 0;

					if(network_points_list_bacnet[index].point_value != DEFUATL_REMOTE_NETWORK_VALUE)
					{
						if(network_points_list_bacnet[index].point_value == 0 )
						{
							return 0;
						}
						*val_ptr = network_points_list_bacnet[index].point_value;
						if(mode == 0)
							network_points_list_bacnet[index].time_to_live = NP_TIME_TO_LIVE;
						else
							network_points_list_bacnet[index].time_to_live = PERMANENT_LIVE;

						Last_Contact_Network_points_bacnet[index] = 0;
					}
					else
					{
						*val_ptr = 0;//DEFUATL_REMOTE_NETWORK_VALUE;
						return 0;
					}
				}
			}
		}

#endif
	}
	else
	{ // local points

		get_point_value( (Point *)p, &tempval );
		*val_ptr = tempval;
	}

	return 1;
}




void initial_graphic_point(void)
{
	U8_T i;
	Point_Net point;
	S32_T value;
	for(i = 0;i < MAX_ELEMENTS;i++)
	{
		if(group_data[i].reg.label_status == 0)
			break;
		point.number = group_data[i].reg.nPoint_number;
		point.point_type = group_data[i].reg.nPoint_type;
		point.panel = group_data[i].reg.nMain_Panel;
		point.sub_id = group_data[i].reg.nSub_Panel;
		point.network_number = 0;//Setting_Info.reg.network_number;

		get_net_point_value(&point,&value,1,1);

	}

}


#endif //BAC_POINT


#if 1//MONITOR

#define IN_SVAR_SAMPLE 20


U8_T SD_exist = 0; // dont use SD card in ESP
U8_T Read_Picture_from_SD(U8_T file,U16_T index);
//void update_comport_health(void);
//U32_T 				 		SD_block_num[MAX_MONITORS * 2];

Str_mon_element          read_mon_point_buf[MAX_MON_POINT];
Str_mon_element          write_mon_point_buf[MAX_MONITORS * 2][MAX_MON_POINT];

Monitor_Block					mon_block[2 * MAX_MONITORS];

//void monitor_reboot(void);
U8_T Get_start_end_packet_by_time(U8_T file_index,U32_T start_time,U32_T end_time, U32_T * start_seg, U32_T * total_seg,U32_T block_no)
{
	// tbd:?????????????
	if(SD_exist != 2)
	{
		*start_seg = 0;
		*total_seg = 1;
		return 0;
	}
	return 1;
}




void init_new_analog_block( int mon_number, Str_monitor_point *mon_ptr/*, Monitor_Block *block_ptr*/ )
{
	Ulong sample_time;

	sample_time = mon_ptr->hour_interval_time * 3600L;
	sample_time += mon_ptr->minute_interval_time * 60;
	sample_time += mon_ptr->second_interval_time;

	memcpy( (void*)mon_block[mon_number * 2].inputs, (void*)mon_ptr->inputs,
			                mon_ptr->an_inputs*sizeof(Point_Net)  );

	mon_block[mon_number * 2].second_interval_time = mon_ptr->second_interval_time;
	mon_block[mon_number * 2].minute_interval_time = mon_ptr->minute_interval_time;
	mon_block[mon_number * 2].hour_interval_time = mon_ptr->hour_interval_time;
	mon_block[mon_number * 2].monitor = mon_number;
	mon_block[mon_number * 2].no_points = mon_ptr->an_inputs;
	mon_block[mon_number * 2].start_time = (U32_T)get_current_time() / sample_time;
	mon_block[mon_number * 2].start_time++;
	mon_block[mon_number * 2].start_time *= sample_time;
	mon_block[mon_number * 2].index = 0;
}


void init_new_digital_block( int mon_number, Str_monitor_point *mon_ptr/*,Monitor_Block *block_ptr */)
{
	memcpy( (void*)mon_block[mon_number * 2 + 1].inputs, (void*)(mon_ptr->inputs + mon_ptr->an_inputs) , \
		(mon_ptr->num_inputs - mon_ptr->an_inputs) *sizeof(Point_Net)  );


	mon_block[mon_number * 2 + 1].monitor = mon_number;
    mon_block[mon_number * 2 + 1].no_points = mon_ptr->num_inputs - mon_ptr->an_inputs;
	mon_block[mon_number * 2 + 1].index = 0;

}


/*=========================================================================*/
void sample_analog_points(char i, Str_monitor_point *mon_ptr/*, Mon_aux  *aux_ptr*/ )
{
	int j,k;
	Ulong time;
	Str_points_ptr ptr;
	S32_T value;

	U8_T temp[4];



	time = 3600L * mon_ptr->hour_interval_time;
	time += 60 * mon_ptr->minute_interval_time;
	time += mon_ptr->second_interval_time;
	mon_ptr->next_sample_time += time;

	for(j = 0; j < mon_block[i * 2].no_points;j++)
	{
		ptr.pnet = mon_block[i * 2].inputs + j;
		if( !mon_block[i * 2].index )
		{
			mon_block[i * 2].start_time = get_current_time();
		}

		write_mon_point_buf[i * 2][mon_block[i * 2].index].index = mon_block[i * 2].monitor;

		write_mon_point_buf[i * 2][mon_block[i * 2].index].point.number = ptr.pnet->number;
		write_mon_point_buf[i * 2][mon_block[i * 2].index].point.point_type = ptr.pnet->point_type;
		write_mon_point_buf[i * 2][mon_block[i * 2].index].point.panel = ptr.pnet->panel;
		write_mon_point_buf[i * 2][mon_block[i * 2].index].point.sub_id = ptr.pnet->sub_id;
		write_mon_point_buf[i * 2][mon_block[i * 2].index].point.network_number = ptr.pnet->network_number;


		temp[0] = get_current_time();
		temp[1] = get_current_time() >> 8;
		temp[2] = get_current_time() >> 16;
		temp[3] = get_current_time() >> 24;

		write_mon_point_buf[i * 2][mon_block[i * 2].index].time = temp[3] + (U16_T)(temp[2] << 8)
	 + ((U32_T)temp[1] << 16) + ((U32_T)temp[0] << 24);

		write_mon_point_buf[i * 2][mon_block[i * 2].index].mark = 0x0a0d;

		if(get_net_point_value( ptr.pnet, &value,0,0 ) == 1)  // get rid of default value 0x55555555
		{
			write_mon_point_buf[i * 2][mon_block[i * 2].index].value =  value;

			temp[0] = (write_mon_point_buf[i * 2][mon_block[i * 2].index].value);
			temp[1] = (write_mon_point_buf[i * 2][mon_block[i * 2].index].value) >> 8;
			temp[2] = (write_mon_point_buf[i * 2][mon_block[i * 2].index].value) >> 16;
			temp[3] = (write_mon_point_buf[i * 2][mon_block[i * 2].index].value) >> 24;

			write_mon_point_buf[i * 2][mon_block[i * 2].index].value = temp[3] + (U16_T)(temp[2] << 8)
						+ ((U32_T)temp[1] << 16) + ((U32_T)temp[0] << 24);

			mon_block[i * 2].index++;

		}

	}


	if(mon_block[i * 2].index +  mon_block[i * 2].no_points > MAX_MON_POINT)
	{
		// current buffer is full, save data to SD card.
		// clear no used point
		for(k = mon_block[i * 2].index;k < MAX_MON_POINT;k++)
		{
			memset(&write_mon_point_buf[i * 2][k],0,sizeof(Str_mon_element));
		}
#if STORE_TO_SD
		if(Write_SD(((SD_block_num[i * 2]) >> 8 & 0xfff),i,1,(U32_T)LOW_BYTE(SD_block_num[i * 2]) * MAX_MON_POINT * sizeof(Str_mon_element)) == 1)  // write ok
#endif
		{
			/*if(SD_exist == 2) // exist ???????????????
			{
				U8_T temp;
				//E2prom_Read_Byte(EEP_SD_BLOCK_HI1 + i,&temp);
				if((temp & 0x0f) != (SD_block_num[i * 2] >> 16 & 0x0f))
				{
					temp &= 0xf0;
					temp |= (SD_block_num[i * 2] >> 16 & 0x0f);
					//E2prom_Write_Byte(EEP_SD_BLOCK_HI1 + i,temp);

				}
				//E2prom_Write_Byte(EEP_SD_BLOCK_A1 + i * 2,HIGH_BYTE(SD_block_num[i * 2]));
				//E2prom_Write_Byte(EEP_SD_BLOCK_A1 + i * 2 + 1,LOW_BYTE(SD_block_num[i * 2]));
				if(SD_block_num[i * 2] < 0xfffff)
					SD_block_num[i * 2]++;
				else
				{
					//E2prom_Write_Byte(EEP_SD_BLOCK_A1 + i * 2,0);
					//E2prom_Write_Byte(EEP_SD_BLOCK_A1 + i * 2 + 1,0);
					//E2prom_Write_Byte(EEP_SD_BLOCK_HI1 + i,temp & 0xf0);
					SD_block_num[i * 2] = 0;
				}
			}*/
		}
		init_new_analog_block( i, mon_ptr);
	}

}


U8_T  last_digital_index[12];
U32_T  last_digital_sample_time[12];
void sample_digital_points( U8_T i,Str_monitor_point *mon_ptr/*, Mon_aux *aux_ptr*/ )
{
	int j,k;//,A,B;
	S32_T value;
	Str_points_ptr ptr;
	static U8_T  first = 0;
//	static U16_T  count_write_sd = 0;
    U8_T temp[4];
//	monitor_reboot();
//	if(SD_exist == 2)
//		count_write_sd++;

	if( !mon_block[i * 2 + 1].index )
	{
		mon_block[i * 2 + 1].start_time = get_current_time();
	}

	for( j = 0; j < mon_block[i * 2 + 1].no_points; j++ )
	{
		if(get_net_point_value( mon_block[i * 2 + 1].inputs + j, &value,1,0 ) == 1)
		{
			// store the points changed
			ptr.pnet = mon_block[i * 2 + 1].inputs + j;

			if(get_current_time() >= last_digital_sample_time[i] + 250l)   // record digital points if no change in 250s
			{
				if(j == mon_block[i * 2 + 1].no_points - 1)
					last_digital_sample_time[i] = get_current_time();
				// 1 min exceed, store all digital points
					write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].index = mon_block[i * 2 + 1].monitor;
					write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.number = ptr.pnet->number;
					write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.point_type = ptr.pnet->point_type;
					write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.panel = ptr.pnet->panel;
					write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.sub_id = ptr.pnet->sub_id;
					write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.network_number = ptr.pnet->network_number;

					if(value)
					{
							mon_block[i * 2 + 1].last_digital_state |= (1<<j);


					temp[0] = get_current_time();
					temp[1] = get_current_time() >> 8;
					temp[2] = get_current_time() >> 16;
					temp[3] = get_current_time() >> 24;

					write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].time = ((U32_T)temp[0] << 24) + ((U32_T)temp[1] << 16)\
					+ ((U16_T)temp[2] << 8) + temp[3];
					write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].value = 0x01000000;
					write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].mark = 0x0a0d;


					}
					else
					{

					temp[0] = get_current_time();
					temp[1] = get_current_time() >> 8;
					temp[2] = get_current_time() >> 16;
					temp[3] = get_current_time() >> 24;

					write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].time = ((U32_T)temp[0] << 24) + ((U32_T)temp[1] << 16)\
					+ ((U16_T)temp[2] << 8) + temp[3];
					write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].value = 0;
					write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].mark = 0x0a0d;

					}
					mon_block[i * 2 + 1].index++;
			}
			else

			{
//					if(first == 0)
//					{
//						first = 1;
//					}
//					else
					{
						if( 1 << j & mon_block[i * 2 + 1].last_digital_state )
						{
							if(!value)
							{

								mon_block[i * 2 + 1].last_digital_state &= ~(1<<j);
								write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].index = mon_block[i * 2 + 1].monitor;

								temp[0] = get_current_time();
								temp[1] = get_current_time() >> 8;
								temp[2] = get_current_time() >> 16;
								temp[3] = get_current_time() >> 24;

								write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].time = ((U32_T)temp[0] << 24) + ((U32_T)temp[1] << 16)\
										+ ((U16_T)temp[2] << 8) + temp[3];
								write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].mark = 0x0a0d;



								write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.number = ptr.pnet->number;
								write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.point_type = ptr.pnet->point_type;
								write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.panel = ptr.pnet->panel;
								write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.sub_id = ptr.pnet->sub_id;
								write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.network_number = ptr.pnet->network_number;
								write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].value = 0;//get_input_sample( ptr.pnet->number );
								mon_block[i * 2 + 1].index++;
							}
							else
							{
									mon_block[i * 2 + 1].last_digital_state |= (1<<j);

									if(first == 0 )
									{
										first = 1;
										write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].index = mon_block[i * 2 + 1].monitor;
										write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.number = ptr.pnet->number;
										write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.point_type = ptr.pnet->point_type;
										write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.panel = ptr.pnet->panel;
										write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.sub_id = ptr.pnet->sub_id;
										write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.network_number = ptr.pnet->network_number;



										temp[0] = get_current_time();
										temp[1] = get_current_time() >> 8;
										temp[2] = get_current_time() >> 16;
										temp[3] = get_current_time() >> 24;

										write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].time = ((U32_T)temp[0] << 24) + ((U32_T)temp[1] << 16)\
										+ ((U16_T)temp[2] << 8) + temp[3];
										write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].value = 0x01000000;
										write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].mark = 0x0a0d;

										mon_block[i * 2 + 1].index++;
									}

							}
						}
						else
						{
							if( value)
							{
								mon_block[i * 2 + 1].last_digital_state |= (1<<j);
								write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].index = mon_block[i * 2 + 1].monitor;
								write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.number = ptr.pnet->number;
								write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.point_type = ptr.pnet->point_type;
								write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.panel = ptr.pnet->panel;
								write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.sub_id = ptr.pnet->sub_id;
								write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.network_number = ptr.pnet->network_number;


								temp[0] = get_current_time();
								temp[1] = get_current_time() >> 8;
								temp[2] = get_current_time() >> 16;
								temp[3] = get_current_time() >> 24;

								write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].time = ((U32_T)temp[0] << 24) + ((U32_T)temp[1] << 16)\
								+ ((U16_T)temp[2] << 8) + temp[3];
								write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].value = 0x01000000;
								write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].mark = 0x0a0d;

								mon_block[i * 2 + 1].index++;
							}
							else
							{
								mon_block[i * 2 + 1].last_digital_state &= ~(1<<j);
								if(first == 0 )
								{
										// first time or
										// if digial value keep same for 1 minute, still record
									first = 1;

									write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].index = mon_block[i * 2 + 1].monitor;
									write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.number = ptr.pnet->number;
									write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.point_type = ptr.pnet->point_type;
									write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.panel = ptr.pnet->panel;
									write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.sub_id = ptr.pnet->sub_id;
									write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].point.network_number = ptr.pnet->network_number;
									write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].value = 0;//get_input_sample( ptr.pnet->number );



									temp[0] = get_current_time();
									temp[1] = get_current_time() >> 8;
									temp[2] = get_current_time() >> 16;
									temp[3] = get_current_time() >> 24;

									write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].time = ((U32_T)temp[0] << 24) + ((U32_T)temp[1] << 16)\
									+ ((U16_T)temp[2] << 8) + temp[3];
									write_mon_point_buf[i * 2 + 1][mon_block[i * 2 + 1].index].mark = 0x0a0d;

									mon_block[i * 2 + 1].index++;
								}
							}

						}
					}
				}
		}


  }

	// exceed 1 min, if packet is not full
		if( (mon_block[i * 2 + 1].index + mon_block[i * 2 + 1].no_points > MAX_MON_POINT) /*|| \
			( (count_write_sd >= 300) && (last_digital_index[i] != mon_block[i * 2 + 1].index))*/ )
  	{
//			count_write_sd = 0;
			for(k = mon_block[i * 2 + 1].index;k < MAX_MON_POINT;k++)
			{
				memset(&write_mon_point_buf[i * 2 + 1][k],0,sizeof(Str_mon_element));
			}
#if STORE_TO_SD
			if(Write_SD(HIGH_BYTE(SD_block_num[i * 2 + 1]) + ((SD_block_num[i * 2 + 1] >> 24) << 16),i,0,(U32_T)LOW_BYTE(SD_block_num[i * 2 + 1]) * MAX_MON_POINT * sizeof(Str_mon_element))==1)
#endif
			{
				/*if(SD_exist == 2)
				{
					U8_T temp;
					//E2prom_Read_Byte(EEP_SD_BLOCK_HI1 + i,&temp);  // high 4 bits
					if((temp & 0xf0) != (SD_block_num[i * 2] >> 16 & 0xf0))
					{
						temp &= 0x0f;
						temp |= (SD_block_num[i * 2] >> 16 & 0xf0);
						//E2prom_Write_Byte(EEP_SD_BLOCK_HI1 + i,temp);

					}
					//E2prom_Write_Byte(EEP_SD_BLOCK_D1 + i * 2,HIGH_BYTE(SD_block_num[i * 2 + 1]));
					//E2prom_Write_Byte(EEP_SD_BLOCK_D1 + i * 2 + 1,LOW_BYTE(SD_block_num[i * 2 + 1]));
					if(SD_block_num[i * 2 + 1] < 0xfffff)
						SD_block_num[i * 2 + 1] ++;
					else
					{
						//E2prom_Write_Byte(EEP_SD_BLOCK_D1 + i * 2,0);
						//E2prom_Write_Byte(EEP_SD_BLOCK_D1 + i * 2 + 1,0);
						//E2prom_Write_Byte(EEP_SD_BLOCK_HI1 + i,temp & 0x0f);
						SD_block_num[i * 2 + 1] = 0;
					}
				}*/

			}

//			if(mon_block[i * 2 + 1].index + mon_block[i * 2 + 1].no_points > MAX_MON_POINT)
					init_new_digital_block( i, mon_ptr);
		}
		last_digital_index[i] = mon_block[i * 2 + 1].index;

}


/*======================================================================*/
void sample_points( void  )
{
	int i;

	Str_monitor_point *mon_ptr;
	mon_ptr = monitors;

	for( i = 0; i < MAX_MONITORS; i++, mon_ptr++)
	{
		if((mon_ptr->status > 0) /*&& (max_monitor_time[i] > 0)*/)
		{
			//if(count_max_time[i] > 0)
			{
				//count_max_time[i]--;
				if( mon_ptr->an_inputs > 0)
				{
					if( mon_ptr->next_sample_time <= get_current_time() )
					{
						sample_analog_points(i, mon_ptr);
					}
				}
				if( mon_ptr->num_inputs - mon_ptr->an_inputs > 0)
				{
					sample_digital_points(i, mon_ptr);

				}
			}
//			else
//			{
//				monitors[i].status = 0;
//			}
		}
	}
}

//void check_whehter_reading_sd(void)
//{
//	if(reading_sd)
//	{
//		count_reading_sd++;
//		if(count_reading_sd > 5)  // keep reaading flag 5s
//		{
//			count_reading_sd = 0;
//			reading_sd = 0;
//		}
//	}
//
//}


U8_T ReadMonitor( Mon_Data *PTRtable)
{
	U8_T ana_dig;
	U32_T block_no;
	U8_T ret;

	ret = 1;
//	reading_sd = 1;
//	count_reading_sd = 0;

	ana_dig = PTRtable->sample_type;
	if(ana_dig == 1)
	{
		block_no = 0;//SD_block_num[PTRtable->index * 2] + 1;
	}
	else
	{
		block_no = 0;//SD_block_num[PTRtable->index * 2 + 1] + 1;
	}
	if(SD_exist != 2) // inexist  ?????????????????
	{
		block_no = 1;
	}
// current packet from T3000

	if(PTRtable->seg_index > 0)
	{
		if(SD_exist != 2) // inexist
		{ // check if no sd card, whether currect data should be stored into PC's DB
			PTRtable->special = 0;
		}
		else
		{
			if(PTRtable->seg_index < block_no)
			{
				PTRtable->special = 0;
			}
			else
			{
				PTRtable->special = 1;
			}
		}
		if((PTRtable->seg_index < block_no) && (PTRtable->seg_index >= 1))
		{
#if STORE_TO_SD
			if(Read_SD(((PTRtable->seg_index - 1) >> 8) & 0xfff,PTRtable->index,ana_dig,(U32_T)LOW_BYTE(PTRtable->seg_index - 1) * MAX_MON_POINT * sizeof(Str_mon_element)) == 1)   // analog data
#endif
			{	 // read ok
				memset( PTRtable->asdu,0,MAX_MON_POINT * sizeof(Str_mon_element));
				memcpy( PTRtable->asdu,&read_mon_point_buf, MAX_MON_POINT * sizeof(Str_mon_element));
			}
#if STORE_TO_SD
			else  // else read error
#endif
				ret = 0;

		}
		else // read last packet, not store into SD
		{
				ret = 1;
				if(ana_dig == 1) // an
				{
					memset( PTRtable->asdu,0,MAX_MON_POINT * sizeof(Str_mon_element));
					memcpy( PTRtable->asdu,&write_mon_point_buf[PTRtable->index * 2], mon_block[PTRtable->index * 2].index * sizeof(Str_mon_element));
					if(SD_exist != 2) // inexist
						mon_block[PTRtable->index * 2].index = 0;
				}
				else  // digital
				{
					memset( PTRtable->asdu,0,MAX_MON_POINT * sizeof(Str_mon_element));
					memcpy( PTRtable->asdu,&write_mon_point_buf[PTRtable->index * 2 + 1], mon_block[PTRtable->index * 2 + 1].index * sizeof(Str_mon_element));
					if(SD_exist != 2) // inexist
					{
						mon_block[PTRtable->index * 2 + 1].index = 0;
					}
				}
		}
	}
//	else
//	{
//		memset( PTRtable->asdu,0,MAX_MON_POINT * sizeof(Str_mon_element));
//	}

	if(PTRtable->seg_index == 0 && PTRtable->total_seg == 0)
	{
		if(PTRtable->sample_type == 1) // analog
			Get_start_end_packet_by_time(PTRtable->index * 2,PTRtable->comm_arg.monupdate.oldest_time,PTRtable->comm_arg.monupdate.most_recent_time,&PTRtable->seg_index,&PTRtable->total_seg,block_no);
		else
			Get_start_end_packet_by_time(PTRtable->index * 2 + 1,PTRtable->comm_arg.monupdate.oldest_time,PTRtable->comm_arg.monupdate.most_recent_time,&PTRtable->seg_index,&PTRtable->total_seg,block_no);

	}



	return ret;

}


void check_monitor_sample_points(U8_T i)
{
	U8_T j,k;

	monitors[i].num_inputs = 0;
	monitors[i].an_inputs = 0;

	for(j = 0;j < MAX_POINTS_IN_MONITOR;j++)
	{
// 0.0. --> invalid points
		if(monitors[i].inputs[j].panel != 0)
		{
			monitors[i].num_inputs++;
			k = monitors[i].inputs[j].number;
			if(monitors[i].inputs[j].sub_id == panel_number || monitors[i].inputs[j].sub_id == 0)
			{ // Local points
				if(monitors[i].inputs[j].point_type == IN + 1)
				{
					monitors[i].range[j] = inputs[k].range;
					if(inputs[k].digital_analog == 1)
					{
						monitors[i].an_inputs++;
					}

				}
				else if(monitors[i].inputs[j].point_type == OUT + 1)
				{
					monitors[i].range[j] = outputs[k].range;
					if(outputs[k].digital_analog == 1)
					{
						monitors[i].an_inputs++;
					}
				}
				else if(monitors[i].inputs[j].point_type == VAR + 1)
				{
					monitors[i].range[j] = vars[k].range;
					if(vars[k].digital_analog == 1)
					{
						monitors[i].an_inputs++;
					}
				}
			}
			else
			{ // remote points or network points
					monitors[i].an_inputs++;
			}
		}
	}
}





void monitor_init(void)
{
	U8_T i;
//	U8_T far time_unit;
//	reading_sd = 0;
//  count_reading_sd = 0;
/*	monitors[0].status = 1;
	monitors[0].an_inputs = 2;
	monitors[0].num_inputs = 2;
	monitors[0].inputs[0].point_type = VAR + 1;
	monitors[0].inputs[0].number = 0;
	monitors[0].inputs[0].panel = panel_number;
	monitors[0].inputs[0].sub_id = panel_number;
	monitors[0].inputs[0].network_number = 0;

	monitors[0].inputs[1].point_type = VAR + 1;
	monitors[0].inputs[1].number = 1;
	monitors[0].inputs[1].panel = panel_number;
	monitors[0].inputs[1].sub_id = panel_number;
	monitors[0].inputs[1].network_number = 0;
	monitors[0].range[0] = 31;
	monitors[0].range[1] = 31;

	monitors[0].hour_interval_time = 0;
	monitors[0].minute_interval_time = 0;
	monitors[0].second_interval_time = 5;
*/

	for(i = 0;i < MAX_MONITORS - 1;i++)
	{
		if(monitors[i].status == 1)
		{  // enalble monitor
			check_monitor_sample_points(i);
			if((monitors[i].second_interval_time != 0) ||
				(monitors[i].minute_interval_time != 0) ||
				(monitors[i].hour_interval_time != 0) )
			{  // sample time is not 0

					if(monitors[i].num_inputs)
					{// check number of inputs
							if(monitors[i].an_inputs > 0)
							{ // exist analog inputs
								init_new_analog_block(i,&monitors[i]);
							}
							if(monitors[i].num_inputs - monitors[i].an_inputs)
							{// exist digital inputs
								init_new_digital_block(i,&monitors[i]);
							}
					}
			}

		}

//	  time_unit = (monitors[i].max_time & 0xc0) >> 6;
//
//		if(time_unit == 0) // sec
//			max_monitor_time[i] = monitors[i].max_time & 0x3f;
//		else if(time_unit == 1) // min
//			max_monitor_time[i] = 60L * (monitors[i].max_time & 0x3f);
//		else if(time_unit == 2) // hour
//			max_monitor_time[i] = 3600L * (monitors[i].max_time & 0x3f);
//		else if(time_unit == 3) // day
//			max_monitor_time[i] = 3600L * 24L * (monitors[i].max_time & 0x3f);
//
//		count_max_time[i] = max_monitor_time[i];

		monitors[i].next_sample_time = get_current_time();
		memcpy(&backup_monitors[i],&monitors[i], sizeof(Str_monitor_point));

	}

	// fix monitor1 for store network health
	memcpy(monitors[NET_HEALTH_INDEX].label,"NETWORK ",9);


	for(i = 0;i < 8;i++)
	{
		monitors[NET_HEALTH_INDEX].inputs[i].number = i + NET_HEALTH_VAR_BASE;
		monitors[NET_HEALTH_INDEX].inputs[i].point_type = VAR + 1;
		monitors[NET_HEALTH_INDEX].inputs[i].panel = panel_number;
		monitors[NET_HEALTH_INDEX].inputs[i].sub_id = panel_number;
		monitors[NET_HEALTH_INDEX].inputs[i].network_number = 0;
		monitors[NET_HEALTH_INDEX].range[i] = HI_spd_count;
		vars[NET_HEALTH_VAR_BASE + i].range = HI_spd_count;
		vars[NET_HEALTH_VAR_BASE + i].digital_analog = 1;
		vars[NET_HEALTH_VAR_BASE + i].value = 0;
	}

	// BOOT
	monitors[NET_HEALTH_INDEX].inputs[8].number = 8 + NET_HEALTH_VAR_BASE;
	monitors[NET_HEALTH_INDEX].inputs[8].point_type = VAR + 1;
	monitors[NET_HEALTH_INDEX].inputs[8].panel = panel_number;
	monitors[NET_HEALTH_INDEX].inputs[8].sub_id = panel_number;
	monitors[NET_HEALTH_INDEX].inputs[8].network_number = 0;
	monitors[NET_HEALTH_INDEX].range[8] = OFF_ON;
	vars[NET_HEALTH_VAR_BASE + 8].range = OFF_ON;
	vars[NET_HEALTH_VAR_BASE + 8].digital_analog = 0;
	vars[NET_HEALTH_VAR_BASE + 8].control = 0;


	monitors[NET_HEALTH_INDEX].second_interval_time = IN_SVAR_SAMPLE;
	monitors[NET_HEALTH_INDEX].minute_interval_time = 0;
	monitors[NET_HEALTH_INDEX].hour_interval_time = 0;
	monitors[NET_HEALTH_INDEX].max_time = 0x81; // 1hour
	monitors[NET_HEALTH_INDEX].num_inputs = 9;
	monitors[NET_HEALTH_INDEX].an_inputs = 8;
	monitors[NET_HEALTH_INDEX].status = 0;  // only for test

	monitors[NET_HEALTH_INDEX].next_sample_time = get_current_time();

	init_new_analog_block(NET_HEALTH_INDEX,&monitors[NET_HEALTH_INDEX]);
	init_new_digital_block(NET_HEALTH_INDEX,&monitors[NET_HEALTH_INDEX]);


	boot = 0;

}

#if TEST
U32_T far com_rx[3] = {0 , 0, 0};
U32_T far com_tx[3] = {0 , 0, 0};

void update_comport_health(void)
{
	static U32_T far com_rx_back[3] = {0 , 0, 0};
	static U32_T far com_tx_back[3] = {0 , 0, 0};
	static U32_T far etr_rx = 0;
	static U32_T far etr_tx = 0;
	static U8_T far count = 0;
	U8_T i;


	if(count < IN_SVAR_SAMPLE)
	{
		count++;
		return;
	}
	else
		count = 0;

	if(monitors[NET_HEALTH_INDEX].status == 0)
		return;

	for(i = 0;i < 3;i++)
	{
		if(((com_tx[i] - com_tx_back[i]) < 1000) && (com_tx[i] > com_tx_back[i]))
			vars[NET_HEALTH_VAR_BASE + i * 2].value = ((com_tx[i] - com_tx_back[i]) * 1000l / IN_SVAR_SAMPLE);

		if(((com_rx[i] - com_rx_back[i]) < 1000) && (com_rx[i] > com_rx_back[i]))
			vars[NET_HEALTH_VAR_BASE + i * 2 + 1].value = ((com_rx[i] - com_rx_back[i]) * 1000l/ IN_SVAR_SAMPLE);

		if(com_rx_back[i] != com_rx[i])
			com_rx_back[i] = com_rx[i];
		if(com_tx_back[i] != com_tx[i])
			com_tx_back[i] = com_tx[i];
	}


	/*if(((ether_rx_packet - etr_rx) < 1000) && (ether_rx_packet > etr_rx))
			vars[NET_HEALTH_VAR_BASE + 6].value = ((ether_rx_packet - etr_rx) * 1000l / IN_SVAR_SAMPLE);

	if(((ether_tx_packet - etr_tx) < 1000) && (ether_tx_packet > etr_tx))
		vars[NET_HEALTH_VAR_BASE + 7].value = ((ether_tx_packet - etr_tx) * 1000l/ IN_SVAR_SAMPLE);

	if(etr_rx != ether_rx_packet)
		etr_rx = ether_rx_packet;
	if(com_tx_back[i] != ether_tx_packet)
		etr_tx = ether_tx_packet;*/

}

void monitor_reboot(void)
{
	// BOOT
	//vars[base + 6].control = boot;

	if(boot <= 25)
	{
		boot++;
		if(boot % 2 == 0)
			vars[NET_HEALTH_VAR_BASE + 8].control = 1;
		else
			vars[NET_HEALTH_VAR_BASE + 8].control = 0;

	}
}
#endif

void dealwithMonitor(uint8_t bank)
{
	Str_points_ptr ptr;
	Str_points_ptr ptr2;
	U8_T flag;

	ptr.pmon = (Str_monitor_point*)(&backup_monitors[bank]);
	ptr2.pmon = (Str_monitor_point*)(&monitors[bank]);
	//for( bank = 0; bank < MAX_MONITORS; bank++, ptr.pmon++, ptr2.pmon++ )
	{
		flag = 0;

		//if(ptr2.pmon->status == 1)
		{
	// check whether change monitor setting, if changed, change next_sample_time
			monitors[bank].next_sample_time = get_current_time();
	// update an_inputs and num_inputs
			check_monitor_sample_points(bank);

		/* compare the old definition with the new one except for the label */
		if( memcmp( ptr.pmon->inputs, ptr2.pmon->inputs, ( sizeof(Str_monitor_point) - 9 ) ) )
		{
		/* compare sample rate */
			if( memcmp( (void*)&ptr.pmon->second_interval_time,
						(void*)&ptr2.pmon->second_interval_time, 3 ))

			{ /* different sample rate */
				if( ptr2.pmon->an_inputs )
					flag |= 0x01;  /* need a new analog block */
				/* compare number of digital points */
				if( ( ptr.pmon->num_inputs - ptr.pmon->an_inputs ) !=  ( ptr2.pmon->num_inputs - ptr2.pmon->an_inputs ) )
					flag |= 0x02;  /* need a new digital block */
				else
				{
					/* compare digital points' definition */
					if( memcmp( (void*)(ptr.pmon->inputs+ptr.pmon->an_inputs),
							(void*)(ptr2.pmon->inputs+ptr2.pmon->an_inputs),
						( ptr.pmon->num_inputs - ptr.pmon->an_inputs ) *
							sizeof( Point_Net ) ) )
						flag |= 0x02;  /* need a new digital block */
				}
			}
			else /* same sample rate */
			{

				/* compare number of analog points */
				if( ptr.pmon->an_inputs == ptr2.pmon->an_inputs )
				{
					/* compare analog points' definition */
					if( memcmp( (void*)(ptr.pmon->inputs),
								(void*)(ptr2.pmon->inputs),
						( ptr.pmon->an_inputs ) * sizeof( Point_Net ) ) )
						flag |= 0x01;  /* need a new analog block */
				}
				else
				{
					flag |= 0x01;  /* need a new analog block */
				}
				/* compare number of digital points */
				if( ( ptr.pmon->num_inputs - ptr.pmon->an_inputs ) !=
						( ptr2.pmon->num_inputs - ptr2.pmon->an_inputs ) )
				{
					flag |= 0x02;  /* need a new digital block */
				}
				else
				{
					/* compare digital points' definition */
					if( memcmp( (void*)(ptr.pmon->inputs+ptr.pmon->an_inputs),
							(void*)(ptr2.pmon->inputs+ptr2.pmon->an_inputs),
						( ptr.pmon->num_inputs - ptr.pmon->an_inputs ) *
							sizeof( Point_Net ) ) )
						flag |= 0x02;  /* need a new digital block */
				}
			}

		}

	//	no_points = ( ptr.pmon - monitors );/* / sizeof(Str_monitor_point);*/
			if( flag & 0x01 ) /* get a new analog block */
			{
#if  STORE_TO_SD
				if(Write_SD((SD_block_num[bank * 2] >> 8) & 0xfff,bank,1,(U32_T)LOW_BYTE(SD_block_num[bank * 2]) * sizeof(Str_mon_element)) == 1)
#endif
				{
					/*if(SD_exist == 2)
					{
						U8_T temp;
						//E2prom_Read_Byte(EEP_SD_BLOCK_HI1 + bank,&temp);
						if((temp & 0x0f) != (SD_block_num[bank * 2] >> 16 & 0x0f))
						{
							temp &= 0xf0;
							temp |= (SD_block_num[bank * 2] >> 16 & 0x0f);
							//E2prom_Write_Byte(EEP_SD_BLOCK_HI1 + bank,temp);

						}

						//E2prom_Write_Byte(EEP_SD_BLOCK_A1 + bank * 2,HIGH_BYTE(SD_block_num[bank * 2]));
						//E2prom_Write_Byte(EEP_SD_BLOCK_A1 + bank * 2 + 1,LOW_BYTE(SD_block_num[bank * 2]));
						if(SD_block_num[bank * 2] < 0xfffff)
							SD_block_num[bank * 2]++;
						else
						{
							//E2prom_Write_Byte(EEP_SD_BLOCK_A1 + bank * 2,0);
							//E2prom_Write_Byte(EEP_SD_BLOCK_A1 + bank * 2 + 1,0);
							//E2prom_Write_Byte(EEP_SD_BLOCK_HI1 + bank,temp & 0xf0);
							SD_block_num[bank * 2] = 0;
						}
					}*/

					init_new_analog_block( bank, ptr2.pmon);
				}

			}
			if( flag & 0x02 ) /* get a new digital block */
			{
#if  STORE_TO_SD

				if(Write_SD(HIGH_BYTE(SD_block_num[bank * 2 + 1]) + ((SD_block_num[bank * 2 + 1] >> 24) << 16),bank,0,(U32_T)LOW_BYTE(SD_block_num[bank * 2 + 1]) * sizeof(Str_mon_element)) == 1)
#endif
				{
/*					if(SD_exist == 2)
					{
						U8_T temp;
						//E2prom_Read_Byte(EEP_SD_BLOCK_HI1 + bank,&temp);  // high 4 bits
						if((temp & 0xf0) != (SD_block_num[bank * 2] >> 16 & 0xf0))
						{
							temp &= 0x0f;
							temp |= (SD_block_num[bank * 2] >> 16 & 0xf0);
							//E2prom_Write_Byte(EEP_SD_BLOCK_HI1 + bank,temp);

						}

						//E2prom_Write_Byte(EEP_SD_BLOCK_D1 + bank * 2,HIGH_BYTE(SD_block_num[bank * 2 + 1]));
						//E2prom_Write_Byte(EEP_SD_BLOCK_D1 + bank * 2 + 1,LOW_BYTE(SD_block_num[bank * 2 + 1]));
						if(SD_block_num[bank * 2 + 1] < 0xfffff)
							SD_block_num[bank * 2 + 1] ++;
						else
						{
							//E2prom_Write_Byte(EEP_SD_BLOCK_D1 + bank * 2,0);
							//E2prom_Write_Byte(EEP_SD_BLOCK_D1 + bank * 2 + 1,0);
							//E2prom_Write_Byte(EEP_SD_BLOCK_HI1 + bank,temp & 0x0f);
							SD_block_num[bank * 2 + 1] = 0;
						}
					}*/
					init_new_digital_block( bank, ptr2.pmon);
				}

			}

		}
		memcpy(&backup_monitors[bank],&monitors[bank], sizeof(Str_monitor_point));  // record moinitor data
	}

}

#endif // monitor


