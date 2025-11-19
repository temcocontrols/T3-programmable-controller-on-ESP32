
#include <stdlib.h>
#include <string.h>
#include "scan.h"
#include "commsub.h"
#include "modbus.h"
#include "define.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "user_data.h"
#include "driver/uart.h"


extern xTaskHandle main_task_handle[20];
#define MAX_REMOTE_PANEL_NUMBER 30

#define ScanSTACK_SIZE					2048
#define ScanNetSTACK_SIZE	  512

void uart_send_string(U8_T *p, U16_T length,U8_T port);
xTaskHandle Handle_Scan;
//xTaskHandle Handle_COV;
//void check_id_alarm(uint8_t type, uint8_t id, uint32_t old_sn, uint32_t new_sn);
void add_id_online(U8_T index);

void check_whether_modbus_slave(uint8_t * uart_rsv, uint16_t len, uint8_t port);

U8_T subnet_rec_package_size;
//U8_T  tmp_sendbuf[500];
U8_T force_scan;
U16_T modbus_scan_count;
extern U8_T count_reset_zigbee;
extern U16_T last_reset_zigbee;
extern uint8_t ChangeFlash;
U8_T far scan_port;
U8_T far scan_baut;

U32_T com_rx[3];
U32_T com_tx[3];
U16_T collision[3];  // id collision
U16_T packet_error[3];  // bautrate not match
U16_T timeout[3];

void Change_com_config(U8_T port);
void read_rmp_ad(uint8_t index);

typedef struct
{
	U8_T func;
	U8_T id;
	U16_T reg;
	U16_T value[50];
	U8_T len;
	U8_T flag;
	U8_T retry;
	U8_T float_type;
}STR_NODE_OPERATE;
STR_NODE_OPERATE far node_write[STACK_LEN];

typedef enum { WRITE_OK = 0,WAIT_FOR_WRITE};


STR_NPM_NODE_OPERATE NPM_node_write[STACK_LEN];

STR_NPB_NODE_OPERATE  NPB_node_write[MAX_NPB_NODE_WRITE];


U32_T  com_rx[3];
U32_T  com_tx[3];
U16_T  collision[3];  // id collision
U16_T  packet_error[3];  // bautrate not match
U16_T  timeout[3];

U8_T temp_baut[3];
U8_T flag_temp_baut[3];

//U8_T far flag_scan_sub;
//U8_T far count_scan_sub_by_hand;



U16_T crc16(U8_T *p, U8_T length);
//void uart_send_string(U8_T *p, U16_T length,U8_T port);

#if 1
U8_T far tstat_name[MAX_ID][16];
U8_T far flag_tstat_name[MAX_ID];
U16_T far count_read_tstat_name[MAX_ID];
U8_T flag_rmp_ad[MAXREMOTEPOINTS];
U8_T count_read_rmp_ad[MAXREMOTEPOINTS];
#endif
SCAN_DB  scan_db[MAX_ID] = {0};// _at_ 0x8000;
SCAN_DB  current_db;
S16_T far scan_db_time_to_live[MAX_ID];
U8_T scan_db_baut[MAX_ID];

U8_T db_ctr = 0;
U8_T reset_scan_db_flag = 0;
U8_T remote_modbus_index = 0;
U8_T remote_bacnet_index = 0;
U8_T remote_mstp_panel_index = 0;

U8_T db_online[32];	// Should be added by scan					 
U8_T db_occupy[32]; // Added/subtracted by scan
U8_T current_online[32]; // Added/subtracted by co2 request command
U8_T current_online_ctr = 0;

U8_T subcom_seq[2];
U8_T subcom_num;


//U8_T get_para[32];

U8_T count_send_id_to_zigbee;

U8_T scan_db_changed = FALSE;


U8_T tst_retry[MAX_ID];


void read_name_of_tstat(U8_T index);

void recount_sub_addr(void)
{	
	U8_T i;
	U8_T tmp_uart0 = 0;
	U8_T tmp_uart1 = 0;
	U8_T tmp_uart2 = 0;

//	for(i = 0;i < SUB_NO;i++)
//		sub_addr[i] = 0;

	for(i = 0;i < db_ctr;i++)
	{
		if((scan_db[i].port & 0x0f) == 0 + 1)
			uart0_sub_addr[tmp_uart0++]	= scan_db[i].id;
		else if((scan_db[i].port & 0x0f) == 1 + 1)
			uart1_sub_addr[tmp_uart1++]	= scan_db[i].id;
		else if((scan_db[i].port & 0x0f) == 2 + 1)
			uart2_sub_addr[tmp_uart2++]	= scan_db[i].id;

//		sub_addr[i] = scan_db[i].id;

	}
	
	uart0_sub_no = tmp_uart0;
//	memcpy(sub_addr,uart0_sub_addr,uart0_sub_no);
	sub_no = uart0_sub_no;

	uart1_sub_no = tmp_uart1;
	uart2_sub_no = tmp_uart2;

	sub_no = uart0_sub_no + uart1_sub_no + uart2_sub_no;
	refresh_extio_by_database(0,0,0,0,0);
	ChangeFlash = 1;
}




void Get_Tst_DB_From_Flash(void)
{
	U8_T i;
	U8_T tmp_uart0 = 0;
	U8_T tmp_uart1 = 0;
	U8_T tmp_uart2 = 0;

	db_ctr = 0;
	for(i = 0;i < SUB_NO;i++)
	{	
		if(((scan_db[i].port & 0x0f) == 1) || ((scan_db[i].port & 0x0f) == 2) || ((scan_db[i].port & 0x0f) == 3)) 
		{ 	
			if((scan_db[i].id != 0) && (scan_db[i].id != 0xff))
			{
				if(i > 0)
				{
				 	if(scan_db[i].id != scan_db[i-1].id )
					{

						db_online[scan_db[i].id / 8] |= 1 << (scan_db[i].id % 8);
						db_occupy[scan_db[i].id / 8] &= ~(1 << (scan_db[i].id % 8));
					//	current_online[scan_db[i].id / 8] |= 1 << (scan_db[i].id % 8);
					//	get_para[db_ctr / 8] |= 1 << (db_ctr % 8);
						db_ctr++;
						
					}
				}
				else
				{
					
					db_online[scan_db[i].id / 8] |= 1 << (scan_db[i].id % 8);
					db_occupy[scan_db[i].id / 8] &= ~(1 << (scan_db[i].id % 8));
				//	current_online[scan_db[i].id / 8] |= 1 << (scan_db[i].id % 8);
				//	get_para[db_ctr / 8] |= 1 << (db_ctr % 8);
					db_ctr++;
				}				
			} 
		}		
	}
	for(i = 0;i < db_ctr;i++)
	{			
		if((scan_db[i].port & 0x0f) == 0 + 1)
			uart0_sub_addr[tmp_uart0++]	= scan_db[i].id;
		else if((scan_db[i].port & 0x0f) == 1 + 1)
			uart1_sub_addr[tmp_uart1++]	= scan_db[i].id;
		
	}
	
	uart0_sub_no = tmp_uart0;
	sub_no = uart0_sub_no;


	uart1_sub_no = tmp_uart1;

	sub_no = uart0_sub_no + uart1_sub_no + uart2_sub_no;
// add T3 MAP

	// �������ݿ��е�����˳����expansion IO��λ��
	for(i = 0;i < sub_no;i++)
	{
 		remap_table(i,scan_db[i].product_model);
	}			
		
	refresh_extio_by_database(0,0,0,0,0);
}



U8_T receive_scan_reply(U8_T *p, U8_T len)
{
	U16_T crc_check = crc16(p, 7); // crc16

	if((p[0] == 0xff) && (p[1] == CHECKONLINE)
		&& (HIGH_BYTE(crc_check) == p[7]) && (LOW_BYTE(crc_check) == p[8]))
	{
		current_db.id = p[2];

		current_db.sn = ((U32_T)p[6] << 24) | ((U32_T)p[5] << 16) | 
						((U32_T)p[4] << 8) | p[3];
		if((len == subnet_rec_package_size) /*&& (subnet_response_buf[2] == subnet_response_buf[3])*/)
		{
			return UNIQUE_ID;
		}
		else
		{
			return UNIQUE_ID_FROM_MULTIPLE;
		}
	}
	else
	{
		return MULTIPLE_ID;
	}
}

U8_T send_scan_cmd(U8_T max_id, U8_T min_id,U8_T port)
{
	U16_T wCrc16;	
	U8_T buf[6];
	U16_T length = 0;
	U8_T ret = 0;
	U16_T delay; 

	
	buf[0] = 0xff;
	buf[1] = 0x19;
	buf[2] = max_id;
	buf[3] = min_id;
	wCrc16 = crc16(buf, 4);

	buf[4] = HIGH_BYTE(wCrc16);
	buf[5] = LOW_BYTE(wCrc16);
	uart_send_string(buf, 6,port);
	
	subnet_rec_package_size = 9;

	uint8_t *subnet_response_buf = (uint8_t*)malloc(50);

	if(port == 0 || port == 2)
	{
		length = uart_read_bytes(port, subnet_response_buf, 50, 100 / portTICK_RATE_MS);
	}
	else
		length = 0;

	if(length > 0)
	{
		check_whether_modbus_slave(subnet_response_buf,length,port);
		//check
		if(port == 0)	{led_sub_rx++; flagLED_sub_rx = 1;}
		else if(port == 2)	{led_main_rx++; flagLED_main_rx = 1;}
		com_rx[port] += length;
		ret = receive_scan_reply(subnet_response_buf, length);
	}
	else // NONE_ID || MULTIPLE_ID
	{
//		auto_check_master_retry[port]++;
		if(length > subnet_rec_package_size)
		{
			ret = MULTIPLE_ID;
		}
		else
			ret = NONE_ID;
	}
	
	free(subnet_response_buf);
	return ret;
}

U8_T receive_assign_id_reply(U8_T *p, U8_T length)
{
	U16_T crc_check = crc16(p, 10); // crc16
	if(/*(length == subnet_rec_package_size) &&*/ (HIGH_BYTE(crc_check) == p[10]) && (LOW_BYTE(crc_check) == p[11]))
		return ASSIGN_ID;
	else
		return NONE_ID;
}

U8_T assignment_id_with_sn(U8_T old_id, U8_T new_id, U32_T current_sn,U8_T port)
{
	U8_T buf[12];
	U16_T wCrc16;
	U16_T length, ret = NONE_ID;
	

	buf[0] = old_id;
	buf[1] = 0x06;
	buf[2] = 0;
	buf[3] = 0x0a;	//MODBUS_ADDRESS_PLUG_N_PLAY = 10
	buf[4] = 0x55;
	buf[5] = new_id;


	buf[6] = (U8_T)(current_sn);
	buf[7] = (U8_T)(current_sn >> 8);
	buf[8] = (U8_T)(current_sn >> 16);
	buf[9] = (U8_T)(current_sn >> 24);

	
	wCrc16 = crc16(buf, 10);
	buf[10] = HIGH_BYTE(wCrc16);
	buf[11] = LOW_BYTE(wCrc16);

	uart_send_string(buf, 12,port);
	subnet_rec_package_size = 12;

	uint8_t *subnet_response_buf = (uint8_t*)malloc(50);

	if(port == 0 || port == 2)
		length = uart_read_bytes(port, subnet_response_buf, 50, 100 / portTICK_RATE_MS);
	else
		length = 0;

	if(length > 0)
	{
		if(port == 0)	{led_sub_rx++; flagLED_sub_rx = 1;}
				else if(port == 2)	{led_main_rx++; flagLED_main_rx = 1;}
		com_rx[port] += length;
		ret = receive_assign_id_reply(subnet_response_buf, 12);
	}

	free(subnet_response_buf);
	return ret;
}

U8_T get_idle_id(void)
{
	U8_T i;
	for(i = 1; i < MAX_ID; i++)
	{
		if(((db_online[i / 8] & (1 << (i % 8))) == 0) && ((db_occupy[i / 8] & (1 << (i % 8))) == 0))
			return i;
	}
	return 0xff;
}



U8_T check_id_in_database(U8_T id, U32_T sn,U8_T port,U8_T baut,U8_T product_id)
{	
	U8_T ret;
	U8_T i;
	U8_T replace;
	ret = 0;
	if(id == 0) return 0;
	// check if switch PORT1 TO PORT2
	for(i = 0; i < db_ctr; i++)
	{
		if((id == scan_db[i].id) && (sn == scan_db[i].sn))
		{
			if(port != (scan_db[i].port & 0x0f) - 1)	  // change port
			{
				scan_db[i].port = (baut << 4) + port + 1;	
				if(product_id > 0)
					scan_db[i].product_model = product_id;
				scan_db_changed = TRUE;
				
				
			}
		}
	}
	replace = 255;
	if(id == Modbus.address) 	
	{
		db_occupy[id / 8] |= 1 << (id % 8); 
		// id is confict with master, generate a alarm
#if ALARM_SYNC		
		check_id_alarm(0,id,0,sn);
#endif
	
#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI )
		Record_conflict_ID(id,Modbus.serialNum[0] + (U16_T)(Modbus.serialNum[1] << 8)
	 + ((U32_T)Modbus.serialNum[2] << 16) + ((U32_T)Modbus.serialNum[3] << 24),sn,port,Modbus.product_model);
#endif
		
	}
	else
	{
		
		if(db_online[id / 8] & (1 << (id % 8)))  // in the database
		{	
					
			for(i = 0; i < db_ctr; i++)
			{
				if(id == scan_db[i].id) // id already in the database
				{
					if(sn != scan_db[i].sn) // if there exist the same id with defferent sn, push it into the db_occupy list
					{
						if(Modbus.external_nodes_plug_and_play == 0)
						{
							// for test now 
							//remove_id_from_db(i);
						}
						else
						{
							db_occupy[id / 8] |= 1 << (id % 8);							
						}
						

						if((sn != 0) && (scan_db[i].sn == 0))
						{ // replace old place
							scan_db[i].sn = sn; 
							scan_db[i].port = (baut << 4) + port + 1;
							if(product_id > 0)
								scan_db[i].product_model = product_id;
							
						}						
						else if((sn != 0) && (scan_db[i].sn != 0))
						{							
						// id is confict with sub, generate a alarm
#if ALARM_SYNC
						check_id_alarm(1,id,scan_db[i].sn,sn);
#endif
							
#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI )
							if(current_online[scan_db[i].id / 8] & (1 << (scan_db[i].id % 8)))
								Record_conflict_ID(id,scan_db[i].sn,sn,port,9);
#endif
						}
					}
					break;
				}
				// if the device is already in the database, return without doing anything
			}
		}
		else
		{ 
		// if change id for tstat in db, delete old id in db
			for(i = 0; i < db_ctr; i++)
			{
				if((sn == scan_db[i].sn) && (sn != 0))
				{
					scan_db[i].id = id;	
					replace = i;
					break;	
				}
			}

// if currenct sn is confilt sn, clear confict id
//			if((sn == T3000_Private.sn) && (id != T3000_Private.oldid))
//			{  // change id succusfully, but do not receive response
//				// clear conflict id
//				clear_confilct_id();
//				
//			}
			
			if(replace != 255)
			{ // replace old one
				db_online[scan_db[replace].id / 8] &= ~(1 << (scan_db[replace].id % 8));
				db_occupy[scan_db[replace].id / 8] &= ~(1 << (scan_db[replace].id % 8));

				db_online[id / 8] |= 1 << (id % 8);
				db_occupy[id / 8] &= ~(1 << (id % 8));
//				get_para[db_ctr / 8] |= 1 << (db_ctr % 8);
				scan_db[replace].id = id;
				scan_db[replace].sn = sn; 
				scan_db[replace].port = (baut << 4) + port + 1;
				if(product_id > 0)
					scan_db[replace].product_model = product_id;
//					
			}
			else
			{ // add new one
				db_online[id / 8] |= 1 << (id % 8);
				db_occupy[id / 8] &= ~(1 << (id % 8));
//				get_para[db_ctr / 8] |= 1 << (db_ctr % 8);
			
				scan_db[db_ctr].id = id;
				scan_db[db_ctr].sn = sn; 
				scan_db[db_ctr].port = (baut << 4) + port + 1;
				if(product_id > 0)
					scan_db[db_ctr].product_model = product_id;
				db_ctr++;
				ret = 1;
			}

			scan_db_changed = TRUE;
		}
	}
// add id on-line database
	tst_retry[id] = 0;
	scan_db_time_to_live[id] = SCAN_DB_TIME_TO_LIVE;
	if((current_online[id / 8] & (1 << (id % 8))) == 0)
	{
		U8_T add_i;
		if(get_index_by_id(id,&add_i) == 1)
		{
			if(scan_db[add_i].product_model < CUSTOMER_PRODUCT)
				add_id_online(add_i);
		}
	}
	return ret;
}

void bin_search(U8_T min_id, U8_T max_id,U8_T port) //reentrant
{
	U8_T scan_status;
	if(min_id > max_id) 	return; 
	scan_status = send_scan_cmd(max_id, min_id, port);

	switch(scan_status)	// wait for response from nodes scan command
	{
		case UNIQUE_ID:
			// unique id means it is the only id in the range.
			// if the id is already in the database, set it occupy and then change the id with sn in the dealwith_conflict_id routine.
			Change_com_config(port); // after auto scan, if config or bautrate is changed, store it.
			check_id_in_database(current_db.id, current_db.sn,port,get_baut_by_port(port + 1),0);
			if(min_id != max_id) // to avoid the miss reply some nodes
			{
				//bin_search(min_id, (U8_T)(((U16_T)min_id + (U16_T)max_id) / 2),port);
				//bin_search((U8_T)(((U16_T)min_id + (U16_T)max_id) / 2) + 1, max_id,port);
			}			
			break;
		case MULTIPLE_ID:
			// multiple id means there is more than one id in the range.
			// if the min_id == max_id, there is same id, set the id occupy and return
			// if the min_id != max_id, there is multi id in the range, divide the range and do the sub scan
		  	Change_com_config(port); // after auto scan, if config or bautrate is changed, store it.
			if(min_id == max_id)
			{
				db_occupy[min_id / 8] |= 1 << (min_id % 8);
				if((db_online[min_id / 8] & (1 << (min_id % 8))) && (Modbus.external_nodes_plug_and_play == 0))
				{
					U8_T i = 0;
					for(i = 0; i < db_ctr; i++)
						if(scan_db[i].id == min_id)
							break;
					//remove_id_from_db(i);  // dont need delete, for test now
				}
			}
			else
			{	
				bin_search(min_id, (U8_T)(((U16_T)min_id + (U16_T)max_id) / 2),port);
				bin_search((U8_T)(((U16_T)min_id + (U16_T)max_id) / 2) + 1, max_id,port);
			}
			break;
		case UNIQUE_ID_FROM_MULTIPLE:
			// there are multiple ids in the range, but the fisrt reply is good.
			Change_com_config(port); // after auto scan, if config or bautrate is changed, store it.
			if(min_id == max_id)
			{
				db_occupy[min_id / 8] |= 1 << (min_id % 8);
				if((db_online[min_id / 8] & (1 << (min_id % 8))) && (Modbus.external_nodes_plug_and_play == 0))
				{
					U8_T i = 0;
					for(i = 0; i < db_ctr; i++)
					{
						if(scan_db[i].id == min_id)
						{
							break;
						}
					}
					//remove_id_from_db(i);	

				}

			}
			else
			{	
				check_id_in_database(current_db.id, current_db.sn,port,get_baut_by_port(port + 1),0);
				bin_search(min_id, (U8_T)(((U16_T)min_id + (U16_T)max_id) / 2),port);
				bin_search((U8_T)(((U16_T)min_id + (U16_T)max_id) / 2) + 1, max_id,port);
			}
			break;
		case NONE_ID:  
			// none id means there is not id in the range
			break;
		case SCAN_BUSY:
		default:
			bin_search(min_id, max_id,port);
			break;
	}
	return;
}


U8_T get_baut_by_port(U8_T port)
{
	if(port == 0)  // no in database 
	{
		if(Modbus.com_config[0] == MODBUS_MASTER)
			return Modbus.baudrate[0];
		else 	if(Modbus.com_config[2] == MODBUS_MASTER)
			return Modbus.baudrate[2];
		else
			return 19200;
	}
	else	if(port == 1)	
		return Modbus.baudrate[0];
	else
		return Modbus.baudrate[2];//scan_port;
	
}

U8_T get_baut_by_id(U8_T port,U8_T id)
{
	U8_T i;
	for(i = 0;i < sub_no;i++)
	{
		if(id == scan_db[i].id)
		{
			if((scan_db[i].port >> 4) > 0)  // if baut is larger than 5, it is new structrue
			{
				return 	scan_db[i].port >> 4;
				
			}
			else  // if baut is 0,it is old, return current baut
			{
				return  get_baut_by_port(port);
			}
		}
	}
	return get_baut_by_port(port);
}


void set_baut_by_port(U8_T port,U8_T baut)
{
	temp_baut[port] = baut;
	flag_temp_baut[port] = 1;
}


//void dealwith_conflict_id(U8_T port)
//{
//	U8_T idle_id;
//	U8_T status;
//	U8_T occupy_id = 1;
//	while(1)
//	{
//		if(db_occupy[occupy_id / 8] & (1 << (occupy_id % 8)))
//		{	
//			idle_id = get_idle_id();
//			if(idle_id == 0xff) break;
//				
//			status = send_scan_cmd(occupy_id, occupy_id, port); // get the seperate sn
//			if((status == UNIQUE_ID) || (status == UNIQUE_ID_FROM_MULTIPLE))
//			{
//				// detect ID conflict, decide whether change id automatically by flag external_nodes_plug_and_play	
////				if(Modbus.external_nodes_plug_and_play == 1)
////				{				
//					if(occupy_id == current_db.id)
//						db_occupy[occupy_id / 8] &= ~(1 << (occupy_id % 8));

//					check_id_in_database(current_db.id, current_db.sn,port,get_baut_by_port(port),0);

//					// if still occupy in the database
//					if(db_occupy[current_db.id / 8] & (1 << (current_db.id % 8)))
//					{
//						// assign idle id with sn to this occupy device.
//						collision[port]++;
//						if(assignment_id_with_sn(current_db.id, idle_id, current_db.sn,port)== ASSIGN_ID)
//						{	
//							db_online[idle_id / 8] |= 1 << (idle_id % 8);

//							scan_db[db_ctr].id = idle_id;
//							scan_db[db_ctr].sn = current_db.sn;	

//							db_ctr++;
//							scan_db_changed = TRUE;
//						}
//					}
////				}
////				else  // prompt a warnning to T3000
////				{
////					Test[15] = occupy_id;
////					Test[14]++;
////				}
//					
//				if(status == UNIQUE_ID_FROM_MULTIPLE)
//					continue;
//			}
//			else if(status == MULTIPLE_ID)
//			{
//				continue;
//			}
//			else // if(status == NONE_ID)
//			{
//				// maybe the nodes are removed from the subnet, so skip it.
//			}
//		}
//		
//		occupy_id++;
//		if(occupy_id == 0xff) break;
//	}

//	return;
//}

void scan_sub_nodes(U8_T port)
{
	bin_search(1, 254,port);
}

// read from flash to get the init db_online status
void init_scan_db(void)
{
//	U8_T i;

	U8_T local_id = Modbus.address;
	U32_T local_sn = ((U32_T)Modbus.serialNum[3] << 24) | ((U32_T)Modbus.serialNum[2] << 16) | \
				((U32_T)Modbus.serialNum[1] << 8) | Modbus.serialNum[0];

	memset(db_online, 0, 32);
	memset(db_occupy, 0, 32);
//	memset(get_para, 0, 32);
	memset(current_online, 0, 32);
	memset((uint8 *)(&scan_db[0].id), 0, sizeof(SCAN_DB) * MAX_ID);
	current_online_ctr = 0;

	db_ctr = 0;
#if (ARM_MINI || ASIX_MINI)
	memset(uart1_sub_addr,0,SUB_NO);
	memset(uart2_sub_addr,0,SUB_NO);
	uart1_sub_no = 0;
	uart2_sub_no = 0; 

#endif
	memset(uart0_sub_addr,0,SUB_NO);

	sub_no = 0;
	online_no = 0;
	uart0_sub_no = 0;

}

void clear_scan_db(void)
{
	U8_T i;
	db_ctr = 0;
	force_scan = 1;
	modbus_scan_count = 0;
	Comm_Tstat_Initial_Data();
	init_scan_db();

	for(i = 0;i < MAX_ID;i++)
	{
		flag_tstat_name[i] = 0;
		count_read_tstat_name[i] = READ_TSTAT_COUNT;
		memset(tstat_name[i],0,16);
		
	}
	for (i = 0;i < MAXREMOTEPOINTS;i++)
	{
		flag_rmp_ad[MAXREMOTEPOINTS] = 0;;
		count_read_rmp_ad[MAXREMOTEPOINTS] = 0;
	}
	// clear remote point list
	number_of_remote_points_modbus = 0;
	memset(remote_points_list,0,MAXREMOTEPOINTS * sizeof(REMOTE_POINTS));
	number_of_remote_points_bacnet = 0;

#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI )
	clear_conflict_id();
#endif
//	start_data_save_timer();
	
	refresh_extio_by_database(0,0,0,0,0);
	// clear mstp devide
	memset(remote_panel_db,0,sizeof(STR_REMOTE_PANEL_DB) * MAX_REMOTE_PANEL_NUMBER);
	remote_panel_num = 0;


}


void remove_id_online(U8_T index)
{
	U8_T i;
//	if(index < db_ctr) // can not delete the internal sensor
	{
		i = scan_db[index].id;
		current_online[i / 8] &= ~(1 << (i % 8));
	}
	Check_On_Line();

}


void add_id_online(U8_T index)
{
	U8_T i;
//	if(index < db_ctr) // can not delete the internal sensor
	{
		i = scan_db[index].id;
		
		current_online[i / 8] |= (1 << (i % 8));
		
	}
	Check_On_Line();

}

//void remove_id_from_db(U8_T index)
//{
//	U8_T i;
//	if((db_ctr > 1) && (index < db_ctr)/* && (index > 0)*/) // can not delete the internal sensor
//	{
//		i = scan_db[index].id;
//		db_online[i / 8] &= ~(1 << (i % 8));
//		db_occupy[i / 8] &= ~(1 << (i % 8));

//		for(i = index; i < db_ctr - 1; i++)
//		{
//			scan_db[i].id = scan_db[i + 1].id;
//			scan_db[i].sn = scan_db[i + 1].sn;
//			scan_db[i].port	= scan_db[i + 1].port;
//			scan_db[i].product_model = scan_db[i + 1].product_model;

//		}

//		scan_db[i + 1].id = 0;
//		scan_db[i + 1].sn = 0;
//		scan_db[i + 1].port = 0;
//		scan_db[i + 1].product_model = 0;

//		db_ctr--;
////		scan_db_changed = TRUE;
//	}

//}



void modify_master_id_in_database(U8_T old_id, U8_T set_id)
{
	Modbus.address = set_id;
	scan_db[0].id = set_id;
	//E2prom_Write_Byte(EEP_ADDRESS, Modbus.address);

	// modify scan datebase
	db_online[old_id / 8] &= ~(1 << (old_id % 8));
	db_online[set_id / 8] |= 1 << (set_id % 8);

//	start_data_save_timer();
}


U8_T get_port_by_id(U8_T id)
{
	U8_T i;
	for(i = 0;i < sub_no;i++)
	{
		if(id == scan_db[i].id)
		{
			return 	scan_db[i].port & 0x0f;
		}
	}
	
	// if id is not in database, check which port is master
	for(i = 0;i < 3;i++)
	{
		if(Modbus.com_config[i] == MODBUS_MASTER)
			return i + 1;
	}
	return 0;
}


U8_T get_index_by_id(U8_T id, U8_T* index)
{
	U8_T i;
	for(i = 0;i < sub_no;i++)
	{
		if(id == scan_db[i].id)
		{
			*index = i;
			return 	TRUE;
		}
	}
	return FALSE;
}

U8_T get_IO_index_by_reg(U16_T reg, U8_T* index)
{
	U8_T i;
	if((reg >= MODBUS_OUTPUT_BLOCK_FIRST) && (reg < MODBUS_OUTPUT_BLOCK_LAST) )
	{
		if((reg - MODBUS_OUTPUT_BLOCK_FIRST) % ((sizeof(Str_out_point) + 1)/ 2) == 0)
		{
			*index = (reg - MODBUS_OUTPUT_BLOCK_FIRST) / ((sizeof(Str_out_point) + 1)/ 2);
			return OUT+1;
		}
		else 
			return 0;
	}	
	if((reg >= MODBUS_INPUT_BLOCK_FIRST) && (reg < MODBUS_INPUT_BLOCK_LAST) )
	{
		if((reg - MODBUS_INPUT_BLOCK_FIRST) % ((sizeof(Str_in_point) + 1)/ 2) == 0)
		{
			*index = (reg - MODBUS_INPUT_BLOCK_FIRST) / ((sizeof(Str_in_point) + 1)/ 2);
			return IN+1;
		}
		else 
			return 0;		
	}	
	else
		return 0;
}
// not only for PM
// get read length
U8_T Check_Read_Len(U16_T reg,U8_T product_id,U16_T *newreg)
{
	U8_T len = 0;
	len = 1;
	*newreg = reg;
	if(product_id == 203)   // PM
	{
		switch(reg)
		{
			case 4:case 8:case 12:case 16:
			case 20:case 24:case 28:case 32:case 36:case 40:
			case 44:case 56:case 60:case 85:
			case 180:case 183:case 186:
			case 190:case 194:case 197:case 200:case 203:case 206:		
			case 210:case 214:case 217:case 220:case 223:case 226:
			case 230:case 234:case 237:
			case 240:case 243:case 246:
			case 250:
				len = 0x80 + 2;
			break;
			case 3:			case 11:			case 19:			case 27:	case 63:case 64:case 67:
				len = 1;
				break;
			default:
				break;
			
		}
	}	
	else if(product_id == PM_T36CTA)   // T3 6CTA
	{
		if((reg >= T3_6CTA_AI_REG_START) && (reg < T3_6CTA_AI_REG_START + 20))
		{
			*newreg = MODBUS_INPUT_BLOCK_FIRST + (reg - T3_6CTA_AI_REG_START) * ((sizeof(Str_in_point) + 1) / 2);
			len = 0x80 + (sizeof(Str_in_point) + 1) / 2;
		}
		if((reg >= T3_6CTA_DO_REG_START) && (reg < T3_6CTA_DO_REG_START + 2))
		{
			*newreg = MODBUS_OUTPUT_BLOCK_FIRST + (reg - T3_6CTA_DO_REG_START) * ((sizeof(Str_out_point) + 1) / 2);
			len = 0x80 + (sizeof(Str_out_point) + 1) / 2;
		}		
	}
	// if T3 moudle, multi-read
	else if(product_id == PM_T322AI)   // T3 22I
	{
		if((reg >= T3_22I_AI_REG_START) && (reg < T3_22I_AI_REG_START + 22))
		{
			*newreg = MODBUS_INPUT_BLOCK_FIRST + (reg - T3_22I_AI_REG_START) * ((sizeof(Str_in_point) + 1) / 2);
			len = 0x80 + (sizeof(Str_in_point) + 1) / 2;
		}
	}
	else if(product_id == PM_T3PT12)
	{		
		if((reg >= T3_PT12_AI_REG_START) && (reg < T3_PT12_AI_REG_START + 12))
		{
			*newreg = MODBUS_INPUT_BLOCK_FIRST + (reg - T3_PT12_AI_REG_START) * ((sizeof(Str_in_point) + 1) / 2);
			len = 0x80 + (sizeof(Str_in_point) + 1) / 2;
		}
	}
	else if(product_id == PM_T38AI8AO6DO)   // T3 8AIAO6DO
	{
		if((reg >= T3_8AIAO6DO_AI_REG_START) && (reg < T3_8AIAO6DO_AI_REG_START + 8))
		{
			*newreg = MODBUS_INPUT_BLOCK_FIRST + (reg - T3_8AIAO6DO_AI_REG_START) * ((sizeof(Str_in_point) + 1) / 2);
			len = 0x80 + (sizeof(Str_in_point) + 1) / 2;
		}
		if((reg >= T3_8AIAO6DO_DO_REG_START) && (reg < T3_8AIAO6DO_DO_REG_START + 6))
		{
			*newreg = MODBUS_OUTPUT_BLOCK_FIRST + (reg - T3_8AIAO6DO_DO_REG_START) * ((sizeof(Str_out_point) + 1) / 2);
			len = 0x80 + (sizeof(Str_out_point) + 1) / 2;
		}
		if((reg >= T3_8AIAO6DO_AO_REG_START) && (reg < T3_8AIAO6DO_AO_REG_START + 8))
		{  // 6 DO + 8AO
			*newreg = MODBUS_OUTPUT_BLOCK_FIRST + (reg - T3_8AIAO6DO_AO_REG_START + 6) * ((sizeof(Str_out_point) + 1) / 2);
			len = 0x80 + (sizeof(Str_out_point) + 1) / 2;
		}
	}
	else if(product_id == PM_T34AO)   // T3 4AO
	{
		if((reg >= T3_4AO_DO_REG_START) && (reg < T3_4AO_DO_REG_START + 8))
		{
			*newreg = T3_4AO_DO_REG_START;
			len = 8;
		}
		else if((reg >= T3_4AO_AO_REG_START) && (reg < T3_4AO_AO_REG_START + 4))
		{
			*newreg = T3_4AO_AO_REG_START;
			len = 4;
		}
		else if((reg >= T3_4AO_AI_REG_START) && (reg < T3_4AO_AI_REG_START + 10))
		{
			*newreg = T3_4AO_AI_REG_START;
			len = 10;
		}
	}
	else if(product_id == PM_T38I13O)   // T3 8I13O
	{
		if((reg >= T3_8A13O_DO_REG_START) && (reg < T3_8A13O_DO_REG_START + 13))
		{
			*newreg = T3_8A13O_DO_REG_START;
			len = 13;
		}
		else if(( reg >= T3_8A13O_AI_REG_START) && (reg < T3_8A13O_AI_REG_START + 16))
		{
			*newreg = T3_8A13O_AI_REG_START;
			len = 0x80 + 16;
		}
	}
	else if(product_id == PM_T3IOA)   // T3 8 AO
	{
		if((reg >= T3_8AO_AO_REG_START) && (reg < T3_8AO_AO_REG_START + 8))
		{
			*newreg = T3_8AO_AO_REG_START;
			len = 8;
		}
		else if((reg >= T3_8AO_AI_REG_START) && (reg < T3_8AO_AI_REG_START + 8))
		{
			*newreg = T3_8AO_AI_REG_START;
			len = 8;
		}
	}
	else if(product_id == PM_T332AI)   // T3 32I
	{
		if((reg >= T3_32I_AI_REG_START) && (reg < T3_32I_AI_REG_START + 32))
		{
			*newreg = T3_32I_AI_REG_START;
			len = 32;
		}
	}
	else if(product_id == 10 || product_id == 74)   // T10 or BB
	{
		if((reg >= MODBUS_VAR_FIRST) && (reg < MODBUS_VAR_LAST + 2))
		{
			len =  0x80 + 2;
		}
		if((reg >= MODBUS_OUTPUT_FIRST) && (reg < MODBUS_OUTPUT_LAST + 2))
		{
			len =  0x80 + 2;
		}
		if((reg >= MODBUS_INPUT_FIRST) && (reg < MODBUS_INPUT_LAST + 2))
		{
			len =  0x80 + 2;
		}
	}
	else 
	{
		len = 1;
	}
	
	return len;
}


	
void get_parameters_from_nodes(U8_T index,U8_T type)
{
	U8_T port;
	U8_T baut;
	U8_T buf[8];
	U16_T crc_check;
	U16_T length;
	U8_T read_len = 0;
	U8_T size0;
	U8_T delay;
	U8_T float_type;

	// if start scan command, resume reading parameters after scanning 10 timers
	if(force_scan == 1)
		return;

	float_type = 0;
	if(type == 0)  // heart beat, read product model
	{
		if(scan_db[index].product_model > CUSTOMER_PRODUCT)
			return;
		buf[0] = scan_db[index].id;
		buf[1] = READ_VARIABLES;
		buf[2] = 0;
		buf[3] = 7; // start address
		port = get_port_by_id(buf[0]);
		baut = get_baut_by_id(port,buf[0]);

	}
	else  // update remote point
	{
//		if(number_of_remote_points == 0) return;
		buf[0] = remote_points_list[index].tb.RP_modbus.id;
		buf[1] = remote_points_list[index].tb.RP_modbus.func & 0x7f;
		float_type = (remote_points_list[index].tb.RP_modbus.func & 0xff00) >> 8;
		buf[2] = HIGH_BYTE(remote_points_list[index].tb.RP_modbus.reg);
		buf[3] = LOW_BYTE(remote_points_list[index].tb.RP_modbus.reg); // start address
		port = get_port_by_id(buf[0]);	
		baut = get_baut_by_id(port,buf[0]);
	}
	
#if 1
	if(port == 0) 
	{  // wrong id
	    return;
	}

	port = port - 1;

	if(Modbus.com_config[port] != MODBUS_MASTER)
		return;
#endif
	
	buf[4] = 0;
// check whether current device is Power Merter, if that, deal with it speicailly, muilt-read
	if(type == 0)	
		buf[5] = 1;
	else
	{
		U16_T newreg;
		read_len = Check_Read_Len(remote_points_list[index].tb.RP_modbus.reg,Get_product_by_id(remote_points_list[index].tb.RP_modbus.id),&newreg);
		if(float_type > 0)
		{
			read_len = 0x80 + 2;  // reading float, two bytes
			// 0x80 - reading 2 bytes, 2 - len
		}
		buf[5] = (read_len & 0x7f);

		buf[2] = HIGH_BYTE(newreg);
		buf[3] = LOW_BYTE(newreg);

		if(read_len & 0x80)	read_len = 1;  // reading two bytes
		else read_len = 0;

	}

	crc_check = crc16(buf, 6); // crc16
	buf[6] = HIGH_BYTE(crc_check);
	buf[7] = LOW_BYTE(crc_check);

	set_baut_by_port(port,baut);   // need set temp baut
	//uart_init_send_com(port);
	uart_send_string(buf, 8,port);

	
	if(buf[1] == READ_COIL || buf[1] == READ_DIS_INPUT)
	{
		size0 = (buf[5] + 7) / 8 + 5;
	}
	else
		size0 = buf[5] * 2 + 5;		

	subnet_rec_package_size = size0;
	uint8_t *subnet_response_buf = (uint8_t*)malloc(50);
	if(port == 0 || port == 2)
		length = uart_read_bytes(port, subnet_response_buf, 100, 100 / portTICK_RATE_MS);
	else
		length = 0;

	if(length > 0)	 
	{
		check_whether_modbus_slave(subnet_response_buf,length,port);
		if(port == 0)	{led_sub_rx++; flagLED_sub_rx = 1;}
		else if(port == 2)	{led_main_rx++; flagLED_main_rx = 1;}
		com_rx[port] += length;
		crc_check = crc16(subnet_response_buf,length - 2);
		if((HIGH_BYTE(crc_check) == subnet_response_buf[length - 2]) && (LOW_BYTE(crc_check) == subnet_response_buf[length - 1]))
		{
			if(type == 0)
			{ 	
				U8_T j;
				if(subnet_response_buf[4] < CUSTOMER_PRODUCT)
				{
					scan_db[index].product_model = subnet_response_buf[4];

					if(scan_db[index].product_model == PM_T322AI || scan_db[index].product_model == PM_T38AI8AO6DO
						|| scan_db[index].product_model == PM_T36CTA || scan_db[index].product_model == PM_T3PT12)
					{	
						refresh_extio_by_database(0,0,0,0,1);  // refresh time of T3-expansion
					}

	//				if((index == 0) || (sub_map[index - 1].add_in_map == 1))
					{		
						U8_T i;

	//					if(index > 0)
	//					{
							for(i = 0;i <= index;i++)
							{
								if(sub_map[i].add_in_map == 0)
									break;
							}
	//					}
							if((current_online[scan_db[index].id / 8] & (1 << (buf[0] % 8))) == 0)
							{
								U8_T add_i;
								if(get_index_by_id(scan_db[index].id,&add_i) == 1)
								{
									if(scan_db[add_i].product_model < CUSTOMER_PRODUCT)
										add_id_online(add_i);
								}
							}
						//add_id_online(index);
						remap_table(index,scan_db[index].product_model);	
					}

				}	
			}
			else
			{					
				U8_T i,j;
				get_index_by_id(buf[0],&j);
				if(scan_db[j].product_model == PM_T322AI || scan_db[j].product_model == PM_T38AI8AO6DO
					|| scan_db[j].product_model == PM_T36CTA || scan_db[j].product_model == PM_T3PT12)
				{	
					refresh_extio_by_database(0,0,0,0,1);  //  refresh time of T3-expansion
				}
				
				if(read_len == 0)
				{
					for(i = 0;i < buf[5];i++)
					{
						remote_points_list[index + i].decomisioned = 1;
						remote_points_list[index + i].product = Get_product_by_id(remote_points_list[index].tb.RP_modbus.id);
						if((remote_points_list[index].tb.RP_modbus.func & 0x7f) == READ_COIL || (remote_points_list[index].tb.RP_modbus.func & 0x7f) == READ_DIS_INPUT)
						{  // modbud fuction 01 02
							if(remote_points_list[index].tb.RP_modbus.reg > 255)
							{
								add_remote_point(remote_points_list[index].tb.RP_modbus.id,
								(remote_points_list[index].tb.RP_modbus.func & 0x1f) | (HIGH_BYTE(remote_points_list[index].tb.RP_modbus.reg) << 5),
								HIGH_BYTE(remote_points_list[index].tb.RP_modbus.reg) >> 3,
								LOW_BYTE(remote_points_list[index].tb.RP_modbus.reg),subnet_response_buf[3],
								(remote_points_list[index].tb.RP_modbus.func & 0x80) ? 1 : 0,
								float_type);
							}
							else
								add_remote_point(remote_points_list[index].tb.RP_modbus.id,remote_points_list[index].tb.RP_modbus.func & 0x1f,0,remote_points_list[index].tb.RP_modbus.reg,(U16_T)subnet_response_buf[3],\
							(remote_points_list[index].tb.RP_modbus.func & 0x80) ? 1 : 0,
							float_type);
						}
						else // 03 04
						{
							if(remote_points_list[index].tb.RP_modbus.reg > 255)
							{
								add_remote_point(remote_points_list[index].tb.RP_modbus.id,
								(remote_points_list[index].tb.RP_modbus.func & 0x1f)| (HIGH_BYTE(remote_points_list[index].tb.RP_modbus.reg) << 5),
								HIGH_BYTE(remote_points_list[index].tb.RP_modbus.reg) >> 3,
								LOW_BYTE(remote_points_list[index].tb.RP_modbus.reg),subnet_response_buf[3] * 256 + subnet_response_buf[4],\
								(remote_points_list[index].tb.RP_modbus.func & 0x80) ? 1 : 0,
								float_type);
							}
							else
							{
								add_remote_point(remote_points_list[index].tb.RP_modbus.id,remote_points_list[index].tb.RP_modbus.func & 0x1f,0,remote_points_list[index].tb.RP_modbus.reg + i,(U16_T)subnet_response_buf[3 + i * 2] * 256 + subnet_response_buf[4 + i * 2],\
							(remote_points_list[index].tb.RP_modbus.func & 0x80) ? 1 : 0,
								float_type);
							}
						}
					}
				}
				else
				{	// 读多个字节
					for(i = 0;i < buf[5] / 2;i++)
					{
						if(Get_product_by_id(remote_points_list[index].tb.RP_modbus.id) == 203)  // only for PM test
							add_remote_point(remote_points_list[index].tb.RP_modbus.id,remote_points_list[index].tb.RP_modbus.func,
						HIGH_BYTE(remote_points_list[index].tb.RP_modbus.reg) >> 3,
						remote_points_list[index].tb.RP_modbus.reg,1000L * (256l * subnet_response_buf[3] + subnet_response_buf[4]) \
							+ 1000L * (256l * subnet_response_buf[5] + subnet_response_buf[6]) / 65536 ,\
						(remote_points_list[index].tb.RP_modbus.func & 0x80) ? 1 : 0,
						float_type);
						else  // T3 read 2 bytes
						{
							// if multi-read
							U8_T product = Get_product_by_id(remote_points_list[index].tb.RP_modbus.id);
							if((product == PM_T38AI8AO6DO) || (product == PM_T322AI) || (product == PM_T36CTA) || (product == PM_T3PT12)
							)
							{
								add_remote_point(remote_points_list[index].tb.RP_modbus.id,remote_points_list[index].tb.RP_modbus.func,
								0,remote_points_list[index].tb.RP_modbus.reg,(U32_T)(subnet_response_buf[35] << 24) + (U32_T)(subnet_response_buf[36] << 16) \
									+ (U16_T)(subnet_response_buf[33] << 8) + subnet_response_buf[34],\
								(remote_points_list[index].tb.RP_modbus.func & 0x80) ? 1 : 0,
								float_type);

							}
							else	// old T3 module
							{	
							if(remote_points_list[index].tb.RP_modbus.reg > 255)
							{
								if(product == 10 || product == 74) // T10 or MINIPANEL
								{
									add_remote_point(remote_points_list[index].tb.RP_modbus.id,
									(remote_points_list[index].tb.RP_modbus.func & 0x1f)| (HIGH_BYTE(remote_points_list[index].tb.RP_modbus.reg) << 5),
									HIGH_BYTE(remote_points_list[index].tb.RP_modbus.reg) >> 3,
									remote_points_list[index].tb.RP_modbus.reg + i * 2,(U32_T)(subnet_response_buf[3 + i * 4] << 24) + (U32_T)(subnet_response_buf[4 + i * 4] << 16) \
									+ (U16_T)(subnet_response_buf[5 + i * 4] << 8) + subnet_response_buf[6 + i * 4],
									0,
									float_type);
								}
								else
								add_remote_point(remote_points_list[index].tb.RP_modbus.id,
								(remote_points_list[index].tb.RP_modbus.func & 0x1f)| (HIGH_BYTE(remote_points_list[index].tb.RP_modbus.reg) << 5),
								HIGH_BYTE(remote_points_list[index].tb.RP_modbus.reg) >> 3,
								remote_points_list[index].tb.RP_modbus.reg + i * 2,(U32_T)(subnet_response_buf[3 + i * 4] << 24) + (U32_T)(subnet_response_buf[4 + i * 4] << 16) \
								+ (U16_T)(subnet_response_buf[5 + i * 4] << 8) + subnet_response_buf[6 + i * 4],
								(remote_points_list[index].tb.RP_modbus.func & 0x80) ? 1 : 0,
								float_type);
							}
							else
							{
								add_remote_point(remote_points_list[index].tb.RP_modbus.id,
								remote_points_list[index].tb.RP_modbus.func  & 0x1f,
								0,
								remote_points_list[index].tb.RP_modbus.reg + i * 2,(U32_T)(subnet_response_buf[3 + i * 4] << 24) + (U32_T)(subnet_response_buf[4 + i * 4] << 16) \
								+ (U16_T)(subnet_response_buf[5 + i * 4] << 8) + subnet_response_buf[6 + i * 4],\
							(remote_points_list[index].tb.RP_modbus.func & 0x80) ? 1 : 0,
								float_type);
							}								
							}
						}
						remote_points_list[index + i].decomisioned = 1;
					}
				}											
				

					if(read_len == 0)
					{ // read one byte
						for(i = 0;i < buf[5];i++)
						{
							update_remote_map_table(remote_points_list[index].tb.RP_modbus.id,remote_points_list[index].tb.RP_modbus.reg + i,(U16_T)subnet_response_buf[3 + i * 2] * 256 + subnet_response_buf[4 + i * 2],subnet_response_buf);
						}
						remote_modbus_index = remote_modbus_index + buf[5] - 1;
					}
					else
					{	// multi-read	, only for new T3, work as external IO
						U8_T product = Get_product_by_id(remote_points_list[index].tb.RP_modbus.id);
						if((product == PM_T38AI8AO6DO) || (product == PM_T322AI) || (product == PM_T36CTA) || (product == PM_T3PT12)) 
						{	
							update_remote_map_table(remote_points_list[index].tb.RP_modbus.id,remote_points_list[index].tb.RP_modbus.reg,0,subnet_response_buf);
						}
						else
						{
							for(i = 0;i < buf[5] / 2;i++)
							{
								update_remote_map_table(remote_points_list[index].tb.RP_modbus.id,remote_points_list[index].tb.RP_modbus.reg + i * 2,(U32_T)(subnet_response_buf[3 + i * 4] << 24)  
								+ (U32_T)(subnet_response_buf[4 + i * 4] << 16)	+ (U16_T)(subnet_response_buf[5 + i * 4] << 8) + subnet_response_buf[6 + i * 4],subnet_response_buf);						
							}
							remote_modbus_index = remote_modbus_index + buf[5] / 2 - 1;
						}						
					}


			}
			tst_retry[buf[0]] = 0;
			scan_db_time_to_live[buf[0]] = SCAN_DB_TIME_TO_LIVE;
			if((current_online[buf[0] / 8] & (1 << (buf[0] % 8))) == 0)
			{
				U8_T add_i;
				if(get_index_by_id(buf[0],&add_i) == 1)
				{
					if(scan_db[add_i].product_model < CUSTOMER_PRODUCT)
						add_id_online(add_i);
				}
			}
			
		}
		else
		{
			packet_error[port]++;
		}			
	}
	else
	{	
		timeout[port]++;
		tst_retry[buf[0]]++;

	if(type == 1)  // read parameters
	{
			if(read_len == 0)
			{ // read one byte				
				remote_modbus_index = remote_modbus_index + buf[5] - 1;
			}
			else
			{	// multi-read	, only for new T3, work as external IO
				U8_T product = Get_product_by_id(remote_points_list[index].tb.RP_modbus.id);
				if((product == PM_T38AI8AO6DO) || (product == PM_T322AI) || (product == PM_T36CTA) || (product == PM_T3PT12)) 
				{
					remote_modbus_index = remote_modbus_index + 22;
				}
				else
				{
					
					remote_modbus_index = remote_modbus_index + buf[5] / 2 - 1;
				}						
			}
		}
								

	}

	free(subnet_response_buf);

	if(tst_retry[buf[0]] >= 10)	
	{  		
		U8_T remove_i;
 		// get index form scan_db
		if(get_index_by_id(buf[0],&remove_i) == 1)
		{		
			if(scan_db[remove_i].product_model < CUSTOMER_PRODUCT)
			{
				remove_id_online(remove_i);
			}
		}
	}

}



void write_parameters_to_nodes(uint8_t func,uint8 id, uint16 reg, uint16* value,uint8_t len)
{
	U8_T i;
	// if id is on line, add it to node_write_stack
	// check whether id is in database
	if(Get_product_by_id(id) > 0)
	{ // it is in database,otherwise product is 0
		if((current_online[ id / 8] & (1 << ( id % 8))) == 0)
		{ // current id id off line
			return;
		}
	}
	for(i = 0;i < STACK_LEN;i++)
	{
		if(node_write[i].flag == WRITE_OK) 	
			break;
	}
	if(i == STACK_LEN)		
	{  // stack full
	// tbd
		return;	
	}
	else
	{
		if(len >= 50) return;
		node_write[i].func = func;
		node_write[i].id = id;
		node_write[i].reg = reg;
		node_write[i].len = len;
		
		if(len == 1)
		{
			node_write[i].value[0] = value[0];
		}
		else
		{			
			U8_T temp_value[50];
			U8_T j;
			for(j = 0;j < len / 2;j++)
			{
				temp_value[j * 2] = value[j] >> 8;
				temp_value[j * 2 + 1] = value[j];
			}
			memcpy(&node_write[i].value[0],&temp_value,len);
		}
		node_write[i].len = len;
		node_write[i].flag = WAIT_FOR_WRITE;
		node_write[i].retry = 0;
	}
}

void check_write_to_nodes(U8_T port)
{
	U8_T far buf[100];
	U16_T length;
	U16_T crc_check;
	U8_T baut;
	U8_T i,j;
	for(i = 0;i < STACK_LEN;i++)
	{
		if(node_write[i].flag == WAIT_FOR_WRITE) //	get current index, 1 -- WAIT_FOR_WRITE, 0 -- WRITE_OK
		{
			if(node_write[i].retry < MAX_WRITE_RETRY)
			{
				node_write[i].retry++;
				break;
			}
			else
			{  	// retry 10 time, give up
				node_write[i].flag = WRITE_OK; 
				node_write[i].retry = 0;
			}
		}
	}
	if(i >= STACK_LEN)		// no WAIT_FOR_WRITE
	{
		return ;	
	}

	if(Get_product_by_id(node_write[i].id) > 0)
	{ // it is in database,otherwise product is 0
		if((current_online[ node_write[i].id / 8] & (1 << ( node_write[i].id % 8))) == 0)
		{ // current id id off line
			return ;
		}
	}
		

	buf[0] = node_write[i].id;

	baut = get_baut_by_id(port,buf[0]);
	set_baut_by_port(port,baut);	
	
	//uart_init_send_com(port);
	
	buf[2] = HIGH_BYTE(node_write[i].reg);
	buf[3] = LOW_BYTE(node_write[i].reg); // start address
	if(node_write[i].len > 1)
	{
		buf[1] = node_write[i].func;
		buf[4] = 0;
		buf[5] = node_write[i].len;

		if(node_write[i].func == 0x0f)
		{
			uint8_t temp = 0;
			buf[6] = (node_write[i].len + 7) / 8;

			for( j = 0;j < node_write[i].len;j++)
			{
				temp |= ((node_write[i].value[j] != 0) ? 1 : 0) << (j % 8);

				if((j + 1) % 8 == 0)
				{
					buf[6 + (j + 1) / 8] = temp;
					temp = 0;
				}
				else if(j + 1 == node_write[i].len)
				{
					buf[6 + (j + 1) / 8 + 1] = temp;
				}
			}

		}
		else
		{
			buf[6] = node_write[i].len * 2;

			for(j = 0;j < node_write[i].len;j++)
			{
				buf[7 + j * 2] = HIGH_BYTE(node_write[i].value[j]);
				buf[8 + j * 2] = LOW_BYTE(node_write[i].value[j]);
			}
		}
		crc_check = crc16(buf, buf[6] + 7); // crc16
		buf[buf[6] + 7] = HIGH_BYTE(crc_check);				 
		buf[buf[6] + 8] = LOW_BYTE(crc_check);
		uart_send_string(buf, buf[6] + 9,port);
	}
	else  // signal wirte
	{		
		buf[1] = node_write[i].func;
		if(node_write[i].func == 0x05)
		{
			if(node_write[i].value[0] != 0)
			{
				buf[4] = 0xff;
				buf[5] = 0x00;
			}
			else
			{
				buf[4] = 0x00;
				buf[5] = 0x00;
			}
		}
		else
		{
			buf[4] = HIGH_BYTE(node_write[i].value[0]);
			buf[5] = LOW_BYTE(node_write[i].value[0]);
		}
		
		crc_check = crc16(buf, 6); // crc16
		buf[6] = HIGH_BYTE(crc_check);				 
		buf[7] = LOW_BYTE(crc_check);

		uart_send_string(buf, 8,port);	
	}
	subnet_rec_package_size = 8;

	uint8_t *subnet_response_buf = (uint8_t*)malloc(50);
	if(port == 0 || port == 2)
		length = uart_read_bytes(port, subnet_response_buf, 50, 100 / portTICK_RATE_MS);
	else
		length = 0;

	if(length > 0)
	{
		check_whether_modbus_slave(subnet_response_buf,length,port);
		if(port == 0)	{led_sub_rx++; flagLED_sub_rx = 1;}
		else if(port == 2)	{led_main_rx++; flagLED_main_rx = 1;}
		com_rx[port] += length;
		node_write[i].flag = WRITE_OK; // without doing checksum
//		node_write[i].id = 0;
//		node_write[i].reg = 0;
		if(node_write[i].len == 1)
		{	
			if(node_write[i].func == 0x05)
				add_remote_point(buf[0],READ_COIL + (buf[2] << 5),buf[2] >> 3,buf[3],node_write[i].value[0],1,0);
			else if(node_write[i].func == 0x06)
			{
				add_remote_point(buf[0],READ_VARIABLES + (buf[2] << 5),buf[2] >> 3,buf[3],node_write[i].value[0],1,0);
			}
		}
		
		tst_retry[buf[0]] = 0;
		scan_db_time_to_live[buf[0]] = SCAN_DB_TIME_TO_LIVE;
		if((current_online[buf[0] / 8] & (1 << (buf[0] % 8))) == 0)
		{
			U8_T add_i;
			if(get_index_by_id(buf[0],&add_i) == 1)
			{
				if(scan_db[add_i].product_model < CUSTOMER_PRODUCT)
					add_id_online(add_i);
			}
		}
	}
	else
	{
//#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI ) // ASIX does not receive packets
		timeout[port]++;
		tst_retry[buf[0]]++;
//#endif

	}
	
	if(tst_retry[buf[0]] >= 10)	
	{  		
		U8_T remove_i;
		
 		// get index form scan_db
		if(get_index_by_id(buf[0],&remove_i) == 1)
		{			
			if(scan_db[remove_i].product_model < CUSTOMER_PRODUCT)
			{
				remove_id_online(remove_i);
			}
		}
	}
	
	free(subnet_response_buf);

}

void Count_com_config(void)
{
	U8_T i = 0;	
	U8_T temp = 0;
	for(i = 0;i < 3;i++)
		if(Modbus.com_config[i] == MODBUS_MASTER)
		{
			subcom_seq[temp] = i;
			temp++;
		}
	subcom_num = temp;
}


//#define READ_POINT_TIMER 100
//#define SCAN_SUB_TIMER   1000
uint16 READ_POINT_TIMER;
uint16 READ_POINT_TIMER_FROM_EEP;

U8_T far index_sub;

// Heart beat scan
void Scan_Idle(void)
{

	static uint8_t count = 0;

	//if(count++ % 5 == 0)
	{
		if(sub_no > 0)
		{
			if(index_sub < sub_no - 1)
			{
				index_sub++;
			}
			else
			{
				index_sub = 0;
			}


			if(current_online[scan_db[index_sub].id / 8] & (1 << (scan_db[index_sub].id % 8)))  // on line
			{
#if 1
				if(flag_tstat_name[index_sub] == 0)  // read name
				{
					// read name of tstat
					read_name_of_tstat(index_sub);
				}
				else
#endif
				{
					get_parameters_from_nodes(index_sub,0);  // heartbeat
				}
			}
			else
			{
				if(scan_db[index_sub].product_model > CUSTOMER_PRODUCT)  // customer device
				{
					add_id_online(index_sub);
				}
				//else
				{
					get_parameters_from_nodes(index_sub,0); // heartbeat
				}
			}
		}
	}
	//else
	{ // check whether it is modbus slave

		if(uart0_sub_no == 0)
		{
			uint8_t *subnet_response_buf = (uint8_t*)malloc(50);
			int len = uart_read_bytes(0, subnet_response_buf, 50, 10 / portTICK_RATE_MS);

			if(len>0)
			{
				led_sub_rx++;
				com_rx[0] += len;
				flagLED_sub_rx = 1;
				check_whether_modbus_slave(subnet_response_buf,len,0);
			}
			free(subnet_response_buf);

		}
		if(((Modbus.mini_type >= MINI_BIG_ARM) && (Modbus.mini_type <= MINI_NANO))
			    	|| (Modbus.mini_type == PROJECT_RMC1216) || (Modbus.mini_type == PROJECT_NG2_NEW))
		{
			if(uart2_sub_no == 0)
			{
				uint8_t *subnet_response_buf = (uint8_t*)malloc(50);
				int len = uart_read_bytes(2, subnet_response_buf, 50, 10 / portTICK_RATE_MS);

				if(len>0)
				{
					led_main_rx++;
					com_rx[2] += len;
					flagLED_main_rx = 1;
					check_whether_modbus_slave(subnet_response_buf,len,2);
				}
				free(subnet_response_buf);
			}
		}

/*		static uint8_t index;
		if(subcom_num >= 1)
		{
			uint8_t *subnet_response_buf = (uint8_t*)malloc(50);
			uint8_t port;
			static uint8_t comindex2 = 0;
			if(comindex2 < subcom_num - 1)
			{
				comindex2++;
			}
			else
				comindex2 = 0;

			port = subcom_seq[comindex2];
			Test[21]++;
			int len = uart_read_bytes(port, subnet_response_buf, 50, 10 / portTICK_RATE_MS);

			if(len>0)
			{
				if(port == 2)
				{
					led_main_rx++;
					com_rx[2] += len;
					flagLED_main_rx = 1;
				}
				if(port == 0)
				{
					led_sub_rx++;
					com_rx[0] += len;
					flagLED_sub_rx = 1;
				}
				check_whether_modbus_slave(subnet_response_buf,len,port);
			}

		}
*/

	}
}

void check_whether_force_scan(void)
{
	if(force_scan == 1)
	{
		modbus_scan_count++;	
		if(modbus_scan_count > 10)
		{
			force_scan = 0;
			modbus_scan_count = 0;
		}
	}	
}

U8_T tempcount;	
U8_T comindex1;
U8_T comindex2;	

#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI )
U8_T TimeSync_Tstat(void);

STR_Sch_To_T8 Sch_To_T8[MAX_ID];
U8_T Schedule_Sync_Tstat(void);
U8_T SendSchedualData(U8_T protocal);
#endif
uint8 flag_response_zigbee;
void Get_ZIGBEE_ID(void);

extern uint32_t  system_timer;

void update_Read_point_timer(void)
{
	if(((system_timer / 1000) > 120) && (force_scan == 0)) //update Read_point_timer 10min later
	{
		READ_POINT_TIMER = READ_POINT_TIMER_FROM_EEP;
	}
	if(force_scan == 1)
		READ_POINT_TIMER = 100;

	// limit range (100ms ~ 10s)
	if(READ_POINT_TIMER < 100)
		READ_POINT_TIMER = 100;
	else if(READ_POINT_TIMER > 10000)
		READ_POINT_TIMER = 10000;
}

void ScanTask(void)
{
	U8_T far port = 0;
	U8_T far baut = 0;
	U8_T far type = 0;

	tempcount = 15;
	comindex1 = 0;
	comindex2 = 0;

	Count_com_config();	
	force_scan = 1;
	modbus_scan_count = 0;
	memset(tst_retry,0,MAX_ID);
	index_sub = 0;
	flag_response_zigbee = 0;
	type = 0;

	while(1)	 
	{	

		update_Read_point_timer();
		if(flag_suspend_scan == 0)
		{
			// write schedule data
			check_whether_force_scan();
			if(tempcount >= 20000 / READ_POINT_TIMER)	 // 20s
			{
				if(subcom_num >= 1)
				{
					port = subcom_seq[comindex1];
					if(comindex1 < subcom_num - 1)
					{
						comindex1++;
					}
					else
						comindex1 = 0;

					scan_sub_nodes(port);		   // 19200
				}
				tempcount = 0;
			}
			else
			{
				Check_scan_db_time_to_live();

				vTaskDelay( READ_POINT_TIMER / portTICK_RATE_MS);
				tempcount++;

				if(type < 50)
				{
					type++;
				}
				else
				{
					type = 0;
				}


				if(type == 0)  // hearbeat or program
				{
					Scan_Idle();
				}
				else
				{// update remote point
					if(subcom_num > 0)
					{
						port = subcom_seq[comindex2];
						
						if(comindex2 < subcom_num - 1)
						{
							comindex2++;
						}
						else
							comindex2 = 0;

						check_write_to_nodes(port);  // write have high priority
						// no write cmd, then read
						if(number_of_remote_points_modbus == 0)
						{  // no remote point, go to heartbeat
							uint8_t i;
							for(i = 0;i < 5;i++)
							{
								Scan_Idle();
								vTaskDelay( 1000 / portTICK_RATE_MS);
							}
							tempcount = tempcount + 50;
	// only heartbeat frame, slow down
						}
						else
						{
							U8_T temp_index;

							if(remote_modbus_index < number_of_remote_points_modbus)
							{
								remote_modbus_index++;
							}
							else
							{
								remote_modbus_index = 0;
							}
							get_index_by_id(remote_points_list[remote_modbus_index].tb.RP_modbus.id, &temp_index);

							// if current id is in DB, must check whether it is on line
							// if current id is not in DB, must implement it
							if(((current_online[remote_points_list[remote_modbus_index].tb.RP_modbus.id / 8] & (1 << (remote_points_list[remote_modbus_index].tb.RP_modbus.id % 8))) || ((scan_db[temp_index].product_model >= CUSTOMER_PRODUCT)))
								|| (!(db_online[remote_points_list[remote_modbus_index].tb.RP_modbus.id / 8] & (1 << (remote_points_list[remote_modbus_index].tb.RP_modbus.id % 8))))  // not in DB
							)
							{

								// current device is on line, read remote point one by one
								if(remote_points_list[remote_modbus_index].tb.RP_modbus.id > 0)
								{
									if(flag_rmp_ad[remote_modbus_index] == 0)
										read_rmp_ad(remote_modbus_index);
									else
										get_parameters_from_nodes(remote_modbus_index,type);	// remote modubs point
								}
								else
								{
									Scan_Idle();
								}
							}
							else
							{
								// once find current device is off line
								remote_points_list[remote_modbus_index].decomisioned = 0;
							// heart beat
								Scan_Idle();

							}

						}

					}
				}

			}

			if(scan_db_changed == TRUE)
			{
				recount_sub_addr();
	//  recount IN OUT VAR map table
	
				scan_db_changed = FALSE;
			}

		}
		else
		{
			if(suspend_scan_count++ > 5)
			{
				flag_suspend_scan = 0;
				suspend_scan_count = 0;
			}
			vTaskDelay(1000 / portTICK_RATE_MS);
		}
	}
}

U8_T Get_product_by_id(U8_T id)
{
	U8_T i;
	for(i = 0;i < sub_no;i++)
	{
		if(id == scan_db[i].id)
		{
				return scan_db[i].product_model;
		}
	}
	return 0;
}

void Check_On_Line(void)
{
	U8_T temp_no;
	U8_T i;
	temp_no = 0;
	for(i = 0;i < sub_no;i++)
	{
		if((current_online[scan_db[i].id / 8] & (1 << (scan_db[i].id % 8))))	  	 // in database and on_line
		{
			temp_no++;
		}				
	}	
	current_online_ctr = temp_no;		

}

void write_NP_Modbus_to_nodes(U8_T ip,U8_T func,U8_T sub_id, U16_T reg, U16_T * value, U8_T len)
{

	U8_T i = 0;
	for(i = 0;i < STACK_LEN;i++)
	{
		if(NPM_node_write[i].flag == WRITE_OK)
			break;
	}
	
	if(i == STACK_LEN)
	{  // stack full
	// tbd
		//return -1;
	}
	else
	{
		if(len >= 2) return ;
		NPM_node_write[i].ip = ip;
		NPM_node_write[i].func = func;
		NPM_node_write[i].id = sub_id;
		NPM_node_write[i].reg = reg;
		NPM_node_write[i].len = len;
		
		if(len == 1)
		{
			NPM_node_write[i].value[0] = value[0];
		}
		else
		{
			U8_T temp_value[50];
			U8_T j;
			for(j = 0;j < len / 2;j++)
			{
				temp_value[j * 2] = value[j] >> 8;
				temp_value[j * 2 + 1] = value[j];
			}
			memcpy(&NPM_node_write[i].value[0],&temp_value,len);

		}
		NPM_node_write[i].len = len;
		NPM_node_write[i].flag = WAIT_FOR_WRITE;
		NPM_node_write[i].retry = 0;
	}
	//return 0;
}



void Response_TCPIP_To_SUB(U8_T *buf, U16_T len,U8_T port,U8_T *header)
{
	int length = 0;
	U16_T crc_check = 0;
	U16_T size0 = 0;
	U8_T flag_expansion = 0;
	U8_T ret = 0;
	//uint8 tmp_sendbuf[500];
	//uint8_t subnet_response_buf[500];
	uint8_t *tmp_sendbuf = NULL;
	uint8_t *subnet_response_buf = NULL;

	tmp_sendbuf = malloc(512);
	if (tmp_sendbuf == NULL) {
		// 处理内存分配失败
		return;
	}
	subnet_response_buf = malloc(512);
	if (subnet_response_buf == NULL) {
		// 处理内存分配失败
		return;
	}

	flag_expansion = 0;

	if(buf[1] == READ_VARIABLES || buf[1] == READ_INPUT || buf[1] == READ_COIL || buf[1] == READ_DIS_INPUT ) // read
	{	
		U8_T i;
		if(buf[1] == READ_COIL || buf[1] == READ_DIS_INPUT)
		{
			size0 = (buf[5] + 7) / 8 + 5;
		}
		else
		  size0 = buf[5] * 2 + 5;		
		// change it bigger, tstat7 response too slow
	}
	else if(buf[1] == WRITE_VARIABLES || buf[1] == MULTIPLE_WRITE || buf[1] == WRITE_COIL || buf[1] == WRITE_MULTI_COIL)
	{	
		size0 = 8;

		if(buf[1] == MULTIPLE_WRITE)  // multi-write
		{
			if((buf[2] * 256 + buf[3]) >= MODBUS_OUTPUT_BLOCK_FIRST)  // write expansion i/o
			{
				flag_expansion = 1;
			}
		}
	}
	else if(buf[1] == TEMCO_MODBUS)
	{
		uint16 size;

//0A FF 54 45 4D 43 4F 07 00 15 00 00 0A 00 0F 39
//0A FF 54 45 4D 43 4F 07 00 15 00 00 0A 00 3A 4E C1 57 20 00 00 00 00 00 BF 24

		if(buf[9] < 100)  // read   command < 100
		{
			size = (U16_T)((buf[13] & 0x01)	<< 8)	+ buf[12]; // entysize
			size0 = 16 + size * (buf[11] - buf[10] + 1); // �ظ��ĳ�����xx
		}
		else  // private command > 100 write
		{
			size0 = 16;  // �ظ��ĳ�����16
		}		
		
	}

	if(buf[1] == CHECKONLINE)  // scan command
	{
		port = scan_port;
		set_baut_by_port(port,scan_baut);		
		size0 = 9;
	}
	else
	{
		if(buf[0] != 255)
		{
			set_baut_by_port(port,get_baut_by_id(0,buf[0]));
		}
		else
		{
			if(port == 0)	
				set_baut_by_port(port,Modbus.baudrate[0]);
			else if(port == 1)	
				set_baut_by_port(port,Modbus.baudrate[1]);
			
		}
	}


	if(buf[1] == TEMCO_MODBUS)
	{
		memcpy(tmp_sendbuf,buf,len - 2);
		crc_check = crc16(tmp_sendbuf, len - 2);
		tmp_sendbuf[len - 2] = HIGH_BYTE(crc_check);
		tmp_sendbuf[len - 1] = LOW_BYTE(crc_check);
		uart_send_string(tmp_sendbuf, len,port);
	}
	else
	{	
		memcpy(tmp_sendbuf,buf,len);
		crc_check = crc16(tmp_sendbuf, len);

		tmp_sendbuf[len] = HIGH_BYTE(crc_check);
		tmp_sendbuf[len + 1] = LOW_BYTE(crc_check);

		uart_send_string(tmp_sendbuf, len + 2 ,port);
	}

#if 1
	
	if(buf[1] == CHECKONLINE)  // scan command
	{
		if(port == 0 || port == 2)
			length = uart_read_bytes(port, subnet_response_buf, 512, 20 / portTICK_RATE_MS);
		else
			length = 0; // tbd:
		if(length > 0)
		{
			check_whether_modbus_slave(subnet_response_buf,length,port);
			if(port == 0)	{led_sub_rx++; flagLED_sub_rx = 1;}
			else if(port == 2)	{led_main_rx++; flagLED_main_rx = 1;}
			com_rx[port] += length;
			memcpy(tmp_sendbuf,header,6);
			memcpy(&tmp_sendbuf[6],subnet_response_buf,length);			

			memcpy(modbus_send_buf,tmp_sendbuf,length + 6);
			modbus_send_len = length + 6;

			
			ret = receive_scan_reply(subnet_response_buf, length);
			if((ret == UNIQUE_ID) || (ret == UNIQUE_ID_FROM_MULTIPLE))
			{
				check_id_in_database(current_db.id, current_db.sn,port,scan_baut,0);
				if(Modbus.com_config[port] != MODBUS_MASTER)
				{
					Modbus.com_config[port] = MODBUS_MASTER;
					//E2prom_Write_Byte(EEP_COM0_CONFIG + port,Modbus.com_config[port]);
				}
				
				if(port == 0) 
				{
					Modbus.baudrate[0] = scan_baut;
					//E2prom_Write_Byte(EEP_UART0_BAUDRATE,uart0_baudrate);
				}
				else if(port == 1)
				{
					Modbus.baudrate[1] = scan_baut;
					//E2prom_Write_Byte(EEP_UART2_BAUDRATE,uart2_baudrate);
				}				
		
				if(scan_db_changed == TRUE)
				{
					recount_sub_addr();
					scan_db_changed = FALSE;			
				}
			}
		}
	}
	else
	{
		if(port == 0 || port == 2)
			length = uart_read_bytes(port, subnet_response_buf, 512, 100 / portTICK_RATE_MS);
		else
			length = 0;

#if 1
		if(length > 2)
		{
			check_whether_modbus_slave(subnet_response_buf,length,port);
			if(port == 0)	{led_sub_rx++; flagLED_sub_rx = 1;}
			else if(port == 2)	{led_main_rx++; flagLED_main_rx = 1;}
			com_rx[port] += length;
			crc_check = crc16(subnet_response_buf, length - 2);		
			if(crc_check == subnet_response_buf[length - 2] * 256 + subnet_response_buf[length - 1])
			{
//			auto_check_master_retry[port] = 0;
				memcpy(tmp_sendbuf,header,6);


				if(buf[1] == TEMCO_MODBUS)
				{
					memcpy(&tmp_sendbuf[6],subnet_response_buf,length);	
					memcpy(modbus_send_buf,tmp_sendbuf,size0 + 6);
					modbus_send_len = size0 + 6;
				}
				else
				{
					memcpy(&tmp_sendbuf[6],subnet_response_buf,length - 2);
					memcpy(modbus_send_buf,tmp_sendbuf,size0 + 4);
					modbus_send_len = size0 + 4;
				}

				// if write name, copy name to tstat_name right now
#if 1
				if(buf[2] == 0x02 && buf[3] == 0xcb && buf[5] == 0x10)  // 715
				{
					// check id
					U8_T index;
					if(get_index_by_id(buf[0], &index))
					{
						memcpy(tstat_name[index],&buf[7],16);
					}
				}
#endif


				if(flag_expansion == 1)
				{  
					// forword mult_write expansion io, refresh local status at once					
					update_remote_map_table(buf[0],buf[2] * 256 + buf[3],0,&buf[7]);					
				}


				ret = 1;
			// add id to online
				tst_retry[buf[0]] = 0;
				scan_db_time_to_live[buf[0]] = SCAN_DB_TIME_TO_LIVE;
				if((current_online[buf[0] / 8] & (1 << (buf[0] % 8))) == 0)
				{
					U8_T add_i;
					if(get_index_by_id(buf[0],&add_i) == 1)
					{				
						if(scan_db[add_i].product_model < CUSTOMER_PRODUCT)
							add_id_online(add_i);
					}
				}
#endif
			}
			else
			{
				packet_error[port]++;
			}			
		}
		else
		{// һ��read�����ظ�ȫ0
			memset(&modbus_send_buf,0,size0 + 4);
			modbus_send_len = size0 + 4;

			timeout[port]++;
			tst_retry[buf[0]]++;
		}

		/*if(tst_retry[buf[0]] >= 10)
		{  		
			U8_T remove_i;
			// get index form scan_db
			if(get_index_by_id(buf[0],&remove_i) == 1)
			{			
				if(scan_db[remove_i].product_model < CUSTOMER_PRODUCT)
				{
					remove_id_online(remove_i);
				}
			}
		}*/
	}	
#endif
	free(subnet_response_buf);

	free(tmp_sendbuf);

	return ;
}



void vStartScanTask(unsigned char uxPriority)
{
	U8_T i;

	memset(node_write,0,sizeof(STR_NODE_OPERATE) * STACK_LEN);
	subcom_seq[0] = subcom_seq[1] = subcom_seq[2] = 0;
	subcom_num = 0;	 

	reset_scan_db_flag = 0;
	current_online_ctr = 0;
	count_send_id_to_zigbee = 0;
	remote_modbus_index = 0;
//	flag_scan_sub = 0;
//	count_scan_sub_by_hand = 0;
	
	for(i = 0;i < 3;i++)
	{
		com_rx[i] = 0;
		com_tx[i] = 0;
		collision[i] = 0;
		packet_error[i] = 0;
		timeout[i] = 0;
		
		flag_temp_baut[i] = 0;
		temp_baut[i] = 0;
	}


	for(i = 0;i < MAX_ID;i++)
	{
		flag_tstat_name[i] = 0;
		count_read_tstat_name[i] = READ_TSTAT_COUNT;
		scan_db_time_to_live[i] = SCAN_DB_TIME_TO_LIVE;
	}
#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI )
	clear_conflict_id();
#endif
	scan_port = 0xff;
    scan_baut = 0xff;

    xTaskCreate(ScanTask,"ScanTask",4096, NULL, uxPriority, (xTaskHandle *)&main_task_handle[5]);

}


// when get remote modbus points by VARX, INX, OUTX, need know a/d of the point
void read_rmp_ad(uint8_t index)
{
	uint8_t num;
//	uint8_t i;
	REMOTE_POINTS * ptr;
	uint16_t reg = 0;

	U8_T port;
	U8_T baut;
	U8_T buf[8];
	U16_T crc_check;
	U16_T length;

	ptr = &remote_points_list[index];
	if(flag_rmp_ad[index] == 0)
	{
		if( (ptr->point.panel != panel_number) && (ptr->point.sub_id == 0) &&
			(current_online[ptr->point.panel / 8] & (1 << (ptr->point.panel % 8))) // 2VARx  sub modbus device
			)
		{// 3VAR1 3IN1 3OUT1 should check ana_dig of remote modbus points
			if(ptr->point.point_type == VAR + 1)
				reg = MODBUS_VAR_AD_FIRST + ptr->point.number;
			else if(ptr->point.point_type == OUT + 1)
				reg = MODBUS_OUTPUT_AD_FIRST + ptr->point.number;
			else if(ptr->point.point_type == IN + 1) // HIGH 4 BIT
				reg = MODBUS_INPUT_RANGE_FIRST + ptr->point.number;

			buf[0] = ptr->point.panel;
			buf[1] = 0x03;
			buf[2] = reg >> 8;
			buf[3] = reg;
			buf[4] = 0;
			buf[5] = 1;

			port = get_port_by_id(buf[0]);
			baut = get_baut_by_id(port,buf[0]);
			if(port == 0)
			{  // wrong id
					return;
			}

			port = port - 1;

			if(Modbus.com_config[port] != MODBUS_MASTER)
				return;

			crc_check = crc16(buf, 6); // crc16
			buf[6] = HIGH_BYTE(crc_check);
			buf[7] = LOW_BYTE(crc_check);

			set_baut_by_port(port,baut);
			uart_send_string(buf,8,port);

			uint8_t *subnet_response_buf = (uint8_t*)malloc(50);

			if(port == 0 || port == 2)
				length = uart_read_bytes(port, subnet_response_buf, 50, 10 / portTICK_RATE_MS);
			else
				length = 0;

			if(length > 0)
			{
				check_whether_modbus_slave(subnet_response_buf,length,port);
				if(port == 0)	{led_sub_rx++; flagLED_sub_rx = 1;}
						else if(port == 2)	{led_main_rx++; flagLED_main_rx = 1;}
				com_rx[port] += length;
				crc_check = crc16(subnet_response_buf,5);
				if((HIGH_BYTE(crc_check) == subnet_response_buf[5]) && (LOW_BYTE(crc_check) == subnet_response_buf[6]))
				{
					ptr->digital_analog = subnet_response_buf[4] + 100;
					flag_rmp_ad[index] = 1;
// Once find that the point is digital, check whether need to update the monitor points list
					{
						//monitor_init();
					}
				}
			}
			else
			{
				count_read_rmp_ad[index]++;
			}

			free(subnet_response_buf);
			if(count_read_rmp_ad[index] >= 5)
			{
				flag_rmp_ad[index] = 1;
				ptr->digital_analog = 0;
			}
		}
		else
		{// 1.2.mb_reg , dont need check ana_dig
			flag_rmp_ad[index] = 1;
		}

	}


}


#if 1//TEST
void read_name_of_tstat(U8_T index)
{
	U8_T port;
	U8_T baut;
	U8_T buf[8];
	U16_T crc_check;
	U16_T length;


	if(scan_db[index].product_model > CUSTOMER_PRODUCT) 
		return;
	buf[0] = scan_db[index].id;
	buf[1] = READ_VARIABLES;
	buf[2] = 0x02;
	buf[3] = 0xca; // start address 714 , len 9
	buf[4] = 0x00;
	buf[5] = 0x09;
	
	port = get_port_by_id(buf[0]);
	baut = get_baut_by_id(port,buf[0]);
	if(port == 0) 
	{  // wrong id
	    return;
	}
	
	port = port - 1;
	
	if(Modbus.com_config[port] != MODBUS_MASTER)
		return;
	
	crc_check = crc16(buf, 6); // crc16
	buf[6] = HIGH_BYTE(crc_check);
	buf[7] = LOW_BYTE(crc_check);
	
	set_baut_by_port(port,baut);
	uart_send_string(buf,8,port);


	uint8_t *subnet_response_buf = (uint8_t*)malloc(50);
	if(port == 0 || port == 2)
		length = uart_read_bytes(port, subnet_response_buf, 50, 10 / portTICK_RATE_MS);
	else
		length = 0;


	if(length > 0)
	{
		check_whether_modbus_slave(subnet_response_buf,length,port);
		if(port == 0)	{led_sub_rx++; flagLED_sub_rx = 1;}
				else if(port == 2)	{led_main_rx++; flagLED_main_rx = 1;}
		com_rx[port] += length;
		crc_check = crc16(subnet_response_buf,21);
		if((HIGH_BYTE(crc_check) == subnet_response_buf[21]) && (LOW_BYTE(crc_check) == subnet_response_buf[22]))
		{
//			auto_check_master_retry[port] = 0;
			if(subnet_response_buf[3] * 256 + subnet_response_buf[4] == 0x56)
			{
				memcpy(tstat_name[index],&subnet_response_buf[5],16);
			}
			else
			{
				memset(tstat_name[index],0,16);	
			}				
			flag_tstat_name[index] = 1;
			
			// add id to online
			tst_retry[buf[0]] = 0;
			scan_db_time_to_live[buf[0]] = SCAN_DB_TIME_TO_LIVE;
			if((current_online[buf[0] / 8] & (1 << (buf[0] % 8))) == 0)
			{
				U8_T add_i;
				if(get_index_by_id(buf[0],&add_i) == 1)
				{				
					if(scan_db[add_i].product_model < CUSTOMER_PRODUCT)
						add_id_online(add_i);
				}
			}
			
		}
		else
			packet_error[port]++;
	}
	else
	{		
		timeout[port]++;		
		tst_retry[buf[0]]++;
//		auto_check_master_retry[port]++;
	}	

	if(tst_retry[buf[0]] >= 10)	
	{  		
		U8_T remove_i;
 		// get index form scan_db
		if(get_index_by_id(buf[0],&remove_i) == 1)
		{			
			if(scan_db[remove_i].product_model < CUSTOMER_PRODUCT)
			{
				remove_id_online(remove_i);
			}
		}
	}
	
	free(subnet_response_buf);

}

void check_read_tstat_name(void)
{
	U8_T i;
	for(i = 0;i < sub_no;i++)
	{
		//if((current_online[scan_db[i].id / 8] & (1 << (scan_db[i].id % 8))))	  	 // on_line
		{
			if(count_read_tstat_name[i] > 0)
				count_read_tstat_name[i]--;
			else
			{
				flag_tstat_name[i] = 0;
				// if exist many devices, slow down check speed
				count_read_tstat_name[i] = READ_TSTAT_COUNT * sub_no;
			}
		}
	}
}
#endif
// change config and bautrate if auto scan
void Change_com_config(U8_T port)
{
	U8_T temp_baut;

// if discover device in this port by hands, change config
	if(Modbus.com_config[port] != MODBUS_MASTER)
	{
		Modbus.com_config[port] = MODBUS_MASTER;		
		//E2prom_Write_Byte(EEP_COM0_CONFIG + port, Modbus.com_config[port]);
		Count_com_config();
	}
	
	if(port == 0)
	{		
		//E2prom_Read_Byte(EEP_UART0_BAUDRATE,&temp_baut);
		if(temp_baut != Modbus.baudrate[0])
			;//E2prom_Write_Byte(EEP_UART0_BAUDRATE, Modbus.baudrate[0]);
	}
	
	if(port == 1)
	{		
		//E2prom_Read_Byte(EEP_UART1_BAUDRATE,&temp_baut);
		if(temp_baut != Modbus.baudrate[1])
			;//E2prom_Write_Byte(EEP_UART1_BAUDRATE, Modbus.baudrate[1]);
	}

}


//STR_T3000  T3000_Private;

#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI)
Str_CONFILCT_ID far	id_conflict[MAX_ID_CONFILCT];
U8_T index_id_conflict;


void Record_conflict_ID(U8_T id, U32_T oldsn,U32_T newsn,U8_T port,U8_T product)
{
	if(index_id_conflict < MAX_ID_CONFILCT - 1)
	{
		index_id_conflict++;
		id_conflict[index_id_conflict].id = id;
		id_conflict[index_id_conflict].sn_old = oldsn;
		id_conflict[index_id_conflict].sn_new = newsn;
	//	conflict_num = 1;
		id_conflict[index_id_conflict].port = port;
		id_conflict[index_id_conflict].product = product;
		id_conflict[index_id_conflict].count = 0;
	}
}

void Check_whether_clear_conflict_id(void)
{
	U16_T i,j;
	for(i = 0;i < index_id_conflict;i++)
	{
		if(id_conflict[i].count++ > 300)
		{// ɾ����ͻ��Ϣ
			memcpy(&id_conflict[i],&id_conflict[index_id_conflict],sizeof(Str_CONFILCT_ID));
			memset(&id_conflict[index_id_conflict],0,sizeof(Str_CONFILCT_ID));
			index_id_conflict--;
		}
	}
}

void clear_conflict_id(void)
{
//	conflict_num = 0;
//	conflict_id = 0;
//	conflict_sn_old = 0;
//	conflict_sn_new = 0;
//	conflict_port = 0;
	index_id_conflict = 0;
	memset(id_conflict,0,sizeof(Str_CONFILCT_ID) * MAX_ID_CONFILCT);
}
#endif
// check per 100 ms
// do not call it when transfer tcp to rs485
// 1 min offline, delete it from database
void Check_scan_db_time_to_live(void)
{
	U8_T i,j;
	for(i = 0;i < sub_no;i++)
	{		
		if(scan_db_time_to_live[scan_db[i].id]-- < 0)
		{
			db_online[scan_db[i].id / 8] &= ~(1 << (scan_db[i].id % 8));
			db_occupy[scan_db[i].id / 8] &= ~(1 << (scan_db[i].id % 8));

			for(j = i; j < sub_no; j++)
			{
				scan_db[j].id = scan_db[j + 1].id;
				scan_db[j].sn = scan_db[j + 1].sn;
				scan_db[j].port	= scan_db[j + 1].port;
				scan_db[j].product_model = scan_db[j + 1].product_model;

			}

			scan_db[j + 1].id = 0;
			scan_db[j + 1].sn = 0;
			scan_db[j + 1].port = 0;
			scan_db[j + 1].product_model = 0;

			if(db_ctr > 0)	db_ctr--;
			
			recount_sub_addr();
			
			//ChangeFlash = 1;
			scan_db_time_to_live[scan_db[i].id] = 0;
			return;
		}
	}
}


