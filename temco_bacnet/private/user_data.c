#include "bacnet.h"
#include "string.h"
#include "types.h"
#include "point.h"
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

extern S16_T time_zone = 0;
extern U8_T  input_type[32];
extern U8_T  input_type1[32];
STR_MODBUS Modbus;
U8_T Daylight_Saving_Time;
U16_T SW_REV;
U8_T cpu_type;
U8_T PRODUCT;
U8_T uart0_baudrate;
U8_T uart1_baudrate;
U8_T uart2_baudrate;

U8_T SD_exist;
#define DEFAULT_FILTER 5

U8_T E2prom_Read_Byte(U16_T addr,U8_T *value);
U8_T E2prom_Write_Byte(U16_T addr,U16_T dat);


extern U8_T  SD_exist;
uint16_t 	start_day;
uint16_t	end_day;

extern uint8_t flag_start_scan_network;
extern uint8_t start_scan_network_count;
extern uint16_t scan_network_bacnet_count;


BACNET_DATE Local_Date;
BACNET_TIME Local_Time;

UN_Time  update_dyndns_time;
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


U32_T  update_sntp_last_time;
Str_Extio_point  extio_points[MAX_EXTIO];

Str_in_point  inputs[MAX_INS];// _at_ 0x20000;
Str_out_point outputs[MAX_OUTS];//_at_ 0x22000;
U8_T  month_length[12];// = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
//U8_T  table_week[12];// = {0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5};	//月修正数据表	  

Info_Table			far						 info[18];// _at_ 0x41000;
S8_T   var_unit[MAX_VAR_UNIT][VAR_UNIT_SIZE];


Str_Special  Write_Special;
Str_MISC   	MISC_Info;
Str_Remote_TstDB             Remote_tst_db;// _at_ 0x10000;
Str_Panel_Info 		 		Panel_Info;
Str_Setting_Info     		Setting_Info;
//In_aux				far			in_aux[MAX_IO_POINTS];//_at_ 0x17500; 
//Con_aux				far			con_aux[MAX_CONS];//_at_ 0x26000; 
//Mon_aux           	         mon_aux[MAX_MONITORS];// _at_ 0x27000; 
//U32_T 				 		SD_lenght[MAX_MONITORS * 2];
//U32_T 				 		SD_block_num[MAX_MONITORS * 2];

//Monitor_Block		far			mon_block[2 * MAX_MONITORS];

Mon_Data 			 		*Graphi_data;
S8_T 				far			Garphi_data_buf[sizeof(Mon_Data)];//  _at_ 0x41100;
Alarm_point 		far			alarms[MAX_ALARMS];
U8_T 			    far			ind_alarms;
Alarm_set_point 	    		alarms_set[MAX_ALARMS_SET];
U8_T 			    far							 ind_alarms_set;
U16_T                alarm_id;
S8_T                         new_alarm_flag;
Units_element		    far			digi_units[MAX_DIG_UNIT];
U8_T 					 		ind_passwords;
Password_point			far			passwords[ MAX_PASSW ];
Str_program_point	    far			programs[MAX_PRGS];// _at_ 0x24000;
Str_variable_point		far			vars[MAX_VARS + 12];// _at_ 0x18000;
Str_controller_point 	far			controllers[MAX_CONS];// _at_ 0x25000;
Str_totalizer_point          	totalizers[MAX_TOTALIZERS];// _at_ 0x12500;


Str_monitor_point		far			monitors[MAX_MONITORS];// _at_ 0x12800;
Str_monitor_point		far			backup_monitors[MAX_MONITORS];// _at_ 0x2e000;

multiple_struct msv_data[MAX_MSV][STR_MSV_MULTIPLE_COUNT];

Str_Email_point Email_Setting;
//U32_T  count_max_time[MAX_MONITORS];
//U32_T  max_monitor_time[MAX_MONITORS];

//Str_mon_element          mon_point[MAX_MONITORS * 2][MAX_MON_POINT];

//Aux_group_point          		aux_groups[MAX_GRPS];// _at_ 0x13500;
//S8_T                    far		Icon_names[MAX_ICONS][14];
Control_group_point  	 		control_groups[MAX_GRPS];// _at_ 0x14000;
//Str_grp_element		    far	    	group_data[MAX_ELEMENTS];// _at_ 0x14500;
Str_grp_element			   	  group_data[MAX_ELEMENTS];

S16_T 					far							 total_elements;
S16_T 					far							 group_data_length;


Str_weekly_routine_point 	far		weekly_routines[MAX_WR];//_at_ 0x28000; // _at_ 0x15200;
Wr_one_day					far		wr_times[MAX_WR][MAX_SCHEDULES_PER_WEEK];//_at_ 0x29000 ;//_at_ 0x16000;
U8_T  wr_time_on_off[MAX_WR][MAX_SCHEDULES_PER_WEEK][8];
Str_annual_routine_point	far	 	annual_routines[MAX_AR];// _at_ 0x2a000;//_at_ 0x16500;
U8_T                   		     ar_dates[MAX_AR][AR_DATES_SIZE];//_at_ 0x2b000 ;//_at_ 0x17000;

UN_ID  ID_Config[254];
U8_T  ID_Config_Sche[254];

float  output_priority[MAX_OUTS][16];
float  output_relinquish[MAX_OUTS];

//Date_block	ora_current;
 /* Assume bit0 from octet0 = Jan 1st */
//S8_T 			    	far			*program_address[MAX_PRGS]; /*pointer to code*/
//U8_T    	    	far				prg_code[MAX_PRGS][CODE_ELEMENT * MAX_CODE];// _at_ 0x8000; 
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



NETWORK_POINTS      		  network_points_list_bacnet[MAXNETWORKPOINTS];	 /* points wanted by others */
Byte              			  number_of_network_points_bacnet; 

NETWORK_POINTS      		  network_points_list_modbus[MAXNETWORKPOINTS];	 /* points wanted by others */
Byte              			  number_of_network_points_modbus; 

REMOTE_POINTS		    		  remote_points_list_bacnet[MAXREMOTEPOINTS];  /* points from other panels used localy */
Byte              			  number_of_remote_points_bacnet;


REMOTE_POINTS		    		  remote_points_list_modbus[MAXREMOTEPOINTS];  /* points from other panels used localy */
Byte              			  number_of_remote_points_modbus;

U16_T Last_Contact_Network_points_bacnet[MAXNETWORKPOINTS];
U16_T Last_Contact_Network_points_modbus[MAXNETWORKPOINTS];
U16_T Last_Contact_Remote_points_bacnet[MAXREMOTEPOINTS];
U16_T Last_Contact_Remote_points_modbus[MAXREMOTEPOINTS];


U8_T remote_panel_num;
// include remote mstp device and network bip device
STR_REMOTE_PANEL_DB  remote_panel_db[MAX_REMOTE_PANEL_NUMBER];


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


void init_panel(void)
{	 	
	uint16_t i,j;
	uint8 temp[2];
	Str_points_ptr ptr;
	

	just_load = 1;
	miliseclast_cur = 0;
	miliseclast = 0;
	memset(inputs, '\0', MAX_INS *sizeof(Str_in_point) );
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
	memset(outputs,'\0', MAX_OUTS *sizeof(Str_out_point) );
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
	memset(vars,'\0',MAX_VARS*sizeof(Str_variable_point));
	ptr.pvar = vars;

	for( i=0; i < MAX_VARS; i++, ptr.pvar++ )
	{

		ptr.pvar->value = 0;
		ptr.pvar->auto_manual = 0;
		ptr.pvar->digital_analog = 1; //analog point 
		ptr.pvar->unused = 2; 
		ptr.pvar->range = 0;
	}
	

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

	memset(points_header,0,MAXREMOTEPOINTS * sizeof(POINTS_HEADER));
	memset(network_points_list_modbus,0,MAXNETWORKPOINTS * sizeof(NETWORK_POINTS));
	
	ptr.pnp = network_points_list_modbus;
	number_of_network_points_modbus = 0;
	
	for(i = 0; i < MAXNETWORKPOINTS; i++,ptr.pnp++ )
	{
		ptr.pnp->point_value = DEFUATL_REMOTE_NETWORK_VALUE;
		ptr.pnp->decomisioned = 0;

	}
	
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
	memset( ID_Config, 0, 254 * sizeof(UN_ID));
	memset( ID_Config_Sche, 0, 254);
	
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


	flag_Updata_Clock = 1;
	//Updata_Clock(0);
	timestart = 0;
	
	
	memset(input_raw,1000,2 * MAX_INS);
	memset(input_raw_back,1000,2 * MAX_INS);
	memset(output_raw,0,2 * MAX_OUTS);
	memset(output_raw_back,0,2 * MAX_OUTS);
//	memset(chip_info,0,12);
	init_info_table();

	memset(&Remote_tst_db,0,sizeof(Str_Remote_TstDB) );
	memset(panelname,0,20);
	memset(SD_block_num,0,MAX_MONITORS * 2 * 4);
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
	memset(extio_points,0,MAX_EXTIO * sizeof(Str_Extio_point));
	

	memset(panelname,0,20);
	memset(&Email_Setting, 0, sizeof(Str_Email_point));

	//memset(&SSID_Info,0,sizeof(STR_SSID));

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

 //   E2prom_Read_Byte(EEP_TIME_ZONE_LO, &temp[0]); //fandu add timezone 需要从EEP里读取
 //   E2prom_Read_Byte(EEP_TIME_ZONE_HI, &temp[1]);
 //   timezone = temp[1] * 256 + temp[0];

//	Setting_Info.reg.time_zone = timezone;
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
	

	
	if(cpu_type == 1)
		Setting_Info.reg.mini_type = 0x80 + Modbus.mini_type;
	else if(cpu_type == 2)
		Setting_Info.reg.mini_type = 0x40 + Modbus.mini_type;
	else
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
	
	Setting_Info.reg.panel_type = PRODUCT;

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
	Setting_Info.reg.com_baudrate[0] = uart0_baudrate;
	Setting_Info.reg.com_baudrate[1] = uart1_baudrate;
	Setting_Info.reg.com_baudrate[2] = uart2_baudrate;

	Setting_Info.reg.panel_number	= panel_number;
	
	Setting_Info.reg.instance	= Instance;
	Setting_Info.reg.en_panel_name = 1;
	memcpy(Setting_Info.reg.panel_name,panelname,20);

	Setting_Info.reg.sd_exist = SD_exist;

	Panel_Info.reg.product_type = PRODUCT;


	
//	Setting_Info.reg.network_ID[0] = Modbus.network_ID[0];
//	Setting_Info.reg.network_ID[1] = Modbus.network_ID[1];
//	Setting_Info.reg.network_ID[2] = Modbus.network_ID[2];
	
	Setting_Info.reg.zigbee_module_id = Modbus.zigbee_module_id;

	Setting_Info.reg.MAX_MASTER = MAX_MASTER;


	
	Setting_Info.reg.start_month = Modbus.start_month;
	Setting_Info.reg.start_day = Modbus.start_day;
	Setting_Info.reg.end_month = Modbus.end_month;
	Setting_Info.reg.end_day = Modbus.end_day;
}


	




U32_T get_current_time(void)
{
	/*if(Daylight_Saving_Time)  // timezone : +8 ---> 800
	{
		if((Rtc.Clk.day_of_year >= start_day) && (Rtc.Clk.day_of_year <= end_day))
		{
			return RTC_GetCounter() - (S16_T)timezone * 36 - 3600;
		}
		else
			return RTC_GetCounter() - (S16_T)timezone * 36;
	}
	else
		return RTC_GetCounter() - (S16_T)timezone * 36;*/
	return 0;
}

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
void Send_whois_to_mstp(void)
{
#if (ARM_MINI || ARM_CM5 )
	U8_T count;
	if(Modbus.com_config[2] == BACNET_MASTER || Modbus.com_config[0] == BACNET_MASTER)
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
			U8_T i;
			send_mstp_index = 0;
			rec_mstp_index = 0;
			for(i = 0;i < remote_panel_num;i++)
			{
				if(remote_panel_db[i].protocal == BAC_MSTP)
				{
					Send_SUB_I_Am(i);  // mstp device
				}					
			}
			
			if(rec_mstp_index > 0)
			{
				memcpy(&response_bacnet_ip ,uip_udp_conn->ripaddr,4);
				response_bacnet_port = HTONS(uip_udp_conn->rport);
				Response_bacnet_Start();
			}
		}
	}
#endif

}


extern uint8_t Master_Scan_Network_Count;
void add_remote_panel_db(uint32_t device_id,BACNET_ADDRESS* src,uint8_t panel,uint8_t * pdu,uint8_t pdu_len,uint8_t protocal,uint8_t temcoproduct)
{
#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI)
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
			{ 
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
#endif
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
				// delete this point
//				Test[48]++;
//				Test[49] = ptr->panel;
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

S8_T Get_remote_index_by_device_id(uint32_t device_id,uint8 *index)
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
	U8_T i;
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

#if ARM_MINI || ARM_CM5
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

		memcpy(mstp_bac_buf[rec_mstp_index].buf,&mtu, len);
		mstp_bac_buf[rec_mstp_index].len = len;
		rec_mstp_index++;

		bip_send_mstp_rport = HTONS(uip_udp_conn->rport);

//  auto send 
	
	//cSemaphoreGive(Sem_mstp_Flag);
#endif
}


void chech_mstp_collision(void)
{
	//collision[2]++;
}

void check_mstp_packet_error(void)
{
	//packet_error[2]++;
}

void check_mstp_timeout(void)
{
	//timeout[2]++;
}
