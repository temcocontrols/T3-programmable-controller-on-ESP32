#include <string.h>
#include "esp_attr.h"
#include "ud_str.h"
#include "user_data.h"
#include "rtc_value_backup.h"

#if RTC_VALUE_BACKUP_TO_FLASH
#include "nvs.h"
#include "flash.h"
#endif

#define RTC_VALUE_MAGIC		0x52424B31
#define RTC_VALUE_VERSION	1
#define RTC_VALUE_NVS_KEY	"IO_VALUE"

#pragma pack(1)
typedef struct
{
	U32_T magic;
	U16_T version;
	U8_T n_in;
	U8_T n_out;
	U8_T n_var;
	U8_T reserved;
	U32_T crc;
	S32_T in_value[MAX_INS];
	S32_T out_value[MAX_OUTS];
	S32_T var_value[MAX_VARS];
} STR_RTC_VALUE_BACKUP;
#pragma pack()

RTC_NOINIT_ATTR static STR_RTC_VALUE_BACKUP rtc_values;

#if RTC_VALUE_BACKUP_TO_FLASH
static U32_T nvs_last_crc;
static U8_T nvs_last_crc_ok;
static U16_T nvs_sec_cnt;	/* was U8_T: overflow when RTC_VALUE_NVS_SEC > 255 */
#endif

static U32_T rtc_value_calc_crc(STR_RTC_VALUE_BACKUP *bk)
{
	U32_T crc;
	U16_T i;

	crc = 0xA5A5A5A5;
	crc ^= (U32_T)bk->version;
	crc ^= ((U32_T)bk->n_in << 8) | (U32_T)bk->n_out | ((U32_T)bk->n_var << 16);

	for(i = 0; i < bk->n_in && i < MAX_INS; i++)
		crc = (crc * 16777619) ^ (U32_T)bk->in_value[i];
	for(i = 0; i < bk->n_out && i < MAX_OUTS; i++)
		crc = (crc * 16777619) ^ (U32_T)bk->out_value[i];
	for(i = 0; i < bk->n_var && i < MAX_VARS; i++)
		crc = (crc * 16777619) ^ (U32_T)bk->var_value[i];

	return crc;
}

static int rtc_value_backup_valid(STR_RTC_VALUE_BACKUP *bk)
{
	if(bk->magic != RTC_VALUE_MAGIC || bk->version != RTC_VALUE_VERSION)
		return 0;
	if(bk->n_in > MAX_INS || bk->n_out > MAX_OUTS || bk->n_var > MAX_VARS)
		return 0;
	if(bk->crc != rtc_value_calc_crc(bk))
		return 0;
	return 1;
}

static int rtc_value_backup_apply(STR_RTC_VALUE_BACKUP *bk)
{
	U16_T i;
	U8_T n_in;
	U8_T n_out;
	U8_T n_var;

#if NEW_IO
	if(new_inputs == NULL || new_outputs == NULL || new_vars == NULL)
		return 0;

	n_in = (bk->n_in < max_inputs) ? bk->n_in : max_inputs;
	n_out = (bk->n_out < max_outputs) ? bk->n_out : max_outputs;
	n_var = (bk->n_var < max_vars) ? bk->n_var : max_vars;
#else
	n_in = bk->n_in;
	n_out = bk->n_out;
	n_var = bk->n_var;
#endif

	for(i = 0; i < n_in; i++)
	{
#if NEW_IO
		new_inputs[i].value = bk->in_value[i];
#else
		inputs[i].value = bk->in_value[i];
#endif
	}
	for(i = 0; i < n_out; i++)
	{
#if NEW_IO
		new_outputs[i].value = bk->out_value[i];
#else
		outputs[i].value = bk->out_value[i];
#endif
	}
	for(i = 0; i < n_var; i++)
	{
#if NEW_IO
		new_vars[i].value = bk->var_value[i];
#else
		vars[i].value = bk->var_value[i];
#endif
	}

	return 1;
}

#if RTC_VALUE_BACKUP_TO_FLASH
static int rtc_value_backup_to_nvs(void)
{
	nvs_handle_t handle;
	esp_err_t err;

	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
	if(err != ESP_OK)
		return 0;

	err = nvs_set_blob(handle, RTC_VALUE_NVS_KEY, &rtc_values, sizeof(STR_RTC_VALUE_BACKUP));
	if(err == ESP_OK)
		err = nvs_commit(handle);
	nvs_close(handle);

	if(err != ESP_OK)
		return 0;

	nvs_last_crc = rtc_values.crc;
	nvs_last_crc_ok = 1;
	nvs_sec_cnt = 0;
	return 1;
}

static int rtc_value_backup_from_nvs(STR_RTC_VALUE_BACKUP *bk)
{
	nvs_handle_t handle;
	esp_err_t err;
	size_t len;

	len = sizeof(STR_RTC_VALUE_BACKUP);
	err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
	if(err != ESP_OK)
		return 0;

	err = nvs_get_blob(handle, RTC_VALUE_NVS_KEY, bk, &len);
	nvs_close(handle);
	if(err != ESP_OK || len != sizeof(STR_RTC_VALUE_BACKUP))
		return 0;
	if(rtc_value_backup_valid(bk) == 0)
		return 0;
	return 1;
}
#endif

static void rtc_value_backup_fill(void)
{
	U16_T i;
	U8_T n_in;
	U8_T n_out;
	U8_T n_var;

#if NEW_IO
	if(new_inputs == NULL || new_outputs == NULL || new_vars == NULL)
		return;

	n_in = max_inputs;
	n_out = max_outputs;
	n_var = max_vars;
#else
	n_in = MAX_INS;
	n_out = MAX_OUTS;
	n_var = MAX_VARS;
#endif

	if(n_in > MAX_INS)
		n_in = MAX_INS;
	if(n_out > MAX_OUTS)
		n_out = MAX_OUTS;
	if(n_var > MAX_VARS)
		n_var = MAX_VARS;

	memset(&rtc_values, 0, sizeof(STR_RTC_VALUE_BACKUP));
	rtc_values.magic = RTC_VALUE_MAGIC;
	rtc_values.version = RTC_VALUE_VERSION;
	rtc_values.n_in = n_in;
	rtc_values.n_out = n_out;
	rtc_values.n_var = n_var;

	for(i = 0; i < n_in; i++)
	{
#if NEW_IO
		rtc_values.in_value[i] = new_inputs[i].value;
#else
		rtc_values.in_value[i] = inputs[i].value;
#endif
	}
	for(i = 0; i < n_out; i++)
	{
#if NEW_IO
		rtc_values.out_value[i] = new_outputs[i].value;
#else
		rtc_values.out_value[i] = outputs[i].value;
#endif
	}
	for(i = 0; i < n_var; i++)
	{
#if NEW_IO
		rtc_values.var_value[i] = new_vars[i].value;
#else
		rtc_values.var_value[i] = vars[i].value;
#endif
	}

	rtc_values.crc = rtc_value_calc_crc(&rtc_values);
}

void rtc_value_backup_save(void)
{
	rtc_value_backup_fill();

#if RTC_VALUE_BACKUP_TO_FLASH
	if(nvs_last_crc_ok && nvs_last_crc == rtc_values.crc)
	{
		nvs_sec_cnt = 0;
		return;
	}

	if(nvs_sec_cnt < RTC_VALUE_NVS_SEC)
	{
		nvs_sec_cnt++;
		return;
	}

	rtc_value_backup_to_nvs();
#endif
}

void rtc_value_backup_flush(void)
{
	rtc_value_backup_fill();
#if RTC_VALUE_BACKUP_TO_FLASH
	if(nvs_last_crc_ok && nvs_last_crc == rtc_values.crc)
		return;
	rtc_value_backup_to_nvs();
#endif
}

int rtc_value_backup_restore(void)
{
#if RTC_VALUE_BACKUP_TO_FLASH
	STR_RTC_VALUE_BACKUP nvs_bk;
#endif

	if(rtc_value_backup_valid(&rtc_values))
	{
		if(rtc_value_backup_apply(&rtc_values))
		{
#if RTC_VALUE_BACKUP_TO_FLASH
			/* Soft reset used RTC. Sync to NVS so power-off can restore too. */
			nvs_last_crc = rtc_values.crc;
			nvs_last_crc_ok = 1;
			if(rtc_value_backup_from_nvs(&nvs_bk) == 0 || nvs_bk.crc != rtc_values.crc)
				rtc_value_backup_to_nvs();
#endif
			return 1;
		}
	}

#if RTC_VALUE_BACKUP_TO_FLASH
	if(rtc_value_backup_from_nvs(&nvs_bk) == 0)
		return 0;

	memcpy(&rtc_values, &nvs_bk, sizeof(STR_RTC_VALUE_BACKUP));
	if(rtc_value_backup_apply(&rtc_values) == 0)
		return 0;

	nvs_last_crc = rtc_values.crc;
	nvs_last_crc_ok = 1;
	return 1;
#else
	return 0;
#endif
}
