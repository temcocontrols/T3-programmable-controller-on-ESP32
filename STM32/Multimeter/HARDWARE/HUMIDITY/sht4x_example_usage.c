/*
 * Copyright (c) 2020, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "sht4x.h"
#include <stdio.h>  // printf
#include "myiic.h"

// added by chelsea
#include "humidity.h"
#include "inputs.h"
extern uint16_t  test[];
uint8_t hum_exists;
extern uint8_t flag_led_err;

//int16 I2C_Sensor_tem_org[3];
//int16 I2C_Sensor_hum_org[3];

void IIC_Init(void);
/**
 * TO USE CONSOLE OUTPUT (PRINTF) AND WAIT (SLEEP) PLEASE ADAPT THEM TO YOUR
 * PLATFORM
 */


void SHT4x_Initial(void)
{
	uint8_t count = 0;
	uint8_t i = 0;
	IIC_Init();
	
	//for(i = 0;i < 3;i++)
	{
		//i2c_index = i;
		sensirion_i2c_init();
		
		while ((sht4x_probe() != STATUS_OK) && (count++ < 3)) 
		{
			//printf("SHT sensor probing failed\n");
			sensirion_sleep_usec(50000); /* sleep 1s */		
		}	

		if(count <= 3)	
		{
			hum_exists = 3;	
		}
		else
		{
			
		}
	}
	
}

extern vu16 AD_Value[32];
void Refresh_SHT4x(void)
{
		int32_t temperature, humidity;
		/* Measure temperature and relative humidity and store into variables
		 * temperature, humidity (each output multiplied by 1000).
		 */
		int8_t ret;
	
		static uint8_t error_cnt = 0;
		//uint8_t i = 0;
		//for(i = 0;i < 3;i++)
		{
			//i2c_index = i;
			ret = sht4x_measure_blocking_read(&temperature, &humidity);
			if (ret == STATUS_OK) {	
				hum_exists = 3;			
				//I2C_Sensor_tem_org[i2c_index] = temperature / 100;
				//I2C_Sensor_hum_org[i2c_index] = humidity / 100;
				test[15] = temperature / 100;
				test[14] = humidity / 100;
				test[10]++;
				AD_Value[COMMON_CHANNEL] = temperature;
				AD_Value[COMMON_CHANNEL + 2] = humidity;
				
			} else {test[11]++;
				 // printf("error reading measurement\n");
				error_cnt++;
			}		
			
			if(error_cnt > 5)
			{
				error_cnt = 0;				
				hum_exists = 0;				
			}			
			
		}
		
//		if((error_cnt[0] > 5) && (error_cnt[1] > 5) && (error_cnt[2] > 5))
//		{
//			flag_led_err = 1;
//		}
//		else
//			flag_led_err = 0;
}

