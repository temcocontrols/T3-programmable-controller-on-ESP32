#ifndef RTC_VALUE_BACKUP_H
#define RTC_VALUE_BACKUP_H

/* 1 = also save .value to NVS(Flash) for power-off restore; 0 = RTC SRAM only */
#ifndef RTC_VALUE_BACKUP_TO_FLASH
#define RTC_VALUE_BACKUP_TO_FLASH	0// 默认不开启掉电自动存储
#endif

/* Seconds after value change before writing NVS (must fit in U16). */
#ifndef RTC_VALUE_NVS_SEC
#define RTC_VALUE_NVS_SEC	600
#endif

/* RTC blob packs only max_inputs/max_outputs/max_vars entries (version 2).
 * Flash/NVS writes must compare CRC/content before programming. */

void rtc_value_backup_save(void);
void rtc_value_backup_flush(void);
int rtc_value_backup_restore(void);

#endif
