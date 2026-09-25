#ifndef __STORE_H
#define __STORE_H

#include "modbus.h"
#include <stdint.h>

//#define INPUT_PAGE_LENTH		MAX_AIS * sizeof(Str_in_point)

typedef struct
{
	uint16_t trigger;
	uint16_t timer;
	uint16_t alarmOn;
	uint16_t count_down;
} trigger_t;

extern trigger_t occ_trigger;

//extern Str_in_point   inputs[];

extern void mass_flash_init(void);
extern void input_task(void *arg);
void tstat11_occ_init(void);
void tstat11_occ_update(void);

#endif
