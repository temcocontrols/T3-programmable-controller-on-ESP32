#ifndef __MODBUS_H
#define	__MODBUS_H
#include <string.h>
//IO操作函数	 
#include "bitmap.h"
#include "crc.h"


#define TXEN		PAout(8)


#define SEND			1			//1
#define	RECEIVE		0

#define	READ_VARIABLES				0x03
#define	WRITE_VARIABLES				0x06
#define	MULTIPLE_WRITE				0x10
#define	CHECKONLINE					0x19

#define DATABUFLEN					200
#define DATABUFLEN_SCAN				12
#define SENDPOOLLEN         		8



#define SERIAL_COM_IDLE				0
#define INVALID_PACKET				1
#define VALID_PACKET				2

#define USART_REC_LEN  			512  	//定义最大接收字节数 200
#define USART_SEND_LEN			512

extern u8 USART_RX_BUF[USART_REC_LEN];  //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u16 USART_RX_STA;         		//接收状态标记	
extern vu8 revce_count ;
extern u8 dealwithTag ;

void serial_restart(void);
void modbus_data_cope(u8 XDATA* pData, u16 length, u8 conn_id) ;
void modbus_init(void) ;



extern vu8 serial_receive_timeout_count ;
 void dealwithData(void) ;

void send_byte(u8 ch, u8 crc) ;
void USART_SendDataString(u8 num) ;
extern u8 uart_send[USART_SEND_LEN] ;
extern u8 SERIAL_RECEIVE_TIMEOUT ;
extern u8  	Station_NUM;
#endif
