#ifndef MTTP_TASK_H
#define MTTP_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct {
	uint32_t input_voltage;
	uint32_t input_current;
	uint32_t input_power;
	uint32_t input_energy;
	uint32_t output_voltage;
	uint32_t output_current;
	uint32_t output_power;
	uint32_t output_energy;
	uint16_t output_pwm;
	uint8_t ERR;
	uint8_t chargingPause;
	uint8_t MPPT_Mode;
	uint16_t PWM;
	uint16_t PPWM;
	uint8_t REC;
	int16_t currentCharging;
	uint16_t voltageBatteryMax;
	int16_t voltageInputPrev;
	uint16_t voltageDropout;
	uint16_t powerInputPrev;
}mppt_t;

#define MAX_AUTOBAUD_BUF_LEN		256
typedef struct
{
	uint8_t buf[MAX_AUTOBAUD_BUF_LEN];
	uint16_t length;
	uint16_t index;
}AutoBuf;

// Function declarations
extern void mppt_task_init(void);
extern void mppt_task(void *arg);
extern void ina228_read_task(void* arg);

#endif // MTTP_TASK_H
