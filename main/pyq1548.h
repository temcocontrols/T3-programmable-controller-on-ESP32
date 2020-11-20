#ifndef __PYQ1548_H
#define __PYQ1548_H

#include "stdbool.h"

typedef struct
{
	bool pulse_detection_mode;
	bool HPD_cut_off;
	uint8_t signal_source;
	uint8_t operation_mode;
	uint8_t window_time;
	uint8_t pulse_counter;
	uint8_t blind_time;
	uint8_t threshold;
}pyq1548_config_t;

extern pyq1548_config_t pyq1548Config;
extern void pyq1548_task(void *arg);
#endif
