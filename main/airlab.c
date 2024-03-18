#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "types.h"
#include "define.h"
#include "driver/uart.h"
#include "airlab.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "modbus.h"
#include "i2c_task.h"
#include "user_data.h"


extern STR_Task_Test task_test;

//sensirion PM2.5 sensor
enum{
SENSIRION_NULL,
SENSIRION_START_MEASUREMENT,
SENSIRION_STOP_MEASUREMENT,
SENSIRION_READ_MEASUREMENT,
SENSIRION_READ_AUTO_CLEAN,
SENSIRION_DISABLE_AUTO_CLEAN,
SENSIRION_START_FAN_CLEAN,
SENSIRION_RESET
};

#define PACKAGE_END  70
#define UG_M3   0
#define NUM_CM3   1
#define DATA_OFFSET  5

uint8 pm25_current_cmd = SENSIRION_NULL;
uint8 sensirion_rev_end = 0;

uint8 pm25_unit = 0;
uint8 DEGCorF;

uint8 const *p_pm25_cmd;

volatile uint16 disp_pm25_weight_25 = 0;
volatile uint16 disp_pm25_number_25 = 0;
uint16 pm25_number_05 = 0;
uint16 pm25_number_10 = 0;
volatile uint16 pm25_number_25 = 0;
uint16 pm25_number_40 = 0;
volatile uint16 pm25_number_100 = 0;
uint16 pm25_weight_10 = 0;
volatile uint16 pm25_weight_25 = 0;
uint16 pm25_weight_40 = 0;
volatile uint16 pm25_weight_100 = 0;
uint16 typical_partical_size = 0;

u8 uart_sendC[11];

//CDM FORMAT: Start, Adr, Cmd, Length, DataL, Check, Stop 
uint8 const PM25_CMD_START_MEASUREMENT[8] = 	{0x7E, 0x00, 0x00, 0x02, 0x01, 0x03, 0xF9, 0x7E};
uint8 const PM25_CMD_STOP_MEASUREMENT[6] = 		{0x7E, 0x00, 0x01, 0x00, 0xFE, 0x7E};
uint8 const PM25_CMD_READ_MEASUREMENT[6] =	 	{0x7E, 0x00, 0x03, 0x00, 0xFC, 0x7E};
uint8 const PM25_CMD_READ_AUTO_CLEAN[8] = 		{0x7E, 0x00, 0x80, 0x01, 0x00, 0x7D, 0x5E, 0x7E};
uint8 const PM25_CMD_DISABLE_AUTO_CLEAN[11] =   {0x7E, 0x00, 0x80, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7A, 0x7E};
uint8 const PM25_CMD_START_FAN_CLEAN[6] = 		{0x7E, 0x00, 0x56, 0x00, 0xA9, 0x7E};
uint8 const PM25_CMD_RESET[6] = 				{0x7E, 0x00, 0xD3, 0x00, 0x2C, 0x7E};

uint8 sensirion_co2_cmd_ForcedCalibration[8] = {0x61,0x06,0x00,0x39,0x00,0x00,0x00,0x00};


const uint8 Var_label[13][9] = {
	
	"Baudrate",   //0
	"StnNumer",   //1
	"Protocol", //2
	"Instance",//3
	"Unit", //10 
	"Trgger_S",
	"Timer_S",
	"Trgger_L",
	"Timer_L",
	"Trgger_O",
	"Timer_O",
	"Trgger_C",
	"Timer_C",

};
const uint8 Var_Description[13][21] = {
	
	"baudrate select",   	//0
	"station number",   	//1
	"modbus/bacnet switch",    //2
	"instance number",		//3
	"temprature unit",//10
	"Trigger of Sound",
	"Timer of Sound",
	"Trigger of light",
	"Timer of light",
	"Trigger of OCC",
	"Timer of OCC",
	"Trigger of CO2",
	"Timer of CO2",
};


uint8 AQI_area;
uint8 AQI_level;
uint16 AQI_value;
uint16 aqi_background_color;
uint16  aq_calibration;
uint8	air_cal_point[4];
uint16 	aq_level_value[4];

uint8 flag_refresh_PM25;
uint8 update_flag;

uint8 isBlankScreen;
uint8_t display_config[5];
uint8 pir_trigger;
uint8 sound_level;


//uint16 voc_value_raw;

uint32_t PirSensorZero = 2000;
uint32_t Pir_Sensetivity = 500;
uint16 pir_value = 0;


uint16 aqi_table_customer[5] = {12, 35, 55, 150, 250};

uint16 const aqi_table_china[501] =  { 0, 3 , 4 , 6 , 7 , 9 , 10 , 11 , 13 , 14 , 16 , 17 , 19 , 20 , 21 , 23 , 24 , 26 , 27 , 29 , 30 , 31 , 33 , 34 , 36 , 37 , 39 , 40 , 41 , 43 , 44 , 46 , 47 , 49 , 50, 51 , 53 , 54 , 55 , 56 , 58 , 59 , 60 , 61 , 63 , 64 , 65 , 66 , 68 , 69 , 70 , 71 , 73 , 74 , 75 , 76 , 78 , 79 , 80 , 81 , 83 , 84 , 85 , 86 , 88 , 89 , 90 , 91 , 93 , 94 , 95 , 96 , 98 , 99 , 100 , 101 , 103 , 104 , 105 , 106 , 108 , 109 , 110 , 111 , 113 , 114 , 115 , 116 , 118 , 119 , 120 , 121 , 123 , 124 , 125 , 126 , 128 , 129 , 130 , 131 , 133 , 134 , 135 , 136 , 138 , 139 , 140 , 141 , 143 , 144 , 145 , 146 , 148 , 149 , 150 , 151 , 153 , 154 , 156 , 157 , 159 , 160 , 161 , 163 , 164 , 166 , 167 , 169 , 170 , 171 , 173 , 174 , 176 , 177 , 179 , 180 , 181 , 183 , 184 , 186 , 187 , 189 , 190 , 191 , 193 , 194 , 196 , 197 , 199 , 200 , 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300, 300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314, 315, 316, 317, 318, 319, 320, 321, 322, 323, 324, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335, 336, 337, 338, 339, 340, 341, 342, 343, 344, 345, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 368, 369, 370, 371, 372, 373, 374, 375, 376, 377, 378, 379, 380, 381, 382, 383, 384, 385, 386, 387, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397, 398, 399, 400, 401, 402, 403, 403, 404, 405, 405, 406, 407, 407, 408, 409, 409, 410, 411, 411, 412, 413, 413, 414, 415, 415, 416, 417, 417, 418, 419, 419, 420, 420, 421, 422, 422, 423, 424, 424, 425, 426, 426, 427, 428, 428, 429, 430, 430, 431, 432, 432, 433, 434, 434, 435, 436, 436, 437, 438, 438, 439, 440, 440, 441, 442, 442, 443, 444, 444, 445, 446, 446, 447, 448, 448, 449, 450, 450, 451, 452, 452, 453, 454, 454, 455, 455, 456, 457, 457, 458, 459, 459, 460, 461, 461, 462, 463, 463, 464, 465, 465, 466, 467, 467, 468, 469, 469, 470, 471, 471, 472, 473, 473, 474, 475, 475, 476, 477, 477, 478, 479, 479, 480, 481, 481, 482, 483, 483, 484, 485, 485, 486, 487, 487, 488, 489, 489, 490, 490, 491, 492, 492, 493, 494, 494, 495, 496, 496, 497, 498, 498, 499, 500	 };	  

uint16 const aqi_table_usa[501] =  { 0, 4, 8, 13, 17, 21, 25, 29, 33, 38, 42, 46, 50, 53, 55, 57, 59, 61, 63, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 87, 89, 91, 93, 95, 97, 99, 102, 105, 107, 110, 112, 115, 117, 119, 122, 124, 127, 129, 132, 134, 137, 139, 142, 144, 147, 149, 151, 152, 152, 153, 153, 154, 154, 155, 155, 156, 156, 157, 157, 158, 158, 159, 160, 160, 161, 161, 162, 162, 163, 163, 164, 164, 165, 165, 166, 166, 167, 167, 168, 168, 169, 169, 170, 170, 171, 171, 172, 172, 173, 173, 174, 174, 175, 176, 176, 177, 177, 178, 178, 179, 179, 180, 180, 181, 181, 182, 182, 183, 183, 184, 184, 185, 185, 186, 186, 187, 187, 188, 188, 189, 189, 190, 190, 191, 192, 192, 193, 193, 194, 194, 195, 195, 196, 196, 197, 197, 198, 198, 199, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314, 315, 316, 317, 318, 319, 320, 321, 322, 323, 324, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335, 336, 337, 338, 339, 340, 341, 342, 343, 344, 345, 346, 347, 348, 349, 350, 351, 352, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 368, 369, 370, 371, 372, 373, 374, 375, 376, 377, 378, 379, 380, 381, 382, 383, 384, 385, 386, 387, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397, 398, 399, 400, 401, 402, 403, 403, 404, 405, 405, 406, 407, 407, 408, 409, 409, 410, 411, 411, 412, 413, 413, 414, 415, 415, 416, 417, 417, 418, 419, 419, 420, 420, 421, 422, 422, 423, 424, 424, 425, 426, 426, 427, 428, 428, 429, 430, 430, 431, 432, 432, 433, 434, 434, 435, 436, 436, 437, 438, 438, 439, 440, 440, 441, 442, 442, 443, 444, 444, 445, 446, 446, 447, 448, 448, 449, 450, 450, 451, 452, 452, 453, 454, 454, 455, 455, 456, 457, 457, 458, 459, 459, 460, 461, 461, 462, 463, 463, 464, 465, 465, 466, 467, 467, 468, 469, 469, 470, 471, 471, 472, 473, 473, 474, 475, 475, 476, 477, 477, 478, 479, 479, 480, 481, 481, 482, 483, 483, 484, 485, 485, 486, 487, 487, 488, 489, 489, 490, 490, 491, 492, 492, 493, 494, 494, 495, 496, 496, 497, 498, 498, 499, 500	 }; 

void uart_send_string(U8_T *p, U16_T length,U8_T port);

#define AQI_INDEX_RNG				500 

#define LEVEL0			50		//AQI:   0 ~ 50
#define LEVEL1			100		//AQI:  50 ~ 99
#define LEVEL2			150		//AQI:  99 ~ 149	
#define LEVEL3			200		//AQI: 149 ~ 200
#define LEVEL4			300		//AQI: 200 ~ 300
#define LEVEL5			500		//AQI: 300 ~ 500



float Datasum(uint8 FloatByte1, uint8 FloatByte2, uint8 FloatByte3, uint8 FloatByte4)
{   
	float aa;
//	uint8 Sflag;
	uint16 Evalue;
	uint32 Mvalue;
	uint32 Mtemp;
	float mfloat = 0;
	uint32 Etemp;
	uint8 i;
	
//	Sflag = 0x01& (FloatByte1 >> 7);//indicate it is positive or negative value
	
	Evalue = FloatByte1 & 0x7f;
	Evalue = Evalue<<1;
	if((FloatByte2 & 0x80) == 0x80)
		Evalue = Evalue | 0x01 ;
	else
		Evalue = Evalue & 0xfe ;
	
	Mvalue = FloatByte2 & 0x7f;
	Mvalue = (Mvalue << 16);
	Mvalue |= ((uint16)FloatByte3 << 8);
	Mvalue |= FloatByte4;
	

	for(i=0;i<23;i++)
	{
		Mtemp = (Mvalue >> i) & 0x01;
		if(Mtemp != 0)
			mfloat += (float)1/(Mtemp << (23-i));		
	}

	Etemp =  0x01 << (Evalue - 127);
	
	aa = (float)Etemp * (1 + mfloat);
	return aa;
}

void pm25_send_cmd(uint8 command)
{
	switch(command)
	{
		case SENSIRION_START_MEASUREMENT:
			uart_send_string(PM25_CMD_START_MEASUREMENT,8,2);
			break;		
		case SENSIRION_STOP_MEASUREMENT:
			uart_send_string(PM25_CMD_STOP_MEASUREMENT,6,2);
			break;			
		case SENSIRION_READ_MEASUREMENT:
			uart_send_string(PM25_CMD_READ_MEASUREMENT,6,2);
			break;		
		case SENSIRION_READ_AUTO_CLEAN:
			uart_send_string(PM25_CMD_READ_AUTO_CLEAN,8,2);
			break;		
		case SENSIRION_DISABLE_AUTO_CLEAN:
			uart_send_string(PM25_CMD_DISABLE_AUTO_CLEAN,11,2);
			break;		
		case SENSIRION_START_FAN_CLEAN:
			uart_send_string(PM25_CMD_START_FAN_CLEAN,6,2);
			break;		
		case SENSIRION_RESET:	
			uart_send_string(PM25_CMD_RESET,6,2);
			break;
		default:
		break;
	
	}	
}

void get_aqi_value(uint16 PM_val, uint16 *AQI_val,uint8 *AQI_level)
{
	if(PM_val < AQI_INDEX_RNG) 
	{
		if(AQI_area == 1)
			*AQI_val = aqi_table_china[PM_val];
		else if(AQI_area == 2)
		{
			if(PM_val<aqi_table_customer[0])
			{
				*AQI_val = ((500/12) * PM_val) /10;
			}
			else if(PM_val<aqi_table_customer[1])
			{
				*AQI_val = (((PM_val-aqi_table_customer[0])*500)/(aqi_table_customer[1]-aqi_table_customer[0]))/10+50;
			}
			else if(PM_val<aqi_table_customer[2])
			{
				*AQI_val = (((PM_val-aqi_table_customer[1])*500)/(aqi_table_customer[2]-aqi_table_customer[1]))/10+100;
			}
			else if(PM_val<aqi_table_customer[3])
			{
				*AQI_val = (((PM_val-aqi_table_customer[2])*500)/(aqi_table_customer[3]-aqi_table_customer[2]))/10+150;
			}
			else if(PM_val<aqi_table_customer[4])
			{
				*AQI_val = (((PM_val-aqi_table_customer[3])*500)/(aqi_table_customer[4]-aqi_table_customer[3]))/10+200;
			}
			else
			{
				*AQI_val = (((PM_val-aqi_table_customer[4])*500)/(500-aqi_table_customer[4]))/10+300;
			}
		}
		else
			*AQI_val = aqi_table_usa[PM_val];
	}
	else 
		*AQI_val = PM_val;

	if(*AQI_val < LEVEL0) *AQI_level = GOOD;
	else if(*AQI_val < LEVEL1) *AQI_level = MODERATE;
	else if(*AQI_val < LEVEL2 ) *AQI_level = POOL_FOR_SOME;
	else if(*AQI_val < LEVEL3 ) *AQI_level = UNHEALTHY;
	else if(*AQI_val < LEVEL4) *AQI_level = MORE_UNHEALTHY;
	else   *AQI_level = HAZARDOUS; 


}


uint8 Process_Rece_Data(uint8 *p,uint8 rece_count)
{
	u8 check_sum;
	u8 i = 0;
	uint16 pm25_org, pm100_org;
	Str_points_ptr ptr;
	if(pm25_current_cmd == SENSIRION_READ_MEASUREMENT)
	{
		if(p[0] != 0x7E || p[2] != 0x03)
			return 0;
		
		if(pm25_unit == NUM_CM3)
		{
			pm25_org = (uint16)Datasum(p[DATA_OFFSET + 24],p[DATA_OFFSET + 25],p[DATA_OFFSET + 26],p[DATA_OFFSET + 27]);
			pm100_org = (uint16)Datasum(p[DATA_OFFSET + 32],p[DATA_OFFSET + 33],p[DATA_OFFSET + 34],p[DATA_OFFSET + 35]);
		}
		else
		{
			pm25_org = (uint16)(Datasum(p[DATA_OFFSET + 4],p[DATA_OFFSET + 5],p[DATA_OFFSET + 6],p[DATA_OFFSET + 7])*10);
			pm100_org = (uint16)(Datasum(p[DATA_OFFSET + 12],p[DATA_OFFSET + 13],p[DATA_OFFSET + 14],p[DATA_OFFSET + 15])*10);
		}

		check_sum = 0;
		for(i = 1;i < rece_count - 1;i++)
		{
			check_sum += p[i];
		}

		check_sum = ~check_sum;
		if(check_sum == p[rece_count - 1])
		{
			if(pm25_org != 0 && pm100_org != 0)
			{
				//pm25_sensor.pm25 = pm25_org;
				//pm25_sensor.pm10 = pm100_org;
				pm25_weight_10 = (uint16)Datasum(p[DATA_OFFSET + 0],p[DATA_OFFSET + 1],p[DATA_OFFSET + 2],p[DATA_OFFSET + 3]);
				pm25_weight_25 = (uint16)Datasum(p[DATA_OFFSET + 4],p[DATA_OFFSET + 5],p[DATA_OFFSET + 6],p[DATA_OFFSET + 7]);
				disp_pm25_weight_25	= pm25_weight_25;
				get_aqi_value(pm25_weight_25, &AQI_value,&AQI_level);
				pm25_weight_40 = (uint16)Datasum(p[DATA_OFFSET + 8],p[DATA_OFFSET + 9],p[DATA_OFFSET + 10],p[DATA_OFFSET + 11]);
				pm25_weight_100 = (uint16)Datasum(p[DATA_OFFSET + 12],p[DATA_OFFSET + 13],p[DATA_OFFSET + 14],p[DATA_OFFSET + 15]);

				disp_pm25_number_25 = pm25_weight_100;
//				pm25_weight_100 -= pm25_weight_40;
//				pm25_weight_40 -= pm25_weight_25;
//				pm25_weight_25 -= pm25_weight_10;

				pm25_number_05 = (uint16)Datasum(p[DATA_OFFSET + 16],p[DATA_OFFSET + 17],p[DATA_OFFSET + 18],p[DATA_OFFSET + 19]);
				pm25_number_10 = (uint16)Datasum(p[DATA_OFFSET + 20],p[DATA_OFFSET + 21],p[DATA_OFFSET + 22],p[DATA_OFFSET + 23]);
				pm25_number_25 = (uint16)Datasum(p[DATA_OFFSET + 24],p[DATA_OFFSET + 25],p[DATA_OFFSET + 26],p[DATA_OFFSET + 27]);
				//disp_pm25_number_25 = pm25_number_25;
				//bac_input[6] = pm25_number_25;
				pm25_number_40 = (uint16)Datasum(p[DATA_OFFSET + 28],p[DATA_OFFSET + 29],p[DATA_OFFSET + 30],p[DATA_OFFSET + 31]);
				pm25_number_100 = (uint16)Datasum(p[DATA_OFFSET + 32],p[DATA_OFFSET + 33],p[DATA_OFFSET + 34],p[DATA_OFFSET + 35]);
				typical_partical_size	= (uint16)Datasum(p[DATA_OFFSET + 36],p[DATA_OFFSET + 37],p[DATA_OFFSET + 38],p[DATA_OFFSET + 39]);

//				pm25_number_100 -= pm25_number_40;
//				pm25_number_40 -= pm25_number_25;
//				pm25_number_25 -= pm25_number_10;
//				pm25_number_10 -= pm25_number_05;

				ptr = put_io_buf(IN,4);ptr.pin->value = pm25_weight_10 * 1000;
				ptr = put_io_buf(IN,5);ptr.pin->value = pm25_weight_25 * 1000;
				ptr = put_io_buf(IN,6);ptr.pin->value = pm25_weight_40 * 1000;
				ptr = put_io_buf(IN,7);ptr.pin->value = pm25_weight_100 * 1000;
				ptr = put_io_buf(IN,8);ptr.pin->value = pm25_number_05 * 1000;
				ptr = put_io_buf(IN,9);ptr.pin->value = pm25_number_10 * 1000;
				ptr = put_io_buf(IN,10);ptr.pin->value = pm25_number_25 * 1000;
				ptr = put_io_buf(IN,11);ptr.pin->value = pm25_number_40 * 1000;
				ptr = put_io_buf(IN,12);ptr.pin->value = pm25_number_100 * 1000;
				ptr = put_io_buf(IN,13);ptr.pin->value = typical_partical_size * 1000;
			}
		}

		return 1;
	}
	return 0;
}


// UART
void vPM25Task(void *pvParameters )
{
	uint8 count;
	uint8_t uart_rsv[60];
	//uart3_init(115200);
	pm25_current_cmd = SENSIRION_NULL;
	//	pm25_send_cmd(SENSIRION_START_MEASUREMENT);
	pm25_unit = NUM_CM3; // only for test, need to store it 
	task_test.enable[15] = 1;
	for( ;; )
	{
		task_test.count[15]++;
		int len = uart_read_bytes(UART_NUM_2, uart_rsv, 50, 20 / portTICK_RATE_MS);
		
		if(len > 0)
		{
			if(pm25_current_cmd == SENSIRION_START_MEASUREMENT)
			{
				if((uart_rsv[0] == 0x7e) && (uart_rsv[5] == 0xff))
				{
					pm25_current_cmd = SENSIRION_READ_MEASUREMENT;
				}
				else if((uart_rsv[3] == 0x43) && (uart_rsv[5] == 0xbc))
				{
					pm25_current_cmd = SENSIRION_READ_MEASUREMENT;								
				}
			}
			else if(pm25_current_cmd == SENSIRION_READ_MEASUREMENT)
			{				
				Process_Rece_Data(uart_rsv,len - 1);
			}
			
		}
		if((pm25_current_cmd == SENSIRION_NULL)||(pm25_current_cmd == SENSIRION_START_MEASUREMENT))
		{
			pm25_current_cmd = SENSIRION_START_MEASUREMENT;
			pm25_send_cmd(SENSIRION_START_MEASUREMENT);
		}
		else if(pm25_current_cmd == SENSIRION_READ_MEASUREMENT)
		{
			pm25_send_cmd(SENSIRION_READ_MEASUREMENT);
		}		
		
		vTaskDelay(3000 / portTICK_PERIOD_MS);
	}
	
}



// ADC
// 1. MIC
// 2. RS485—VOC
// 3. LIGHT
// 4. PIC sensor
void vInputTask( void *pvParameters )
{
	//static uint16 i = 0;
	uint8 j = 0;
/*	static uint8 occ_trigged = 0;

	inputs_init();
    mul_analog_cal[0] = ((int16)Calibration_AI1_HI<<8) + Calibration_AI1_LO;
	mul_analog_cal[1] = ((int16)Calibration_AI2_HI<<8) + Calibration_AI2_LO;
	mul_analog_cal[2] = ((int16)Calibration_AI3_HI<<8) + Calibration_AI3_LO;
	mul_analog_cal[3] = ((int16)Calibration_AI4_HI<<8) + Calibration_AI4_LO;
	mul_analog_cal[4] = ((int16)Calibration_AI5_HI<<8) + Calibration_AI5_LO;
	mul_analog_cal[5] = ((int16)Calibration_AI6_HI<<8) + Calibration_AI6_LO;
	mul_analog_cal[6] = ((int16)Calibration_AI7_HI<<8) + Calibration_AI7_LO;
	mul_analog_cal[7] = ((int16)Calibration_AI8_HI<<8) + Calibration_AI8_LO;
*/
	task_test.enable[7] = 1;
	for(;;)
	{			
		//CalInput();
		//control_input();
		task_test.count[7]++;
		for(j=0;j<8;j++)
		{

		}
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
		
}


uint16 voice_table[10][2] =
{
	{60,40},{63,80},{67,100},{70,120},{72,140},
	{74,160},{76,180},{78,200},{80,220},{83,250}
};

u8 check_voice_table(uint16 adc)
{
	u8 i = 0;
	if(adc < voice_table[0][1])
		return 50;

	if(adc > voice_table[9][1])
		return 90;

	for(i = 0;i < 9;i++)
	{
		if((adc >= voice_table[i][1]) && (adc < voice_table[i + 1][1]))
		{
			return voice_table[i][0] +
				(voice_table[i + 1][0] - voice_table[i][0]) * (adc - voice_table[i][1]) / (voice_table[i + 1][1] - voice_table[i][1]);
		}
	}
	return 0;
}

static esp_adc_cal_characteristics_t *Airlab_adc_chars;
static const adc_atten_t Airlab_atten = ADC_ATTEN_DB_11;
static const adc_unit_t Airlab_unit = ADC_UNIT_1;
static const adc_channel_t Airlab_transducer_channel_1 = ADC_CHANNEL_3;  // light
static const adc_channel_t Airlab_transducer_channel_2 = ADC_CHANNEL_0; //  occ
static const adc_channel_t Airlab_transducer_channel_3 = ADC_CHANNEL_7;  // voice
static const adc_channel_t Airlab_transducer_channel_4 = ADC_CHANNEL_4;  // rs485_vol

static void Airlab_adc_task(void* arg);
void Airlab_adc_init(void)
{
    //Configure ADC
   // if (Airlab_unit == ADC_UNIT_1)
    {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(Airlab_transducer_channel_1, Airlab_atten);
        adc1_config_channel_atten(Airlab_transducer_channel_2, Airlab_atten);
        adc1_config_channel_atten(Airlab_transducer_channel_3, Airlab_atten);
        adc1_config_channel_atten(Airlab_transducer_channel_4, Airlab_atten);
    }

    xTaskCreate(Airlab_adc_task, "adc_task", 2048*2, NULL, 2, NULL);
}

static void Airlab_adc_task(void* arg)
{
	//uint32_t adc_reading = 0;
	//uint32_t adc_temp = 0;
	uint32_t voltage =0;
	uint32_t adc_light = 0;
	uint32_t adc_pir = 0;
	uint32_t adc_voice = 0;
	uint32_t adc_rs485 = 0;
	Str_points_ptr ptr;
	uint32_t temp_adc_voice;



	uint32_t vol_light =0;
	uint32_t vol_pir =0;
	uint32_t vol_voice =0;
	uint32_t vol_rs485 =0;
	
	int i = 0;
    //Continuously sample ADC1//Characterize ADC
	Airlab_adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_value_t val_type = esp_adc_cal_characterize(Airlab_unit, Airlab_atten, ADC_WIDTH_BIT_12, AIR_DEFAULT_VREF, Airlab_adc_chars);

    while (1) {
        //Multisampling
        for (i = 0; i < AIR_NO_OF_SAMPLES; i++) {
            if (Airlab_unit == ADC_UNIT_1) {
            	adc_light += adc1_get_raw((adc1_channel_t)Airlab_transducer_channel_1);
            	adc_pir += adc1_get_raw((adc1_channel_t)Airlab_transducer_channel_2);

            	temp_adc_voice = adc1_get_raw((adc1_channel_t)Airlab_transducer_channel_3);
            	if(temp_adc_voice > MIC_CARRIER_HI)
				{
					//count++;
					temp_adc_voice = temp_adc_voice - MIC_CARRIER_HI;
				}
				else if(temp_adc_voice < MIC_CARRIER_LO)
				{
					//count++;
					temp_adc_voice = MIC_CARRIER_LO - temp_adc_voice;
				}
				else
				{
					temp_adc_voice = 0;
				}
            	adc_voice += temp_adc_voice;


            	adc_rs485 += adc1_get_raw((adc1_channel_t)Airlab_transducer_channel_4);
            }
        }

        adc_light /= AIR_NO_OF_SAMPLES;
        adc_pir /= AIR_NO_OF_SAMPLES;
        adc_voice /= AIR_NO_OF_SAMPLES;
        adc_rs485 /= AIR_NO_OF_SAMPLES;
        
        vol_light = esp_adc_cal_raw_to_voltage(adc_light, Airlab_adc_chars);
        vol_pir = esp_adc_cal_raw_to_voltage(adc_pir, Airlab_adc_chars);
        vol_voice = esp_adc_cal_raw_to_voltage(adc_voice, Airlab_adc_chars);
        vol_rs485 = esp_adc_cal_raw_to_voltage(adc_rs485, Airlab_adc_chars);

        if(abs(vol_pir - PirSensorZero) > Pir_Sensetivity) //occupied
		{
			pir_trigger = PIR_TRIGGERED;
		}
		else
		{
			pir_trigger = PIR_NOTTRIGGERED;
		}
        
        sound_level = check_voice_table(adc_voice);
        ptr = put_io_buf(IN,14);
        ptr.pin->value = sound_level * 1000;
        ptr.pin->value = pir_trigger;

        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}


uint16_t read_airlab_by_block(uint16_t addr)
{
	uint8_t item;
	uint16_t *block;
	uint8_t *block1;
	uint8_t temp;
	
	if(addr == DEGC_OR_F)
	{
	  return DEGCorF;
	}
	else if(addr == TEMPRATURE_CHIP)
	{
	  return g_sensors.temperature;
	}
	else if(addr == EXTERNAL_SENSOR1)  // CO2
	{
	  return g_sensors.co2;
	}
	else if(addr == EXTERNAL_SENSOR2) // HUM
	{
	  return g_sensors.humidity;
	}
	else if(addr == CALIBRATION)
	{
	  return 0;
	}
	else if(addr == CO2_CALIBRATION)
	{
	  return 0;
	}
	else if(addr == HUM_CALIBRATION)
	{
	  return 0;
	}
	// LIGHT
	else if(addr == LIGHT_SENSOR)
	{
	  return 0;
	}
	// PIR
	else if(addr == PIR_SENSOR_VALUE)
	{
	  return pir_value;
	}
	else if(addr == PIR_SENSOR_ZERO) 
	{
	  return PirSensorZero;
	}
	else if(addr == PIR_SPARE)
	{
	  return pir_trigger;
	}
	// PM2.5
	else if(addr == MODBUS_PM25_WEIGHT_1_0)
	{
	  return pm25_weight_10;
	}
	else if(addr == MODBUS_PM25_WEIGHT_2_5)
	{
	  return pm25_weight_25;
	}	
	else if(addr == MODBUS_PM25_WEIGHT_4_0)
	{
	  return pm25_weight_40;
	}
	else if(addr == MODBUS_PM25_WEIGHT_10)
	{
	  return pm25_weight_100;
	}
	else if(addr == MODBUS_PM25_NUMBER_0_5)
	{
	  return pm25_number_05;
	}
	else if(addr == MODBUS_PM25_NUMBER_1_0)
	{
	  return pm25_number_10;
	}	
	else if(addr == MODBUS_PM25_NUMBER_2_5)
	{
	  return pm25_number_25;
	}
	else if(addr == MODBUS_PM25_NUMBER_4_0)
	{
	  return pm25_number_40;
	}
	else if(addr == MODBUS_PM25_NUMBER_10)
	{
	  return pm25_number_100;
	}
	else if(addr == MODBUS_PM25_TOTAL)
	{
	  return disp_pm25_weight_25;
	}
	else if(addr == MODBUS_PM10_TOTAL)
	{
	  return disp_pm25_number_25;
	}
// VOC	
	else if((addr >= VOC_BASELINE1) && (addr <= VOC_BASELINE1))
	{
	  return g_sensors.voc_baseline[addr - VOC_BASELINE1];
	}
	else if(addr == VOC_DATA)
	{
	  return g_sensors.voc_value;
	}	
	else if(addr == MODBUS_PATICAL_SIZE)
	{
	  return 0;
	}
	else if(addr == MODBUS_WBGT)
	{
	  return 0;
	}
	// AQI
/*	else if(addr == MODBUS_CELBRA_AIR1)
	{
	  return air_cal_point[0];
	}
	else if(addr == MODBUS_CELBRA_AIR2)
	{
	  return air_cal_point[1];
	}
	else if(addr == MODBUS_CELBRA_AIR3)
	{
	  return air_cal_point[2];
	}
	else if(addr == MODBUS_CELBRA_AIR4)
	{
	  return air_cal_point[3];
	}*/
	else if(addr == MODBUS_AQ_LEVEL0)
	{
	  return aq_level_value[0];
	}
	else if(addr == MODBUS_AQ_LEVEL1)
	{
	  return aq_level_value[1];
	}
	else if(addr == MODBUS_AQ_LEVEL2)
	{
	  return aq_level_value[2];
	}
	else if(addr == MODBUS_MAX_AQ_VAL)
	{
	  return aq_level_value[3];
	}
	else if(addr == MODBUS_CALIBRATION_AQ)
	{
	  return aq_calibration;
	}
	else if(addr == MODBUS_AQI)
	{
	  return AQI_value;
	}
	else if(addr == MODBUS_AQI_LEVEL)
	{
	  return AQI_level;
	}
	else if(addr == MODBUS_AQI_AREA)
	{
	  return AQI_area;
	}
	else if((addr >= MODBUS_AQI_FIRST_LINE) && (addr <= MODBUS_AQI_FIFTH_LINE))
	{
	  return aqi_table_customer[addr - MODBUS_AQI_FIRST_LINE];
	}
	
	// CO2
	else if(addr == CO2_ASC_ENABLE)
	{
	  return co2_asc;
	}
	else if(addr == CO2_FRC_VALUE)
	{
	  return co2_frc;
	}
/*	else if(addr == MODBUS_CO2_BKCAL_ONOFF)
	{
	  return co2_bkcal_onoff;
	}
	else if(addr == MODBUS_CO2_NATURE_LEVEL)
	{
	  return co2_level;
	}
	else if(addr == MODBUS_CO2_MIN_ADJ)
	{
	  return min_co2_adj;
	}
	else if(addr == MODBUS_CO2_CAL_DAYS)
	{
	  return co2_bkcal_day;
	}
	else if(addr == MODBUS_CO2_LOWVALUE_REMAIN_TIME)
	{
	  return value_keep_time;
	}
	else if(addr == MODBUS_CO2_BKCAL_VALUE)
	{
	  return co2_bkcal_value;
	}
	else if(addr == MODBUS_CO2_LOWVALUE)
	{
	  return co2_lowest_value;
	}*/
	// DISPLAY CONFIG
	else if((addr >= MODBUS_DISPLAY_CONFIG1) && (addr <= MODBUS_DISPLAY_CONFIG5))
	{
	  return display_config[addr - MODBUS_DISPLAY_CONFIG1];
	}
	else if(addr == MODBUS_IS_BLANK_SCREEN)
	{
	  return isBlankScreen;
	}
	else
	  return 0;
}



void write_airlab_by_block(uint16_t addr,uint8_t HeadLen,uint8_t *pData,uint8_t type)
{
	if(addr == DEGC_OR_F)
	{
		DEGCorF = pData[HeadLen + 5];
	}
	
	else if(addr == CALIBRATION)
	{
	  
	}
	else if(addr == CO2_CALIBRATION)
	{
	  
	}
	else if(addr == HUM_CALIBRATION)
	{
	  
	}
	
	// PIR
	else if(addr == PIR_SENSOR_VALUE)
	{
	  
	}
	else if(addr == PIR_SENSOR_ZERO) 
	{
	  PirSensorZero = pData[HeadLen + 5];
	}
	
// PM2.5
	
// VOC	
	
	// AQI
	else if(addr >= MODBUS_AQ_LEVEL0 && addr <= MODBUS_MAX_AQ_VAL)
	{
	  aq_level_value[addr - MODBUS_AQ_LEVEL0] = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_CALIBRATION_AQ)
	{
	  aq_calibration = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	}
	else if(addr == MODBUS_AQI_AREA)
	{
	  AQI_area = pData[HeadLen + 5];
	}
	else if((addr >= MODBUS_AQI_FIRST_LINE) && (addr <= MODBUS_AQI_FIFTH_LINE))
	{
	  if((pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8)) < 500)
	  {
		aqi_table_customer[addr -MODBUS_AQI_FIRST_LINE] = pData[HeadLen + 5]+ (pData[HeadLen + 4]<<8);
	  }
	}
	
	// CO2	
	else if(addr == CO2_FRC_VALUE)
	{
		uint16 tmp;
		sensirion_co2_cmd_ForcedCalibration[4] = pData[HeadLen + 4];	
		sensirion_co2_cmd_ForcedCalibration[5] = pData[HeadLen + 5];
		tmp = (uint16)(sensirion_co2_cmd_ForcedCalibration[4]<<8 )|sensirion_co2_cmd_ForcedCalibration[5];
		if((tmp >= 0) && (tmp <= 5000))
		{
			co2_frc = tmp;
			/*if(co2_present == SCD30)
			{
				co2_cmd_status = CMD_SET_FRC;
			}
			else*/
			{
				if(scd4x_perform_forced == 0)
				{
					scd4x_perform_forced = 1;
					//scd4x_perform_forced_count = 0;
				}
			}					
		}
	}
	// DISPLAY CONFIG
	else if((addr >= MODBUS_DISPLAY_CONFIG1) && (addr <= MODBUS_DISPLAY_CONFIG5))
	{
		display_config[addr - MODBUS_DISPLAY_CONFIG1] = pData[HeadLen + 5];		
	}
	else if(addr == MODBUS_IS_BLANK_SCREEN)
	{
		if((pData[HeadLen + 5] == 0) || (pData[HeadLen + 5]==1))
		{
			isBlankScreen= pData[HeadLen + 5];	
			//SoftReset();
		}
	}
}


void Get_AVS(void)
{
	Str_points_ptr ptr;
	uint32 baud;
	switch(Modbus.baudrate[0])
	{
	case UART_1200:
		baud = 1200;
		break;
	case UART_2400:
		baud = 2400;
		break;
	case UART_3600:
		baud = 3600;
		break;
	case UART_4800:
		baud = 4800;
		break;
	case UART_7200:
		baud = 7200;
		break;
	case UART_9600:
		baud = 9600;
		break;
	case UART_19200:
		baud = 19200;
		break;
	case UART_38400:
		baud = 38400;
		break;
	case UART_57600:
		baud = 57600;
		break;
	case UART_76800:
		baud = 76800;
		break;
	case UART_115200:
		baud = 115200;
		break;
	case UART_921600:
		baud = 921600;
		break;
	default:
		baud = 115200;
		break;
	}
	ptr = put_io_buf(VAR,0);ptr.pvar->value = baud;
	ptr = put_io_buf(VAR,1);ptr.pvar->value = Station_NUM;
	ptr = put_io_buf(VAR,2);ptr.pvar->value = Modbus.com_config[0];
	ptr = put_io_buf(VAR,3);ptr.pvar->value = (uint32)Instance;
	ptr = put_io_buf(VAR,4);ptr.pvar->value = DEGCorF;
	  
}
