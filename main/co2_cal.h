#ifndef _CO2_CAL_H_
#define _CO2_CAL_H_

//#include "deviceparams.h"

#define CO2_BKCAL_ON 		1
#define CO2_BKCAL_OFF		0

extern uint16_t   co2_bkcal_day;     //how many days the calibration will run, default is 14 days
extern uint16_t   co2_level; 	       //lowest co2 level in current area, default is 400 ppm
extern uint8_t    co2_1h_timer;      //one hour counter, the lowest co2 value need to keep at leat one hour
extern uint8_t    min_co2_adj;       //the minimal adjustable co2 value,default is 1ppm
extern uint8_t    co2_bkcal_onoff;   //co2 back ground calibration on/off flag, 0: off, 1:on
extern int16_t    co2_bkcal_value;   //co2 back ground calibration value, this need to be stored in different place with factory calibration
extern uint16_t   co2_lowest_value;  //the lowest co2 value during back ground calibration
extern uint16_t   co2_temp;          //temporary co2 value
extern uint8_t    value_keep_time;   //how long the lowest value need to keep


void co2_cal_initial(void);
void co2_background_calibration(uint16_t current_co2);

#endif
