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
#define RTC_VALUE_VERSION	2	/* packed: header + in[n] + out[n] + var[n] */
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
} STR_RTC_VALUE_HDR;
#pragma pack()

#define RTC_VALUE_PAYLOAD_MAX	((MAX_INS + MAX_OUTS + MAX_VARS) * sizeof(S32_T))
#define RTC_VALUE_BLOB_MAX	(sizeof(STR_RTC_VALUE_HDR) + RTC_VALUE_PAYLOAD_MAX)

/* Fixed RTC capacity (worst case), content packed by actual max_inputs/outputs/vars */
RTC_NOINIT_ATTR static U8_T rtc_blob[RTC_VALUE_BLOB_MAX];

#if RTC_VALUE_BACKUP_TO_FLASH
static U32_T nvs_last_crc;
static U8_T nvs_last_crc_ok;
static U16_T nvs_sec_cnt;
#endif

static STR_RTC_VALUE_HDR *rtc_hdr(void)
{
	return (STR_RTC_VALUE_HDR *)rtc_blob;
}

static S32_T *rtc_in_ptr(const STR_RTC_VALUE_HDR *h)
{
	(void)h;
	return (S32_T *)(rtc_blob + sizeof(STR_RTC_VALUE_HDR));
}

static S32_T *rtc_out_ptr(const STR_RTC_VALUE_HDR *h)
{
	return rtc_in_ptr(h) + h->n_in;
}

static S32_T *rtc_var_ptr(const STR_RTC_VALUE_HDR *h)
{
	return rtc_out_ptr(h) + h->n_out;
}

static size_t rtc_packed_size(U8_T n_in, U8_T n_out, U8_T n_var)
{
	return sizeof(STR_RTC_VALUE_HDR) +
		((size_t)n_in + (size_t)n_out + (size_t)n_var) * sizeof(S32_T);
}

static U32_T rtc_value_calc_crc(const STR_RTC_VALUE_HDR *h)
{
	U32_T crc;
	U16_T i;
	const S32_T *in_v;
	const S32_T *out_v;
	const S32_T *var_v;

	crc = 0xA5A5A5A5;
	crc ^= (U32_T)h->version;
	crc ^= ((U32_T)h->n_in << 8) | (U32_T)h->n_out | ((U32_T)h->n_var << 16);

	in_v = rtc_in_ptr(h);
	out_v = rtc_out_ptr(h);
	var_v = rtc_var_ptr(h);

	for(i = 0; i < h->n_in; i++)
		crc = (crc * 16777619) ^ (U32_T)in_v[i];
	for(i = 0; i < h->n_out; i++)
		crc = (crc * 16777619) ^ (U32_T)out_v[i];
	for(i = 0; i < h->n_var; i++)
		crc = (crc * 16777619) ^ (U32_T)var_v[i];

	return crc;
}

static int rtc_counts_ok(U8_T n_in, U8_T n_out, U8_T n_var)
{
	if(n_in > MAX_INS || n_out > MAX_OUTS || n_var > MAX_VARS)
		return 0;
	if(rtc_packed_size(n_in, n_out, n_var) > RTC_VALUE_BLOB_MAX)
		return 0;
	return 1;
}

static int rtc_value_backup_valid(void)
{
	STR_RTC_VALUE_HDR *h = rtc_hdr();

	if(h->magic != RTC_VALUE_MAGIC || h->version != RTC_VALUE_VERSION)
		return 0;
	if(!rtc_counts_ok(h->n_in, h->n_out, h->n_var))
		return 0;
	if(h->crc != rtc_value_calc_crc(h))
		return 0;
	return 1;
}

static int rtc_value_backup_apply(void)
{
	U16_T i;
	U8_T n_in;
	U8_T n_out;
	U8_T n_var;
	STR_RTC_VALUE_HDR *h = rtc_hdr();
	S32_T *in_v;
	S32_T *out_v;
	S32_T *var_v;

#if NEW_IO
	if(new_inputs == NULL || new_outputs == NULL || new_vars == NULL)
		return 0;

	n_in = (h->n_in < max_inputs) ? h->n_in : max_inputs;
	n_out = (h->n_out < max_outputs) ? h->n_out : max_outputs;
	n_var = (h->n_var < max_vars) ? h->n_var : max_vars;
#else
	n_in = h->n_in;
	n_out = h->n_out;
	n_var = h->n_var;
#endif

	in_v = rtc_in_ptr(h);
	out_v = rtc_out_ptr(h);
	var_v = rtc_var_ptr(h);

	for(i = 0; i < n_in; i++)
	{
#if NEW_IO
		new_inputs[i].value = in_v[i];
#else
		inputs[i].value = in_v[i];
#endif
	}
	for(i = 0; i < n_out; i++)
	{
#if NEW_IO
		new_outputs[i].value = out_v[i];
#else
		outputs[i].value = out_v[i];
#endif
	}
	for(i = 0; i < n_var; i++)
	{
#if NEW_IO
		new_vars[i].value = var_v[i];
#else
		vars[i].value = var_v[i];
#endif
	}

	return 1;
}

#if RTC_VALUE_BACKUP_TO_FLASH
static size_t rtc_current_packed_size(void)
{
	STR_RTC_VALUE_HDR *h = rtc_hdr();
	return rtc_packed_size(h->n_in, h->n_out, h->n_var);
}

static int rtc_value_backup_to_nvs(void)
{
	nvs_handle_t handle;
	esp_err_t err;
	size_t len = rtc_current_packed_size();

	err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
	if(err != ESP_OK)
		return 0;

	err = nvs_set_blob(handle, RTC_VALUE_NVS_KEY, rtc_blob, len);
	if(err == ESP_OK)
		err = nvs_commit(handle);
	nvs_close(handle);

	if(err != ESP_OK)
		return 0;

	nvs_last_crc = rtc_hdr()->crc;
	nvs_last_crc_ok = 1;
	nvs_sec_cnt = 0;
	return 1;
}

static int rtc_value_backup_from_nvs(void)
{
	nvs_handle_t handle;
	esp_err_t err;
	size_t len = RTC_VALUE_BLOB_MAX;
	U8_T tmp[RTC_VALUE_BLOB_MAX];
	STR_RTC_VALUE_HDR *h;

	err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
	if(err != ESP_OK)
		return 0;

	err = nvs_get_blob(handle, RTC_VALUE_NVS_KEY, tmp, &len);
	nvs_close(handle);
	if(err != ESP_OK || len < sizeof(STR_RTC_VALUE_HDR))
		return 0;

	h = (STR_RTC_VALUE_HDR *)tmp;
	if(h->magic != RTC_VALUE_MAGIC || h->version != RTC_VALUE_VERSION)
		return 0;
	if(!rtc_counts_ok(h->n_in, h->n_out, h->n_var))
		return 0;
	if(len < rtc_packed_size(h->n_in, h->n_out, h->n_var))
		return 0;

	memcpy(rtc_blob, tmp, len);
	if(rtc_hdr()->crc != rtc_value_calc_crc(rtc_hdr()))
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
	STR_RTC_VALUE_HDR *h;
	S32_T *in_v;
	S32_T *out_v;
	S32_T *var_v;

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

	/* Only touch the packed region actually used */
	memset(rtc_blob, 0, rtc_packed_size(n_in, n_out, n_var));
	h = rtc_hdr();
	h->magic = RTC_VALUE_MAGIC;
	h->version = RTC_VALUE_VERSION;
	h->n_in = n_in;
	h->n_out = n_out;
	h->n_var = n_var;
	h->reserved = 0;

	in_v = rtc_in_ptr(h);
	out_v = rtc_out_ptr(h);
	var_v = rtc_var_ptr(h);

	for(i = 0; i < n_in; i++)
	{
#if NEW_IO
		in_v[i] = new_inputs[i].value;
#else
		in_v[i] = inputs[i].value;
#endif
	}
	for(i = 0; i < n_out; i++)
	{
#if NEW_IO
		out_v[i] = new_outputs[i].value;
#else
		out_v[i] = outputs[i].value;
#endif
	}
	for(i = 0; i < n_var; i++)
	{
#if NEW_IO
		var_v[i] = new_vars[i].value;
#else
		var_v[i] = vars[i].value;
#endif
	}

	h->crc = rtc_value_calc_crc(h);
}

void rtc_value_backup_save(void)
{
	rtc_value_backup_fill();

#if RTC_VALUE_BACKUP_TO_FLASH
	if(nvs_last_crc_ok && nvs_last_crc == rtc_hdr()->crc)
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
	/* Never force Flash write when content unchanged */
	if(nvs_last_crc_ok && nvs_last_crc == rtc_hdr()->crc)
		return;
	rtc_value_backup_to_nvs();
#endif
}

int rtc_value_backup_restore(void)
{
	if(rtc_value_backup_valid())
	{
		if(rtc_value_backup_apply())
		{
#if RTC_VALUE_BACKUP_TO_FLASH
			/* Soft-reset: RTC hit. Write NVS only if missing or CRC differs. */
			nvs_handle_t handle;
			esp_err_t err;
			size_t len = RTC_VALUE_BLOB_MAX;
			U8_T tmp[RTC_VALUE_BLOB_MAX];
			U8_T need_write = 1;

			nvs_last_crc = rtc_hdr()->crc;
			nvs_last_crc_ok = 1;

			err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
			if(err == ESP_OK)
			{
				err = nvs_get_blob(handle, RTC_VALUE_NVS_KEY, tmp, &len);
				nvs_close(handle);
				if(err == ESP_OK && len >= sizeof(STR_RTC_VALUE_HDR))
				{
					STR_RTC_VALUE_HDR *th = (STR_RTC_VALUE_HDR *)tmp;
					if(th->magic == RTC_VALUE_MAGIC && th->version == RTC_VALUE_VERSION &&
					   rtc_counts_ok(th->n_in, th->n_out, th->n_var) &&
					   len >= rtc_packed_size(th->n_in, th->n_out, th->n_var) &&
					   th->crc == rtc_hdr()->crc)
					{
						need_write = 0;
					}
				}
			}
			if(need_write)
				rtc_value_backup_to_nvs();
#endif
			return 1;
		}
	}

#if RTC_VALUE_BACKUP_TO_FLASH
	if(rtc_value_backup_from_nvs() == 0)
		return 0;

	if(rtc_value_backup_apply() == 0)
		return 0;

	nvs_last_crc = rtc_hdr()->crc;
	nvs_last_crc_ok = 1;
	return 1;
#else
	return 0;
#endif
}
