#ifndef MINI_BMS_H
#define MINI_BMS_H

#include <stdint.h>

/* GPIO: IO15 = WS2812 x6, IO4=SDA, IO14=SCL, IO32=BMS RST */

void mini_bms_start_tasks(void);
/* All 6 LEDs yellow, then a short delay — call before bootloader reset */
void mini_bms_indicate_bootloader(void);
/* Cancel yellow bootloader indication (e.g. reboot blocked on battery power) */
void mini_bms_clear_bootloader_indication(void);

/* Shared snapshot for LED task / Modbus (updated by BQ task) */
extern uint16_t mini_bms_voltage_x10;
extern uint8_t  mini_bms_bars;
extern uint8_t  mini_bms_charging;
extern uint8_t  mini_bms_comm_ok;
extern uint8_t  mini_bms_ext_power;
extern int16_t  mini_bms_current_ma;
extern uint16_t mini_bms_cell_mv[7];

/* Set by Modbus write of CUVT/COVT/…; BQ task applies to chip */
extern volatile uint8_t mini_bms_prot_apply;

#endif
