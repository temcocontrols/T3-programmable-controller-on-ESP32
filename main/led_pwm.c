#include <stdio.h>
#include "led_pwm.h"
#include "driver/ledc.h"
//#include "deviceparams.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "wifi.h"
#include "driver/pcnt.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "controls.h"
#include "define.h"
#include "i2c_task.h"
#include <math.h>
#include "flash.h"
#include "scan.h"

#define LEDC_HS_TIMER          LEDC_TIMER_0
#define LEDC_HS_MODE           LEDC_HIGH_SPEED_MODE
#define LEDC_HS_CH0_GPIO       (25)
#define LEDC_HS_CH0_CHANNEL    LEDC_CHANNEL_0
#define LEDC_HS_CH1_GPIO       (26)
#define LEDC_HS_CH1_CHANNEL    LEDC_CHANNEL_1
#define LEDC_HS_CH2_CHANNEL	   LEDC_CHANNEL_2
#define LEDC_HS_CH3_CHANNEL	   LEDC_CHANNEL_3
#define LEDC_HS_CH4_CHANNEL	   LEDC_CHANNEL_4
#define LEDC_HS_CH5_CHANNEL	   LEDC_CHANNEL_5


#define LEDC_TEST_CH_NUM       (2)
#define LEDC_TEST_DUTY         (100)
#define LEDC_TEST_FADE_TIME    (3000)

#define LED_HEART_BEAT	23
#define LED_FAN_SPEED	4
#define LED_WIFI		22
#define	LED_RS485_TX	19
#define LED_RS485_RX	18
#define LED_HEART_BEAT_SEL  (1ULL<<LED_HEART_BEAT)
#define LED_FAN_SPEED_SEL  (1ULL<<LED_FAN_SPEED)
#define LED_WIFI_SEL  		(1ULL<<LED_WIFI)
#define LED_RS485_TX_SEL  (1ULL<<LED_RS485_TX)
#define LED_RS485_RX_SEL  (1ULL<<LED_RS485_RX)

#define TRANSDUCER_SWITCH_1	23
#define	TRANSDUCER_SWITCH_2	22
#define TRANSDUCER_SWITCH_3	21
#define TRANSDUCER_SWITCH_4	5
#define TRANSDUCER_SWITCH_5	15
#define TRANSDUCER_SWITCH_6	27
#define TRANSDUCER_SWTICH_1_SEL	(1ULL<<TRANSDUCER_SWITCH_1)
#define TRANSDUCER_SWTICH_2_SEL	(1ULL<<TRANSDUCER_SWITCH_2)
#define TRANSDUCER_SWTICH_3_SEL	(1ULL<<TRANSDUCER_SWITCH_3)
#define TRANSDUCER_SWTICH_4_SEL	(1ULL<<TRANSDUCER_SWITCH_4)
#define TRANSDUCER_SWTICH_5_SEL	(1ULL<<TRANSDUCER_SWITCH_5)
#define TRANSDUCER_SWTICH_6_SEL	(1ULL<<TRANSDUCER_SWITCH_6)

#define PULSE_COUNTER	5
#define PULSE_COUNTER_SEL	(1ULL<<PULSE_COUNTER)

#define ESP_INTR_FLAG_DEFAULT 0

#define LEDC_TRANSDUCER_CH_NUM	(3)
#define LEDC_TRANSDUCER_CH0_GPIO       (32)
#define LEDC_TRANSDUCER_CH1_GPIO       (33)
#define LEDC_TRANSDUCER_CH2_GPIO       (25)

static void pcnt_task(void* arg);
static void adc_task(void* arg);
//static void periodic_timer_callback(void* arg);

#define DEFAULT_VREF    3300        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          //Multisampling

#define MIDDLE_RANGE     8
#define NO_TABLE_RANGES 16

EXT_RAM_ATTR holding_reg_params_t holding_reg_params = {0};
const int16_t led_limit[10][2] = { { -400, 1500 }, { -400, 3020 },
                            { -400, 1200 }, { -400, 2480 },
                            { -400, 1200 }, { -400, 2480 },
                            { -400, 1200 }, { -400, 2480 },
                            { -500, 1100 }, { -580, 2300 }
                      };

uint16_t  my_def_tab[5][17] = {
 /* 3k termistor YSI44005 -40 to 150 Deg.C or -40 to 302 Deg.F */
	{ 233*4,  211*4, 179*4, 141*4, 103*4, 71*4, 48*4, 32*4,
		21*4, 14*4, 10*4, 7*4, 5*4, 4*4, 3*4, 2*4, 1*4 },

 /* 10k termistor GREYSTONE -40 to 120 Deg.C or -40 to 248 Deg.F */  // type2
	{ 3918, 3818, 3650, 3408, 3050, 2670, 2218, 1772,
	    1362, 1040, 758, 550, 406, 283, 200, 137, 91 },

 /* 3k termistor GREYSTONE -40 to 120 Deg.C or -40 to 248 Deg.F */
	{ 233*4, 215*4, 190*4, 160*4, 127*4, 96*4, 70*4, 50*4,
		35*4, 25*4, 18*4, 13*4, 9*4, 7*4, 5*4, 4*4, 3*4 },

 /* 10k termistor KM -40 to 120 Deg.C or -40 to 248 Deg.F */ // type3
	{ 994, 971, 935, 880, 798, 714, 612, 510,
		415, 339, 272, 224, 190, 161, 142, 127, 115 },

 /* 3k termistor AK -40 to 150 Deg.C or -40 to 302 Deg.F */
	{ 246*4, 238*4, 227*4, 211*4, 191*4, 167*4, 141*4, 115*4,
		92*4, 72*4, 55*4, 42*4, 33*4, 25*4, 19*4, 15*4, 12*4 }
};

const int16_t my_tab_int[10] = { 119, 214, 100, 180, 100, 180,100, 180, 100, 180 };
//typedef enum { not_used_input, Y3K_40_150DegC, Y3K_40_300DegF, R10K_40_120DegC,
 //R10K_40_250DegF, R3K_40_150DegC, R3K_40_300DegF, KM10K_40_120DegC,
 //KM10K_40_250DegF, A10K_50_110DegC, A10K_60_200DegF, V0_5, I0_100Amps,
 //I0_20ma, I0_20psi, N0_2_32counts, N0_3000FPM_0_10V, P0_100_0_5V,
 //P0_100_4_20ma/*, P0_255p_min*/, V0_10_IN, table1, table2, table3, table4,
 //table5, HI_spd_count = 100 } Analog_input_range_equate;

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_7;     //GPIO35 if ADC1
static const adc_channel_t channel_0 = ADC_CHANNEL_0;   //GPIO36 if ADC1
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;
static const adc_channel_t transducer_channel_1 = ADC_CHANNEL_3;
static const adc_channel_t transducer_channel_2 = ADC_CHANNEL_7;
static const adc_channel_t transducer_channel_3 = ADC_CHANNEL_6;

uint8_t gSwtich_output_mode=OUT_0_10_V;
uint8_t gSwtich_temp_range=TEMP_20_80;
uint8_t gSwitch_hum_range=HUMIDITY_TYPE;
uint8_t gSwtich_protocol_select=MODBUS_PROTOCAL;

uint8_t gIdentify = 0;
uint8_t count_gIdentify = 0;

static int16_t my_get_input_value_by_range( int range, uint16_t raw )
{
	int index;
	long val=0;
	int work_var;
	int ran_in;
	int delta = MIDDLE_RANGE;
	uint16_t *def_tbl;
	uint8_t end = 0;
	range--;
	ran_in = range;
	range >>= 1;
	def_tbl = ( uint16_t * )&my_def_tab[range];

	if( raw <= def_tbl[NO_TABLE_RANGES] )
		return led_limit[ran_in][1];
	if( raw >= def_tbl[0] )
		return led_limit[ran_in][0];
	index = MIDDLE_RANGE;

	while( !end )
	{
		if( ( raw >= def_tbl[index] ) && ( raw <= def_tbl[index-1] ) )
		{
			index--;
			delta = def_tbl[index] - def_tbl[index+1];
			if( delta )
			{
				work_var = (int)( ( def_tbl[index] - raw ) * 100 );
				work_var /= delta;
				work_var += ( index * 100 );
				val = my_tab_int[ran_in];
				val *= work_var;
				val /= 100;
				val += led_limit[ran_in][0];
			}
			return val;
		}
		else
		{
			if( !delta )
				end = 1;
			delta /= 2;
			if( raw < def_tbl[index] )
				index += delta;
			else
				index -= delta;
			if( index <= 0 )
				return led_limit[ran_in][0];
			if( index >= NO_TABLE_RANGES )
				return led_limit[ran_in][1];
		}
	}
        return 0;
}

void adc_init(void)
{
    //Configure ADC
    if (unit == ADC_UNIT_1) {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(channel, atten);
        adc1_config_channel_atten(channel_0, atten);
    }

    xTaskCreate(adc_task, "adc_task", 2048*2, NULL, 2, NULL);
}

static void adc_task(void* arg)
{
	uint32_t adc_reading = 0;
	uint32_t adc_temp = 0;
	uint32_t voltage =0;
	int i = 0;
    //Continuously sample ADC1//Characterize ADC
	adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);

    while (1) {
        //Multisampling
        for (i = 0; i < NO_OF_SAMPLES; i++) {
            if (unit == ADC_UNIT_1) {
                adc_reading += adc1_get_raw((adc1_channel_t)channel);
                adc_temp += adc1_get_raw((adc1_channel_t)channel_0);
            }
        }
        adc_reading /= NO_OF_SAMPLES;
        adc_temp /= NO_OF_SAMPLES;
        //Convert adc_reading to voltage in mV
        if(adc_reading<50)
        	voltage = 0;
        else
        	voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
        voltage = voltage*33/10;
        voltage = voltage*10/8;
        if(voltage>10000)
        	voltage = 10000;
        Test[2] = adc_temp;
        holding_reg_params.fan_module_10k_temp = my_get_input_value_by_range(R10K_40_120DegC, adc_temp);
        Test[3] = holding_reg_params.fan_module_10k_temp;
        //holding_reg_params.fan_module_10k_temp += holding_reg_params.temp_10k_offset;
        holding_reg_params.fan_module_input_voltage = (uint16_t)voltage;
        //inputs[2].range = 1;
        if(!inputs[2].calibration_sign)
        	holding_reg_params.fan_module_10k_temp += (inputs[2].calibration_hi * 256 + inputs[2].calibration_lo);
		else
			holding_reg_params.fan_module_10k_temp += -(inputs[2].calibration_hi * 256 + inputs[2].calibration_lo);
        if(inputs[2].range == 3)
        	inputs[2].value = holding_reg_params.fan_module_10k_temp*100;
        if(inputs[2].range == 4)
        	inputs[2].value = (holding_reg_params.fan_module_10k_temp*9/5)*100+32000;

        //inputs[3].range = 19;
        if(!inputs[3].calibration_sign)
			holding_reg_params.fan_module_input_voltage += (inputs[3].calibration_hi * 256 + inputs[3].calibration_lo);
		else
			holding_reg_params.fan_module_input_voltage += -(inputs[3].calibration_hi * 256 + inputs[3].calibration_lo);
        inputs[3].value = holding_reg_params.fan_module_input_voltage;



        vTaskDelay(1000 / portTICK_RATE_MS);//pdMS_TO_TICKS(1000));
    }
}


float SHT10_CalcuDewPoint(float t, float h)
{
	float logEx, dew_point;

	logEx = 0.66077 + 7.5 * t / (237.3 + t) + (log10(h) - 2);
	dew_point = ((0.66077 - logEx) * 237.3) / (logEx - 8.16077);

	return dew_point;
}

  //i=(1.01+1.84d)t+2500d;
//i=1.01t+(2500+1.84t)d
float SHT10_CalcuEnthalpy(float t, float h)
{
	float enthalpy, baoheWaterPress, realWaterPress, hanShiLiang;
	baoheWaterPress = 6.11*pow(10,((7.5*t)/(t+237.3)));
	realWaterPress = baoheWaterPress*h/100;
	hanShiLiang = 622*(realWaterPress/(1013.25-realWaterPress))/1000;
	enthalpy = 1.01*t+((2500+1.84*t)*hanShiLiang);//(1.84*h+1.01)*t+2500*h;
	return enthalpy;
}

float SHT10_CalcuAbsHumi(float t, float h)
{
	float absHumi, baoheWaterPress, realWaterPress;
	baoheWaterPress = 6.11*pow(10,((7.5*t)/(t+237.3)));
	realWaterPress = baoheWaterPress*h/100;
	absHumi = 217*realWaterPress/(t+273.15);
	return absHumi;
}

uint16_t tempDac, humDac, co2Dac;
static void transducer_output_task(void* arg)
{
	uint8_t gSwtich_range_1;
	uint8_t gSwtich_range_2;
	uint8_t gSwtich_range_3;
	uint8_t gSwtich_range_4;

	uint32_t adc_feedback1 = 0;
	uint32_t adc_feedback2 = 0;
	uint32_t adc_feedback3 =0;

	float temp_val_real,humi_val_real,dew_point,enthalpy,humi_val_abs;
	int i = 0;
	Test[6] = 2400;
	Test[7] = 390;
	Test[16] = 1580;
	Test[17] = 238;
    //Continuously sample ADC1//Characterize ADC
	adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);

	for(;;) {
		temp_val_real = ((float)g_sensors.temperature)/10;
		humi_val_real = ((float)g_sensors.humidity)/10;

		dew_point = SHT10_CalcuDewPoint(temp_val_real, humi_val_real);
		enthalpy = SHT10_CalcuEnthalpy(temp_val_real, humi_val_real);
		humi_val_abs = SHT10_CalcuAbsHumi(temp_val_real, humi_val_real);
		g_sensors.dewpoint = (int)(dew_point*10);
		g_sensors.enthalpy = (int)(enthalpy*10);
		g_sensors.absHumi = (int)(humi_val_abs*10);
        //Multisampling
        for (i = 0; i < NO_OF_SAMPLES; i++) {
            if (unit == ADC_UNIT_1) {
            	adc_feedback1 += adc1_get_raw((adc1_channel_t)transducer_channel_1);
            	adc_feedback2 += adc1_get_raw((adc1_channel_t)transducer_channel_2);
            	adc_feedback3 += adc1_get_raw((adc1_channel_t)transducer_channel_3);
            }
        }
        adc_feedback1 /= NO_OF_SAMPLES;
        adc_feedback2 /= NO_OF_SAMPLES;
        adc_feedback3 /= NO_OF_SAMPLES;

        Test[13] = adc_feedback1;
        Test[14] = adc_feedback2;
        Test[15] = adc_feedback3;

		gSwtich_range_1 = gpio_get_level(TRANSDUCER_SWITCH_6);
		gSwtich_range_2 = gpio_get_level(TRANSDUCER_SWITCH_5);
		gSwtich_range_3 = gpio_get_level(TRANSDUCER_SWITCH_4);
		gSwtich_range_4 = gpio_get_level(TRANSDUCER_SWITCH_3);

		gSwtich_output_mode = gpio_get_level(TRANSDUCER_SWITCH_2);
		gSwtich_protocol_select = gpio_get_level(TRANSDUCER_SWITCH_1);

		if(gSwtich_protocol_select == MODBUS_PROTOCAL)
		{
			if(Modbus.com_config[0] != MODBUS_SLAVE)
			{
				Modbus.com_config[0] = MODBUS_SLAVE;
				save_uint8_to_flash( FLASH_UART_CONFIG, Modbus.com_config[0]);
				// uart_init(0);
				Count_com_config();
			}
		}
		if(gSwtich_protocol_select == BACNET_PROTOCAL)
		{
			if(Modbus.com_config[0] != BACNET_SLAVE)
			{
				Modbus.com_config[0] = BACNET_SLAVE;
				save_uint8_to_flash( FLASH_UART_CONFIG, Modbus.com_config[0]);
				// uart_init(0);
				Count_com_config();
			}
		}
		if( (gSwtich_range_1== 1)&&(gSwtich_range_2== 0))//&&(gSwtich_range_3== 1)&&(gSwtich_range_4== 1))
			gSwtich_temp_range = TEMP_50_50;
		else if( (gSwtich_range_1== 1)&&(gSwtich_range_2== 1))//&&(gSwtich_range_3== 1)&&(gSwtich_range_4== 1))
			gSwtich_temp_range = TEMP_20_80;
		else if( (gSwtich_range_1== 0)&&(gSwtich_range_2== 0))//&&(gSwtich_range_3== 1)&&(gSwtich_range_4== 1))
			gSwtich_temp_range = TEMP_0_50;
		else if( (gSwtich_range_1== 0)&&(gSwtich_range_2== 1))//&&(gSwtich_range_3== 1)&&(gSwtich_range_4== 1))
			gSwtich_temp_range = TEMP_0_100;
		else
			gSwtich_temp_range = TEMP_UNKNOW;

		if( (gSwtich_range_3== 1)&&(gSwtich_range_4== 1))//&&(gSwtich_range_3== 1)&&(gSwtich_range_4== 1))
			gSwitch_hum_range = HUMIDITY_TYPE;
		else if((gSwtich_range_3== 1)&&(gSwtich_range_4== 0))
			gSwitch_hum_range = ENTHALPY_TYPE;
		else if((gSwtich_range_3== 0)&&(gSwtich_range_4== 0))
			gSwitch_hum_range = DEWPOINT_TYPE;
		else if((gSwtich_range_3 ==0)&&(gSwtich_range_4==1))
			gSwitch_hum_range = ABS_HUM_TYPE;
		else
			gSwitch_hum_range = UNKNOWN_TYPE;

		//Test[6] = gSwtich_output_mode;
		//Test[7] = gSwtich_protocol_select;
		//Test[8] = gSwtich_temp_range;
		//Test[9] = gSwitch_hum_range;

		if(outputs[0].range == 1)
			if((outputs[0].value<=10000)&&(outputs[0].value>=0))
				holding_reg_params.fan_module_pwm1 = outputs[0].value*10/391;
		if(outputs[1].range == 1)
			if((outputs[1].value<=10000)&&(outputs[1].value>=0))
				holding_reg_params.fan_module_pwm2 = outputs[1].value*10/391;
		if(outputs[2].range == 1)
			if((outputs[2].value<=10000)&&(outputs[2].value>=0))
				holding_reg_params.fan_module_pwm3 = outputs[2].value*10/391;
		if((outputs[0].range == 4)||(outputs[0].range == 7))
			if((outputs[0].value<=100000)&&(outputs[0].value>=0))
				holding_reg_params.fan_module_pwm1 = outputs[0].value/391;
		if((outputs[1].range == 4)||(outputs[1].range == 7))
			if((outputs[1].value<=100000)&&(outputs[1].value>=0))
				holding_reg_params.fan_module_pwm2 = outputs[1].value/391;
		if((outputs[2].range == 4)||(outputs[2].range == 7))
			if((outputs[2].value<=100000)&&(outputs[2].value>=0))
				holding_reg_params.fan_module_pwm3 = outputs[2].value/391;
		//Test[0] = holding_reg_params.fan_module_pwm1;
		//Test[1] = holding_reg_params.fan_module_pwm2;
	//        Test[2] = outputs[0].switch_status;
	//        Test[3] = outputs[1].switch_status;

		switch(gSwtich_temp_range)
		{
		case TEMP_0_100:
			if(g_sensors.temperature<0)
				holding_reg_params.fan_module_pwm2 = 0;
			else if(g_sensors.temperature>1000)
				holding_reg_params.fan_module_pwm2 = 250;
			else
				holding_reg_params.fan_module_pwm2 = g_sensors.temperature*250/1000;
			break;
		case TEMP_20_80:
			if(g_sensors.temperature<-20)
				holding_reg_params.fan_module_pwm2 = 0;
			else if(g_sensors.temperature>800)
				holding_reg_params.fan_module_pwm2 = 250;
			else
				holding_reg_params.fan_module_pwm2 = (g_sensors.temperature+200)*250/1000;
			break;
		case TEMP_0_50:
			if(g_sensors.temperature<0)
				holding_reg_params.fan_module_pwm2 = 0;
			else if(g_sensors.temperature>500)
				holding_reg_params.fan_module_pwm2 = 250;
			else
				holding_reg_params.fan_module_pwm2 = g_sensors.temperature*2*250/1000;
			break;
		case TEMP_50_50:
			if(g_sensors.temperature<-50)
				holding_reg_params.fan_module_pwm2 = 0;
			else if(g_sensors.temperature>50)
				holding_reg_params.fan_module_pwm2 = 250;
			else
				holding_reg_params.fan_module_pwm2 = (g_sensors.temperature+500)*250/1000;
			break;
		default:
			if(g_sensors.temperature<0)
				holding_reg_params.fan_module_pwm2 = 0;
			else if(g_sensors.temperature>1000)
				holding_reg_params.fan_module_pwm2 = 250;
			else
				holding_reg_params.fan_module_pwm2 = g_sensors.temperature*250/1000;
			break;
		}
		switch(gSwitch_hum_range)
		{
		case HUMIDITY_TYPE:
			holding_reg_params.fan_module_pwm1 = g_sensors.humidity*250/1000;
			break;
		case ABS_HUM_TYPE:
			if(g_sensors.absHumi>500)
				holding_reg_params.fan_module_pwm1 = 250;
			else
				holding_reg_params.fan_module_pwm1 = g_sensors.absHumi*2*250/1000;
			break;
		case DEWPOINT_TYPE:
			if(g_sensors.dewpoint<-200)
				holding_reg_params.fan_module_pwm1 = 0;
			else if(g_sensors.dewpoint > 800)
				holding_reg_params.fan_module_pwm1 = 250;
			else
				holding_reg_params.fan_module_pwm1 = (g_sensors.dewpoint+200)*250/1000;
			break;
		case ENTHALPY_TYPE:
			if(g_sensors.enthalpy<0)
				holding_reg_params.fan_module_pwm1 = 0;
			else if(g_sensors.enthalpy > 850)
				holding_reg_params.fan_module_pwm1 = 250;
			else
				holding_reg_params.fan_module_pwm1 = (g_sensors.enthalpy*1000/850)*250/1000;
			break;
		default:
			holding_reg_params.fan_module_pwm1 = g_sensors.humidity*250/1000;
			break;
		}
		if(g_sensors.co2 > 2000)
			g_sensors.co2 = 2000;
		holding_reg_params.fan_module_pwm3 = g_sensors.co2*125/1000;
		if(gSwtich_output_mode == OUT_0_10_V)
		{
			ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, holding_reg_params.fan_module_pwm1);//LEDC_TEST_DUTY);//
			ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, holding_reg_params.fan_module_pwm2);
			ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, holding_reg_params.fan_module_pwm3);
		}
		if(gSwtich_output_mode == OUT_4_20_MA)
		{
			int16_t th_offset;
			uint16_t target_temp_ad, target_hum_ad, target_co2_ad;
			if(Test[3]==1)//switch_am)
			{
				target_temp_ad = (Test[4]-4)*(Test[6]-Test[7])/16+Test[7];
				target_co2_ad = (Test[5]-4)*(Test[16]-Test[17])/16+Test[17];
				if( adc_feedback2>target_temp_ad)
				{
					th_offset = (adc_feedback2-target_temp_ad)/2;
					tempDac = ((tempDac-th_offset) >0)?(tempDac-th_offset):0;
				}
				else if(adc_feedback2<target_temp_ad)
				{
					th_offset = (target_temp_ad-adc_feedback2)/2;
					tempDac = ((tempDac+th_offset) < 4096)?(tempDac+th_offset):Test[6];
				}

				if( adc_feedback3>target_co2_ad)
				{
					th_offset = (adc_feedback3-target_co2_ad)/2;
					co2Dac = ((co2Dac-th_offset) >0)?(co2Dac-th_offset):0;
				}
				else if(adc_feedback3<target_co2_ad)
				{
					th_offset = (target_co2_ad-adc_feedback3)/2;
					co2Dac = ((co2Dac+th_offset) < 4096)?(co2Dac+th_offset):Test[6];
				}

			}else{
				target_temp_ad = holding_reg_params.fan_module_pwm2*(Test[6]-Test[7])/255+Test[7];
				target_hum_ad = holding_reg_params.fan_module_pwm1*(Test[6]-Test[7])/255+Test[7];
				target_co2_ad = holding_reg_params.fan_module_pwm3*(Test[16]-Test[17])/255+Test[17];
				if( adc_feedback2>target_temp_ad)
				{
					th_offset = (adc_feedback2-target_temp_ad)/2;
					tempDac = ((tempDac-th_offset) >0)?(tempDac-th_offset):0;
				}
				else if(adc_feedback2<target_temp_ad)
				{
					th_offset = (target_temp_ad-adc_feedback2)/2;
					tempDac = ((tempDac+th_offset) < 4096)?(tempDac+th_offset):Test[6];
				}
				if( adc_feedback1>target_hum_ad)
				{
					th_offset = (adc_feedback1-target_hum_ad)/2;
					humDac = ((humDac-th_offset) >0)?(humDac-th_offset):0;
				}
				else if(adc_feedback1<target_hum_ad)
				{
					th_offset = (target_hum_ad-adc_feedback1)/2;
					humDac = ((humDac+th_offset) < 4096)?(humDac+th_offset):Test[6];
				}
				if( adc_feedback3>target_co2_ad)
				{
					th_offset = (adc_feedback3-target_co2_ad)/2;
					co2Dac = ((co2Dac-th_offset) >0)?(co2Dac-th_offset):0;
				}
				else if(adc_feedback3<target_co2_ad)
				{
					th_offset = (target_co2_ad-adc_feedback3)/2;
					co2Dac = ((co2Dac+th_offset) < 4096)?(co2Dac+th_offset):Test[16];
				}
			}
			//tempDac /= 16;
			Test[8] = target_temp_ad;
			Test[9] = co2Dac;
			Test[11]= target_co2_ad;
			ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, tempDac/16);
			ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, humDac/16);
			ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, co2Dac/16);
		}
		ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
		ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
		ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2);

		vTaskDelay(50 / portTICK_RATE_MS);
	}
}
static void fan_led_task(void* arg)
{
	uint32_t cnt=0;
	uint8_t heart_stat=0;
	uint8_t identify_cn = 0;
	uint8_t identify_count = 0;
	holding_reg_params.led_rx485_rx = 0;
	holding_reg_params.led_rx485_tx = 0;

    for(;;) {
		//gpio_set_level(LED_HEART_BEAT, (cnt++) % 8);
    	if(gIdentify == 1)
    	{
    		identify_count ++;
    		identify_cn = !identify_cn;
    		if(identify_cn )
    		{
    			gpio_set_level(LED_HEART_BEAT, 1);
				gpio_set_level(LED_FAN_SPEED, 1);
				gpio_set_level(LED_WIFI, 1);
				gpio_set_level(LED_RS485_TX, 1);
				gpio_set_level(LED_RS485_RX, 1);
				vTaskDelay(150 / portTICK_RATE_MS);
    		}
    		/*else if(identify_cn >6)
    		{
    			identify_cn = 0;
    		}*/
    		else
    		{
    			gpio_set_level(LED_HEART_BEAT, 0);
				gpio_set_level(LED_FAN_SPEED, 0);
				gpio_set_level(LED_WIFI, 0);
				gpio_set_level(LED_RS485_TX, 0);
				gpio_set_level(LED_RS485_RX, 0);
				vTaskDelay(150 / portTICK_RATE_MS);
    		}
    		if(identify_count >= 50)
    		{
    			gIdentify = 0;
    			identify_count = 0;
    		}
    	}
    	else
    	{
        	heart_stat++;
			if(heart_stat==5)
			{
				//heart_stat = 0;
				gpio_set_level(LED_HEART_BEAT, 1);
			}
			if(heart_stat == 10)
			{
				//heart_stat = 1;
				heart_stat = 0;
				gpio_set_level(LED_HEART_BEAT, 0);
			}
			if((holding_reg_params.fan_module_pulse==0))//||(holding_reg_params.fan_module_pwm2==255))
				gpio_set_level(LED_FAN_SPEED, 1);
			else if((holding_reg_params.fan_module_pulse<50))//||(holding_reg_params.fan_module_pwm2>200))
				gpio_set_level(LED_FAN_SPEED, (cnt) % 6);
			else if((holding_reg_params.fan_module_pulse<100))//||(holding_reg_params.fan_module_pwm2>150))
				gpio_set_level(LED_FAN_SPEED, (cnt) % 5);
			else if((holding_reg_params.fan_module_pulse<150))//||(holding_reg_params.fan_module_pwm2>100))
				gpio_set_level(LED_FAN_SPEED, (cnt) % 4);
			else if((holding_reg_params.fan_module_pulse<200))//||(holding_reg_params.fan_module_pwm2>50))
				gpio_set_level(LED_FAN_SPEED, (cnt) % 3);
			else if((holding_reg_params.fan_module_pulse<256))//||(holding_reg_params.fan_module_pwm2>0))
				gpio_set_level(LED_FAN_SPEED, (cnt) % 2);
			if(SSID_Info.IP_Wifi_Status == WIFI_CONNECTED)
			{
				//gpio_set_level(LED_WIFI, 0);
				if(holding_reg_params.wifi_led > 0)
				{
					gpio_set_level(LED_WIFI, 1);
					holding_reg_params.wifi_led --;
				}
				else
					gpio_set_level(LED_WIFI, 0);
			}
			else
				gpio_set_level(LED_WIFI, 1);

			if(holding_reg_params.led_rx485_tx>0){
				gpio_set_level(LED_RS485_TX, 0);
				holding_reg_params.led_rx485_tx--;
			}
			else
				gpio_set_level(LED_RS485_TX, 1);

			if(holding_reg_params.led_rx485_rx>0)
			{
				gpio_set_level(LED_RS485_RX, 0);
				holding_reg_params.led_rx485_rx--;
			}
			else
				gpio_set_level(LED_RS485_RX, 1);
    	}

		//outputs[1].value = 200;
        //holding_reg_params.fan_module_pwm1 = get_output_raw(0);
        //holding_reg_params.fan_module_pwm2 = get_output_raw(1);
        /*outputs[0].switch_status = 1;
        outputs[1].switch_status = 1;
        outputs[0].auto_manual = 1;
        outputs[1].auto_manual = 1;
        outputs[0].digital_analog = 1;
        outputs[1].digital_analog = 1;
        outputs[0].range = 1;
        outputs[1].range = 1;*/
		if(outputs[0].range == 1)
			if((outputs[0].value<=10000)&&(outputs[1].value>=0))
				holding_reg_params.fan_module_pwm1 = outputs[0].value*10/391;
		if(outputs[1].range == 1)
			if((outputs[1].value<=10000)&&(outputs[1].value>=0))
				holding_reg_params.fan_module_pwm2 = outputs[1].value*10/391;
		if((outputs[0].range == 4)||(outputs[0].range == 7))
			if((outputs[0].value<=100000)&&(outputs[1].value>=0))
				holding_reg_params.fan_module_pwm1 = outputs[0].value/391;
		if((outputs[1].range == 4)||(outputs[1].range == 7))
			if((outputs[1].value<=100000)&&(outputs[1].value>=0))
				holding_reg_params.fan_module_pwm2 = outputs[1].value/391;
        //Test[0] = holding_reg_params.fan_module_pwm1;
        //Test[1] = holding_reg_params.fan_module_pwm2;
//        Test[2] = outputs[0].switch_status;
//        Test[3] = outputs[1].switch_status;
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, holding_reg_params.fan_module_pwm1);//LEDC_TEST_DUTY);//
		ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, (255-holding_reg_params.fan_module_pwm2));
		ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
		ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);

		vTaskDelay(50 / portTICK_RATE_MS);
	}
}

/*void timer_init(void)
{
    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            // name is optional, but may help identify the timer when debugging
            .name = "periodic"
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    // The timer has been created but is not running yet

    // Start the timers
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000000));
}*/

void transducer_switch_init(void)
{
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = TRANSDUCER_SWTICH_1_SEL | TRANSDUCER_SWTICH_2_SEL | TRANSDUCER_SWTICH_3_SEL |
			TRANSDUCER_SWTICH_4_SEL | TRANSDUCER_SWTICH_5_SEL | TRANSDUCER_SWTICH_6_SEL;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);

    if (unit == ADC_UNIT_1) {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(transducer_channel_1, atten);
        adc1_config_channel_atten(transducer_channel_2, atten);
        adc1_config_channel_atten(transducer_channel_3, atten);
    }
}

void led_init(void)
{
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask = LED_HEART_BEAT_SEL | LED_FAN_SPEED_SEL | LED_WIFI_SEL |
			LED_RS485_TX_SEL | LED_RS485_RX_SEL;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);

//	outputs[0].switch_status = 0;
//	outputs[1].switch_status = 1;
//	outputs[2].switch_status = 2;

	xTaskCreate(fan_led_task, "fan_led_task", 2048, NULL, 1, NULL);
}
#if 0
static xQueueHandle gpio_evt_queue = NULL;
static uint32_t pulseValue = 0;

static void IRAM_ATTR pulse_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void pulse_task(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            //printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
        	pulseValue++;
        }
    }
}


void pulse_couter_init(void)
{
	gpio_config_t io_conf;
	//interrupt of rising edge
	io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
	//bit mask of the pins,
	io_conf.pin_bit_mask = PULSE_COUNTER_SEL;
	//set as input mode
	io_conf.mode = GPIO_MODE_INPUT;
	//enable pull-up mode
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);
	//create a queue to handle gpio event from isr
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(pulse_task, "pulse", 2048, NULL, 9, NULL);
	//change gpio intrrupt type for one pin
	//gpio_set_intr_type(PULSE_COUNTER, GPIO_INTR_ANYEDGE);
    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(PULSE_COUNTER, pulse_isr_handler, (void*) PULSE_COUNTER);
}
#endif

#define PCNT_INPUT_SIG_IO   5  // Pulse Input GPIO
#define PCNT_TEST_UNIT      PCNT_UNIT_0
#define PCNT_H_LIM_VAL      30000
#define PCNT_L_LIM_VAL     -30000

xQueueHandle pcnt_evt_queue;   // A queue to handle pulse counter events
pcnt_isr_handle_t user_isr_handle = NULL; //user's ISR service handle
/* A sample structure to pass events from the PCNT
 * interrupt handler to the main program.
 */
typedef struct {
    int unit;  // the PCNT unit that originated an interrupt
    uint32_t status; // information on the event type that caused the interrupt
} pcnt_evt_t;

/* Decode what PCNT's unit originated an interrupt
 * and pass this information together with the event type
 * the main program using a queue.
 */
static void IRAM_ATTR pcnt_example_intr_handler(void *arg)
{
    uint32_t intr_status = PCNT.int_st.val;
    int i;
    pcnt_evt_t evt;
    portBASE_TYPE HPTaskAwoken = pdFALSE;

    for (i = 0; i < PCNT_UNIT_MAX; i++) {
        if (intr_status & (BIT(i))) {
            evt.unit = i;
            /* Save the PCNT event type that caused an interrupt
               to pass it to the main program */
            evt.status = PCNT.status_unit[i].val;
            PCNT.int_clr.val = BIT(i);
            //xQueueSendFromISR(pcnt_evt_queue, &evt, &HPTaskAwoken);
            //if (HPTaskAwoken == pdTRUE) {
            //    portYIELD_FROM_ISR();
            //}
        }
    }
}

/* Initialize PCNT functions:
 *  - configure and initialize PCNT
 *  - set up the input filter
 *  - set up the counter events to watch
 */
void my_pcnt_init(void)
{
    /* Prepare configuration for the PCNT unit */
    pcnt_config_t pcnt_config = {
        // Set PCNT input signal and control GPIOs
        .pulse_gpio_num = PCNT_INPUT_SIG_IO,
        .ctrl_gpio_num = -1,
        .channel = PCNT_CHANNEL_0,
        .unit = PCNT_TEST_UNIT,
        // What to do on the positive / negative edge of pulse input?
        .pos_mode = PCNT_COUNT_INC,   // Count up on the positive edge
        .neg_mode = PCNT_COUNT_DIS,   // Keep the counter value on the negative edge
        // What to do when control input is low or high?
        .lctrl_mode = PCNT_MODE_KEEP, // Reverse counting direction if low
        .hctrl_mode = PCNT_MODE_KEEP,    // Keep the primary counter mode if high
        // Set the maximum and minimum led_limit values to watch
        .counter_h_lim = PCNT_H_LIM_VAL,
        .counter_l_lim = PCNT_L_LIM_VAL,
    };
    /* Initialize PCNT unit */
    pcnt_unit_config(&pcnt_config);

    /* Configure and enable the input filter */
    pcnt_set_filter_value(PCNT_TEST_UNIT, 100);
    pcnt_filter_enable(PCNT_TEST_UNIT);

    /* Initialize PCNT's counter */
    pcnt_counter_pause(PCNT_TEST_UNIT);
    pcnt_counter_clear(PCNT_TEST_UNIT);

    /* Register ISR handler and enable interrupts for PCNT unit */
	pcnt_isr_register(pcnt_example_intr_handler, NULL, 0, &user_isr_handle);
	pcnt_intr_enable(PCNT_TEST_UNIT);

	/* Everything is set up, now go to counting */
	pcnt_counter_resume(PCNT_TEST_UNIT);
	xTaskCreate(pcnt_task, "pcnt_task", 2048, NULL, 1, NULL);
}

static void pcnt_task(void* arg)
{
	/* Initialize PCNT event queue and PCNT functions */
	//pcnt_evt_queue = xQueueCreate(10, sizeof(pcnt_evt_t));
	//pcnt_init();

    int16_t count = 0;
    //pcnt_evt_t evt;
    //portBASE_TYPE res;
    while (1) {
    	/* Wait for the event information passed from PCNT's interrupt handler.
		 * Once received, decode the event type and print it on the serial monitor.
		 */
		//res = xQueueReceive(pcnt_evt_queue, &evt, 1000 / portTICK_PERIOD_MS);
		//if (res == pdTRUE) {
			pcnt_get_counter_value(PCNT_TEST_UNIT, &count);
			holding_reg_params.fan_module_pulse = (uint16_t)count;
			pcnt_counter_clear(PCNT_TEST_UNIT);
			//inputs[4].range = 26;
			//if(inputs[4].range == 26)

			if(inputs[4].range == 29)
				inputs[4].value = holding_reg_params.fan_module_pulse*100*60;
			else
				inputs[4].value = holding_reg_params.fan_module_pulse*100;
		//}
			//count = 0;
			vTaskDelay(10000 / portTICK_RATE_MS);
    }
	if(user_isr_handle) {
		//Free the ISR service handle.
		esp_intr_free(user_isr_handle);
		user_isr_handle = NULL;
	}
}

void led_pwm_init(void)
{
	int ch;
	/*
	 * Prepare and set configuration of timers
	 * that will be used by LED Controller
	 */
	ledc_timer_config_t ledc_timer = {
		.duty_resolution = LEDC_TIMER_8_BIT, // resolution of PWM duty
		.freq_hz = 10000,                      // frequency of PWM signal
		.speed_mode = LEDC_HS_MODE,           // timer mode
		.timer_num = LEDC_HS_TIMER,            // timer index
		.clk_cfg = LEDC_USE_APB_CLK,//LEDC_AUTO_CLK,              // Auto select the source clock
	};
	// Set configuration of timer0 for high speed channels
	ledc_timer_config(&ledc_timer);
	if(Modbus.mini_type == PROJECT_FAN_MODULE)
	{
		ledc_channel_config_t ledc_channel[LEDC_TEST_CH_NUM] = {
			{
				.channel    = LEDC_HS_CH0_CHANNEL,
				.duty       = 0,
				.gpio_num   = LEDC_HS_CH0_GPIO,
				.speed_mode = LEDC_HS_MODE,
				.hpoint     = 0,
				.timer_sel  = LEDC_HS_TIMER
			},
			{
				.channel    = LEDC_HS_CH1_CHANNEL,
				.duty       = 0,
				.gpio_num   = LEDC_HS_CH1_GPIO,
				.speed_mode = LEDC_HS_MODE,
				.hpoint     = 0,
				.timer_sel  = LEDC_HS_TIMER
			},
			/*{
				.channel    = LEDC_HS_CH2_CHANNEL,
				.duty       = 0,
				.gpio_num   = LED_HEART_BEAT,
				.speed_mode = LEDC_HS_MODE,
				.hpoint     = 0,
				.timer_sel  = LEDC_HS_TIMER
			},
			{
				.channel    = LEDC_HS_CH3_CHANNEL,
				.duty       = 0,
				.gpio_num   = LED_FAN_SPEED,
				.speed_mode = LEDC_HS_MODE,
				.hpoint     = 0,
				.timer_sel  = LEDC_HS_TIMER
			},
			{
				.channel    = LEDC_HS_CH4_CHANNEL,
				.duty       = 0,
				.gpio_num   = LED_WIFI,
				.speed_mode = LEDC_HS_MODE,
				.hpoint     = 0,
				.timer_sel  = LEDC_HS_TIMER
			},*/
		};
		for (ch = 0; ch < LEDC_TEST_CH_NUM; ch++) {
			ledc_channel_config(&ledc_channel[ch]);
		}

		ledc_set_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel, holding_reg_params.fan_module_pwm1);//LEDC_TEST_DUTY);//
		ledc_set_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel, (255-holding_reg_params.fan_module_pwm2));
		ledc_update_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel);
		ledc_update_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel);
	}
	if(Modbus.mini_type == PROJECT_TRANSDUCER)
	{
		ledc_channel_config_t ledc_channel[LEDC_TRANSDUCER_CH_NUM] = {
			{
				.channel    = LEDC_HS_CH0_CHANNEL,
				.duty       = 0,
				.gpio_num   = LEDC_TRANSDUCER_CH0_GPIO,
				.speed_mode = LEDC_HS_MODE,
				.hpoint     = 0,
				.timer_sel  = LEDC_HS_TIMER
			},
			{
				.channel    = LEDC_HS_CH1_CHANNEL,
				.duty       = 0,
				.gpio_num   = LEDC_TRANSDUCER_CH1_GPIO,
				.speed_mode = LEDC_HS_MODE,
				.hpoint     = 0,
				.timer_sel  = LEDC_HS_TIMER
			},
			{
				.channel    = LEDC_HS_CH2_CHANNEL,
				.duty       = 0,
				.gpio_num   = LEDC_TRANSDUCER_CH2_GPIO,
				.speed_mode = LEDC_HS_MODE,
				.hpoint     = 0,
				.timer_sel  = LEDC_HS_TIMER
			},
		};
		for (ch = 0; ch < LEDC_TRANSDUCER_CH_NUM; ch++) {
			ledc_channel_config(&ledc_channel[ch]);
		}

		ledc_set_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel, holding_reg_params.fan_module_pwm1);//LEDC_TEST_DUTY);//
		ledc_set_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel, (255-holding_reg_params.fan_module_pwm2));
		ledc_set_duty(ledc_channel[2].speed_mode, ledc_channel[2].channel, holding_reg_params.fan_module_pwm3);
		ledc_update_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel);
		ledc_update_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel);
		ledc_update_duty(ledc_channel[2].speed_mode, ledc_channel[2].channel);

		xTaskCreate(transducer_output_task,"transducer_fan_task",2048,NULL,1,NULL);
	}
}
