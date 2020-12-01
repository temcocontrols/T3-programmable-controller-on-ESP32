#ifndef __STORE_H
#define __STORE_H

#include "modbus.h"

#define INPUT_PAGE_LENTH		MAX_AIS * sizeof(Str_in_point)

typedef enum
{
	IN_NORMAL = 0,IN_OPEN = 1,IN_SHORT
}Str_Input_status;

extern Str_in_point   inputs[];

extern void mass_flash_init(void);
extern void input_task(void *arg);

#endif
