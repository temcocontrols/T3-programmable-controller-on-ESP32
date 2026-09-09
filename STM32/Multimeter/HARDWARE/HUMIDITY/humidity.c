
//#include "config.h"
#include "myiic.h"
#include "sht3x.h"

uint8 point_num = 0; 
bit serial_int_flag = 0;
uint8 humidity_version;
STR_HUMIDITY HumSensor; 
uint8 hum_heat_status = 0;

int16 external_operation_value = 0;
uint8 external_operation_flag = 0;
uint8 current_i2c = 0;


#define SDA			PCout(12)	//SDA	
#define SCL			PCout(11)	//SCL
//#define IIC_SCL		PAout(3)	//SCL
//#define IIC_SDA		PAout(2)	//SDA	 
//#define READ_SDA	PAin(2)		//输入SDA 
//IIC所有操作函数 
void i2c_pic_start(void)
{
	SDA_OUT(); 
	IIC_SDA_LO();
	delay_us(30);
	IIC_SCL_LO();
	delay_us(30);
	
	// reset bus
	IIC_SDA_HI();
	IIC_SCL_HI();
	delay_us(30);
	
	// 2nd start condition
	IIC_SDA_LO();
	delay_us(30);
	IIC_SCL_LO();

}


u8 GET_ACK(void)
{
	bit c = 0;
	uint8 i = 0;
	SDA_IN();			//SDA设置为输入
	delay_us(10);
	for (i = 0; i < 10; i++)
	{
		c = READ_SDA();
		if(c == 0)
		{
			// if data line is low, pulse the clock.
			IIC_SCL_HI();
			delay_us(50);
			IIC_SCL_LO();
			return 0;
		}
		delay_us(2);
	}
	IIC_SCL_LO();
	return 1;
}
