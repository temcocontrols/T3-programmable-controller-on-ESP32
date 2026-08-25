#include "bq76907.h"
#include "bq76907_i2c.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

/* STM32 IWDG kick — no-op on ESP-IDF */
#ifndef IWDG_ReloadCounter
#define IWDG_ReloadCounter()  ((void)0)
#endif

static inline void delay_ms(unsigned int ms)
{
	if (ms == 0) {
		return;
	}
	vTaskDelay(pdMS_TO_TICKS(ms));
}

static inline void delay_us(unsigned int us)
{
	if (us == 0) {
		return;
	}
	esp_rom_delay_us(us);
}

/*
 * Soft FLASH stubs: keep call sites compiling.
 * Do NOT touch STM32 flash addresses (would fault on ESP).
 */
typedef int FLASH_Status;
#ifndef FLASH_COMPLETE
#define FLASH_COMPLETE 0
#endif
static inline void FLASH_Unlock(void) {}
static inline void FLASH_Lock(void) {}
static inline FLASH_Status FLASH_ErasePage(uint32_t addr)
{
	(void)addr;
	return FLASH_COMPLETE;
}
static inline FLASH_Status FLASH_ProgramHalfWord(uint32_t addr, uint16_t data)
{
	(void)addr;
	(void)data;
	return FLASH_COMPLETE;
}

/* GPIO32 = BMS hardware reset (active-high pulse; hold low when running) */
#define BMS_HW_RESET_GPIO   GPIO_NUM_32

/* NVS/Modbus-backed thresholds (defined in modbus.c as uint16) */
extern unsigned short rmc_cuv;
extern unsigned short rmc_cov;
extern unsigned short rmc_stack;
extern unsigned short rmc_oc_chg;
extern unsigned short rmc_oc_dsg;

/* Local PWR_TEST_* image — do not share Test[50] */
static uint16_t bq_pwr_test[200];
#define test bq_pwr_test

uint16_t cuv_threshold_mv;
uint16_t cov_threshold_mv;


/*
 * FET-off / heartbeat log — last 2KB Flash page @ 512KB parts.
 * Append-only halfword program at trip time (µs). Page erase only while power stable.
 * Record: [magic][a][b][chk]  magic 0xA510 = FET off (a=reason,b=detail)
 *                          magic 0xA511 = heartbeat (a=V×10, b=min cell mV)
 */
#define FET_OFF_FLASH_ADDR    (0x08000000UL + (512UL * 1024UL) - 2048UL)
#define FET_OFF_FLASH_PAGE    2048UL
#define FET_OFF_REC_BYTES     8U
#define FET_OFF_MAX_REC       (FET_OFF_FLASH_PAGE / FET_OFF_REC_BYTES)
#define FET_OFF_FLASH_MAGIC   0xA510U
#define FET_OFF_HB_MAGIC      0xA511U

/*
 * Protection thresholds — 2KB page just above FET-off log.
 * v1 magic 0xA520: [magic][cuv][cov][sd][chk]
 * v2 magic 0xA521: [magic][cuv][cov][sd][oc_chg][oc_dsg][chk]
 */
#define PROT_THR_FLASH_ADDR   (0x08000000UL + (512UL * 1024UL) - 4096UL)
#define PROT_THR_FLASH_MAGIC  0xA520U
#define PROT_THR_FLASH_MAGIC2 0xA521U

uint16_t power_cell_uv_mv = POWER_CELL_UV_MV_DEFAULT;
uint16_t power_cell_ov_mv = POWER_CELL_OV_MV_DEFAULT;
uint16_t power_shutdown_cell_mv = POWER_SHUTDOWN_CELL_MV_DEFAULT;
uint16_t power_oc_chg_ma = POWER_OC_CHG_MA_DEFAULT;
uint16_t power_oc_dsg_ma = POWER_OC_DSG_MA_DEFAULT;

/* Last successful cell block parse (mV). Invalid cells stored as 0. */
static uint16_t s_cell_mv[BQ76907_CELL_COUNT];
static uint8_t  s_cell_valid_count;
/* Set while any cell < POWER_CELL_UV_MV / > OV or software OC; blocks EnableDischarge */
static uint8_t  s_cell_uv_active;
static uint8_t  s_cell_ov_active;
static uint8_t  s_oc_active;
/* Edge: one BQ76907 RESET() per CUV/COV event until cells recover */
static uint8_t  s_cuv_cov_reset_done;
/* 0 until BootInit finishes CFG (HOST_FETON etc.); block host FET writes before that */
static uint8_t  s_pwr_ready;
static uint8_t  s_shutdown_abort_done;
static uint8_t  s_fet_ctrl_rb;
static uint8_t  s_fet_options_rb;
static uint8_t  s_safety_b_rb;
static uint16_t s_alarm_status_rb;
/* Sticky: last reason FETs were forced off (or DSG cut for CUV recovery) */
static uint16_t s_fet_off_reason;
static uint16_t s_fet_off_detail;
static uint16_t s_fet_off_hb_v;
static uint16_t s_fet_off_hb_cell;
static uint16_t s_fet_off_wr_idx; /* next free record slot in page */
static uint8_t  s_prev_any_fet_on = 1;
static uint8_t  s_prev_dsg_on = 1;
static uint16_t s_cell_min_mv;
static uint16_t s_cell_max_mv;
static uint8_t  s_ext_present;
static uint8_t  s_ext_cache_valid;
static uint8_t  s_ext_quiet_cnt;
static uint8_t  s_ext_probe_cd;
static uint8_t  s_ext_chg_cnt;
static uint8_t  s_ext_cdraw_low;
/*
 * Any charger plug-in: wait POWER_BOOT_WAIT_SEC before opening CHG.
 * 0=unplugged/idle, 1=waiting after plug, 2=delay done (charge allowed).
 */
static uint8_t    s_plug_chg_hold;
static TickType_t s_plug_chg_deadline;
static uint8_t    s_adapter_latched;   /* sticky until clear unplug */
static uint8_t    s_adapter_gone_cnt;
static uint8_t    s_chg_low_i_cnt;     /* hold==2: debounce leave on low I */
static TickType_t s_chg_probe_next;    /* periodic CHG pulse if no CDRAW */

static int16_t BQ76907_Abs16(int16_t v);
static uint8_t BQ76907_FetsDriverOn(uint16_t bat);
static uint8_t BQ76907_WriteFetCtrl(uint8_t v);
static uint8_t BQ76907_CuvDsgOffChgOn(void);
static uint8_t BQ76907_CuvDsgOffChgOnGated(void);
static void BQ76907_PlugChgHoldReset(void);
static void BQ76907_PlugChgHoldAllowNow(void);
static uint8_t BQ76907_PlugChgAllowed(void);
static uint8_t BQ76907_HoldChgOffDsgOn(void);
static uint8_t BQ76907_AdapterPresent(void);

/* Instant CHG pin detector — not debounced CHGDETFLAG. */
static uint8_t BQ76907_ReadChgDetRaw(uint8_t *det)
{
	uint16_t raw;

	if(det == 0)
		return 1;
	if(BQ76907_I2C_ReadReg(BQ76907_REG_ALARM_RAW, &raw) != 0)
		return 1;
	*det = (raw & BQ76907_ALARM_CDRAW) ? 1 : 0;
	return 0;
}

/* Program 4 halfwords into already-erased Flash — must stay <1 ms (no page erase). */
static uint8_t BQ76907_FlashProgramRec(uint32_t addr, uint16_t magic, uint16_t a, uint16_t b)
{
	/* ESP: no STM32 flash page — RAM sticky log only. */
	(void)addr;
	(void)magic;
	(void)a;
	(void)b;
	return 0;
}

void BQ76907_FetOffFlash_Prepare(void)
{
	/* ESP: no flash page erase; keep RAM sticky reason. */
	s_fet_off_wr_idx = 0;
}

static void BQ76907_FetOffFlash_Append(uint16_t magic, uint16_t a, uint16_t b)
{
	/* ESP: FET-off / heartbeat stay RAM-sticky; no flash append. */
	(void)magic;
	(void)a;
	(void)b;
}

void BQ76907_FetOffLog_Restore(void)
{
	/* ESP: FET-off log is RAM-sticky only. */
	test[PWR_TEST_FET_OFF_REASON] = s_fet_off_reason;
	test[PWR_TEST_FET_OFF_DETAIL] = s_fet_off_detail;
	test[PWR_TEST_FET_OFF_HB_V] = s_fet_off_hb_v;
	test[PWR_TEST_FET_OFF_HB_CELL] = s_fet_off_hb_cell;
}

static void BQ76907_LogFetOff(uint16_t reason, uint16_t detail)
{
	if(reason == FET_OFF_NONE)
		return;

	/* Same event already latched — keep first-trip detail, skip Flash spam */
	if(s_fet_off_reason == reason && s_fet_off_detail == detail)
	{
		test[PWR_TEST_FET_OFF_REASON] = reason;
		test[PWR_TEST_FET_OFF_DETAIL] = detail;
		return;
	}
	/*
	 * Soft OC: never replace a real trip current with a later smaller reading
	 * (after FETs open, |I| collapses — was showing up as detail=192).
	 */
	if((reason == FET_OFF_SOFT_OC_CHG || reason == FET_OFF_SOFT_OC_DSG)
	   && (s_fet_off_reason == FET_OFF_SOFT_OC_CHG
	       || s_fet_off_reason == FET_OFF_SOFT_OC_DSG)
	   && detail < s_fet_off_detail)
	{
		test[PWR_TEST_FET_OFF_REASON] = s_fet_off_reason;
		test[PWR_TEST_FET_OFF_DETAIL] = s_fet_off_detail;
		return;
	}

	/* New reason, or first OC/UV log: publish + append Flash before FET cut */
	test[PWR_TEST_FET_OFF_REASON] = reason;
	test[PWR_TEST_FET_OFF_DETAIL] = detail;
	s_fet_off_reason = reason;
	s_fet_off_detail = detail;
	BQ76907_FetOffFlash_Append(FET_OFF_FLASH_MAGIC, reason, detail);
}

static void BQ76907_FetOffHeartbeat(uint16_t voltage_x10)
{
	static uint8_t div;

	if((++div & 0x07U) != 0) /* ~1.6 s at 200 ms LED loop */
		return;
	if(voltage_x10 == 0 && s_cell_min_mv == 0)
		return;

	s_fet_off_hb_v = voltage_x10;
	s_fet_off_hb_cell = s_cell_min_mv;
	test[PWR_TEST_FET_OFF_HB_V] = s_fet_off_hb_v;
	test[PWR_TEST_FET_OFF_HB_CELL] = s_fet_off_hb_cell;
	BQ76907_FetOffFlash_Append(FET_OFF_HB_MAGIC, voltage_x10, s_cell_min_mv);
}

/* Infer cause from soft latches / chip Safety A — used on every FET-off edge.
 * Do NOT refresh detail from live current/cells: after FETs cut, I drops and
 * would overwrite the real trip current (e.g. OC logged as 192 mA). */
static void BQ76907_CaptureFetOffReason(uint16_t fallback)
{
	uint8_t safety = 0;

	if(s_oc_active)
	{
		if(s_fet_off_reason != FET_OFF_SOFT_OC_CHG
		   && s_fet_off_reason != FET_OFF_SOFT_OC_DSG)
			BQ76907_LogFetOff(fallback ? fallback : FET_OFF_SOFT_OC_DSG,
					  s_fet_off_detail);
		return;
	}
	if(s_cell_ov_active)
	{
		if(s_fet_off_reason != FET_OFF_SOFT_COV
		   && s_fet_off_reason != FET_OFF_CHIP_COV)
			BQ76907_LogFetOff(FET_OFF_SOFT_COV, s_cell_max_mv);
		return;
	}
	if(s_cell_uv_active)
	{
		if(s_fet_off_reason != FET_OFF_SOFT_CUV
		   && s_fet_off_reason != FET_OFF_CHIP_CUV
		   && s_fet_off_reason != FET_OFF_PACK_UV)
			BQ76907_LogFetOff(FET_OFF_SOFT_CUV, s_cell_min_mv);
		return;
	}
	if(BQ76907_ReadSafetyStatusA(&safety) == 0)
	{
		if(safety & BQ76907_SAFETY_A_CUV)
			BQ76907_LogFetOff(FET_OFF_CHIP_CUV, safety);
		else if(safety & BQ76907_SAFETY_A_COV)
			BQ76907_LogFetOff(FET_OFF_CHIP_COV, safety);
		else if(safety & (BQ76907_PROT_A_SCD | BQ76907_PROT_A_OCD1
				  | BQ76907_PROT_A_OCD2 | BQ76907_PROT_A_OCC))
			BQ76907_LogFetOff(FET_OFF_CHIP_OC, safety);
		else
			BQ76907_LogFetOff(fallback ? fallback : FET_OFF_CHIP_OTHER, safety);
		return;
	}
	BQ76907_LogFetOff(fallback ? fallback : FET_OFF_CHIP_OTHER, 0);
}

/* Any CHG/DSG on→off or DSG-only cut: always refresh sticky reason */
static void BQ76907_WatchFetOffEdge(void)
{
	uint16_t bat;
	uint8_t any_on = 0;
	uint8_t dsg_on = 0;

	if(BQ76907_ReadBatteryStatus(&bat) != 0)
		return;

	any_on = BQ76907_FetsDriverOn(bat);
	dsg_on = (bat & BQ76907_BAT_DSG) ? 1 : 0;

	if(s_prev_any_fet_on && !any_on)
		BQ76907_CaptureFetOffReason(FET_OFF_CHIP_OTHER);
	else if(s_prev_dsg_on && !dsg_on)
		BQ76907_CaptureFetOffReason(FET_OFF_SOFT_CUV);

	s_prev_any_fet_on = any_on;
	s_prev_dsg_on = dsg_on;
}

static uint8_t BQ76907_WriteDataMemoryOnce(uint16_t addr, const uint8_t *buf, uint8_t len);
static uint8_t BQ76907_ConfigProtections(void);
static uint8_t BQ76907_EnsureFullAccess(void);
static uint8_t BQ76907_CovApplyPolicy(void);
static uint8_t BQ76907_ForceFetsOn(void);

static uint8_t BQ76907_SubCommand(uint16_t cmd)
{
	if(BQ76907_I2C_WriteReg(BQ76907_REG_SUBCMD, cmd) != 0)
		return 1;

	delay_ms(2);
	return 0;
}

/* Write subcommand then read len bytes from transfer buffer 0x40. */
static uint8_t BQ76907_ReadSubcommand(uint16_t cmd, uint8_t *buf, uint8_t len)
{
	uint16_t echo;

	if(buf == 0 || len == 0)
		return 1;

	if(BQ76907_I2C_WriteReg(BQ76907_REG_SUBCMD, cmd) != 0)
		return 1;

	delay_ms(2);

	if(BQ76907_I2C_ReadReg(BQ76907_REG_SUBCMD, &echo) != 0)
		return 1;
	if(echo != cmd)
		return 1;

	return BQ76907_I2C_ReadBlock(BQ76907_REG_SUBCMD_BUF, buf, len);
}

static uint32_t BQ76907_Le32(const uint8_t *p)
{
	return (uint32_t)p[0]
		| ((uint32_t)p[1] << 8)
		| ((uint32_t)p[2] << 16)
		| ((uint32_t)p[3] << 24);
}

static uint8_t BQ76907_CheckComm(void)
{
	uint16_t status;

	if(BQ76907_I2C_ReadReg(BQ76907_REG_CTRL_STATUS, &status) != 0)
	{
		return 1;
	}
	if(status == 0xFFFF)
	{
		return 1;
	}

	return 0;
}

uint8_t BQ76907_Init(void)
{
	uint8_t addr = 0xFF;

	BQ76907_I2C_Init();
	delay_ms(100);
	BQ76907_I2C_BusRecovery();
	delay_ms(10);

	if(BQ76907_I2C_Scan(&addr) != 0)
		return 1;

	if(BQ76907_CheckComm() == 0)
		return 0;

	BQ76907_SubCommand(BQ76907_CMD_EXIT_DEEPSLEEP);
	delay_ms(10);

	if(BQ76907_CheckComm() != 0)
		return 1;

	return 0;
}

uint8_t BQ76907_ReadBatteryStatus(uint16_t *status)
{
	if(status == 0)
		return 1;

	return BQ76907_I2C_ReadReg(BQ76907_REG_BAT_STATUS, status);
}

uint8_t BQ76907_ReadCurrentRaw_mA(int16_t *current_ma)
{
	uint16_t raw;
	int32_t scaled;

	if(current_ma == 0)
		return 1;
	/* CC2 0x3A: faster update; CC1 0x3C is slower (250 ms) */
	if(BQ76907_I2C_ReadReg(BQ76907_REG_CURRENT, &raw) != 0)
		return 1;

	scaled = (int32_t)(int16_t)raw;
	scaled = (scaled * (int32_t)BQ76907_CURRENT_SCALE_NUM)
		 / (int32_t)BQ76907_CURRENT_SCALE_DEN;
	if(scaled > 32767)
		scaled = 32767;
	if(scaled < -32768)
		scaled = -32768;
	*current_ma = (int16_t)scaled;
	return 0;
}

uint8_t BQ76907_ReadCurrent_mA(int16_t *current_ma)
{
	int16_t raw;

	if(BQ76907_ReadCurrentRaw_mA(&raw) != 0)
		return 1;

	*current_ma = (int16_t)(BQ76907_CURRENT_SIGN * raw);
	return 0;
}

static int16_t BQ76907_Abs16(int16_t v)
{
	return (v < 0) ? (int16_t)(-v) : v;
}

static uint8_t BQ76907_CurrentSampleValid(int16_t sample)
{
	if(BQ76907_Abs16(sample) > BQ76907_CURRENT_MAX_MA)
		return 0;
	return 1;
}

static int16_t s_current_filtered_ma = 0;
static uint8_t s_current_filter_ready = 0;

static void BQ76907_ResetCurrentFilter(void)
{
	s_current_filtered_ma = 0;
	s_current_filter_ready = 0;
}

static int16_t BQ76907_ReadCurrentFiltered_mA(void)
{
	int16_t sample;

	if(BQ76907_ReadCurrent_mA(&sample) != 0)
	{
		if(s_current_filter_ready)
			return s_current_filtered_ma;
		return 0;
	}

	if(BQ76907_CurrentSampleValid(sample) == 0)
	{
		if(s_current_filter_ready)
			return s_current_filtered_ma;
		return 0;
	}

	if(s_current_filter_ready == 0)
	{
		s_current_filtered_ma = sample;
		s_current_filter_ready = 1;
		return s_current_filtered_ma;
	}

	/* Light IIR only — do not reject large real steps (e.g. 0 → 2000 mA) */
	s_current_filtered_ma = (int16_t)(((int32_t)s_current_filtered_ma * 3 + sample) / 4);
	return s_current_filtered_ma;
}

uint8_t BQ76907_IsCharging(int16_t *current_ma)
{
	uint16_t bat;
	int16_t current;
	static uint8_t charging = 0;
	static uint8_t on_cnt = 0;
	static uint8_t off_cnt = 0;

	if(current_ma != 0)
		*current_ma = 0;

	if(BQ76907_ReadBatteryStatus(&bat) != 0)
		return charging;

	if(bat & BQ76907_BAT_SLEEP)
	{
		charging = 0;
		on_cnt = 0;
		off_cnt = 0;
		BQ76907_ResetCurrentFilter();
		return 0;
	}

	current = BQ76907_ReadCurrentFiltered_mA();

	if(current_ma != 0)
		*current_ma = current;

	/* Sustained charge into pack — ignore trickle/noise below LED threshold */
	if(current >= (int16_t)BQ76907_CHARGE_LED_MIN_MA)
	{
		off_cnt = 0;
		if(on_cnt < 255)
			on_cnt++;
		if(on_cnt >= BQ76907_CHARGE_ON_CYCLES)
			charging = 1;
	}
	else
	{
		on_cnt = 0;
		if(current <= 0)
		{
			/* Discharge / idle — stop blink immediately */
			charging = 0;
			off_cnt = 0;
		}
		else if(off_cnt < 255)
		{
			off_cnt++;
			if(off_cnt >= BQ76907_CHARGE_OFF_CYCLES)
				charging = 0;
		}
	}

	return charging;
}

/* 0x02 Alert A (LSB) + 0x03 Status A (MSB) in one 16-bit read */
uint8_t BQ76907_ReadSafetyStatusA(uint8_t *status)
{
	uint16_t v;

	if(status == 0)
		return 1;
	if(BQ76907_I2C_ReadReg(0x02, &v) != 0)
		return 1;
	*status = (uint8_t)(v >> 8); /* Safety Status A @ 0x03 */
	return 0;
}

uint8_t BQ76907_ReadSafetyAlertA(uint8_t *alert)
{
	uint16_t v;

	if(alert == 0)
		return 1;
	if(BQ76907_I2C_ReadReg(0x02, &v) != 0)
		return 1;
	*alert = (uint8_t)(v & 0xFF); /* Safety Alert A @ 0x02 */
	return 0;
}

uint8_t BQ76907_IsCellUvLatched(void)
{
	return (s_cell_uv_active || s_cell_ov_active || s_oc_active) ? 1 : 0;
}

/*
 * External / charger present (LED43 green when set).
 * Do NOT trust CHGDET while CHG FET is on (gate pulls CHG pin high).
 *
 * Green: charge current, or CDRAW=1 (adapter), or latched until CDRAW=0.
 * Red:   CDRAW=0 (incl. battery + heavy load — must probe, not sticky-hold).
 * Probe whenever I<=0 while latched; CDRAW distinguishes chg+load vs battery.
 */
uint8_t BQ76907_IsExternalPowerPresent(void)
{
	uint16_t bat;
	int16_t ma;
	uint8_t cdraw = 0;

	if(s_ext_cache_valid)
		return s_ext_present;

	s_ext_cache_valid = 1;

	if(s_ext_probe_cd)
		s_ext_probe_cd--;

	if(BQ76907_ReadBatteryStatus(&bat) != 0)
		return s_ext_present;

	ma = BQ76907_ReadCurrentFiltered_mA();

	/*
	 * Solid charge current → external power present.
	 * Below charge threshold (incl. idle ~0 / small discharge like -20 mA) →
	 * not charging: clear sticky. Only CDRAW/CHGDET with CHG off can keep green
	 * (adapter idle / float).
	 */
	if(ma >= (int16_t)BQ76907_CHARGE_LED_MIN_MA)
	{
		s_ext_quiet_cnt = 0;
		s_ext_cdraw_low = 0;
		s_ext_chg_cnt = BQ76907_CHARGE_ON_CYCLES;
		s_ext_present = 1;
		return 1;
	}

	if(ma < (int16_t)BQ76907_CHARGE_CURRENT_MIN_MA)
	{
		s_ext_chg_cnt = 0;
		s_ext_quiet_cnt = 0;
		s_ext_cdraw_low = 0;
		/*
		 * Not charging (0 / -20 mA idle, or small noise). Clear sticky.
		 * With CHG off, only CDRAW counts — do not OR CHGDET (can stick high).
		 */
		if((bat & BQ76907_BAT_CHG) == 0)
		{
			if(BQ76907_ReadChgDetRaw(&cdraw) == 0)
				s_ext_present = cdraw;
			else
				s_ext_present = 0;
			return s_ext_present;
		}
		/* CHG still on but not charging → treat as no ext for flag */
		s_ext_present = 0;
		return 0;
	}

	/* CHG FET off: CDRAW decides (battery + load → red; adapter → green) */
	if((bat & BQ76907_BAT_CHG) == 0)
	{
		if(BQ76907_ReadChgDetRaw(&cdraw) == 0)
			s_ext_present = cdraw;
		else
			s_ext_present = (bat & BQ76907_BAT_CHGDET) ? 1 : 0;
		if(bat & BQ76907_BAT_CHGDET)
			s_ext_present = 1;
		s_ext_quiet_cnt = 0;
		s_ext_chg_cnt = 0;
		s_ext_cdraw_low = 0;
		return s_ext_present;
	}

	/* Trickle 10..49 mA: debounce before latching green */
	s_ext_quiet_cnt = 0;
	s_ext_cdraw_low = 0;
	if(s_ext_chg_cnt < 255)
		s_ext_chg_cnt++;
	if(s_ext_chg_cnt >= BQ76907_CHARGE_ON_CYCLES)
		s_ext_present = 1;
	return s_ext_present;
}

static void BQ76907_EnsureFetEnable(void)
{
	uint16_t bat;

	if(BQ76907_ReadBatteryStatus(&bat) != 0)
		return;
	if(bat & BQ76907_BAT_SLEEP)
		BQ76907_SubCommand(BQ76907_CMD_SLEEP_DISABLE);
	if((bat & BQ76907_BAT_FET_EN) == 0)
	{
		BQ76907_SubCommand(BQ76907_CMD_FET_ENABLE);
		delay_ms(5);
	}
}

/* H1: one data byte (WriteReg would also write REGOUT @ 0x69). */
static uint8_t BQ76907_WriteFetCtrl(uint8_t v)
{
	s_fet_ctrl_rb = v;
	return BQ76907_I2C_WriteByte((uint8_t)BQ76907_REG_FET_CTRL, v);
}

static uint8_t BQ76907_ReadFetCtrl(uint8_t *v)
{
	uint16_t w;

	if(v == 0)
		return 1;
	if(BQ76907_I2C_ReadReg(BQ76907_REG_FET_CTRL, &w) != 0)
		return 1;
	*v = (uint8_t)(w & 0xFF);
	s_fet_ctrl_rb = *v;
	return 0;
}

/* 0x009B PROT_RECOVERY — same buffered write protocol as data memory */
static uint8_t BQ76907_ProtRecoveryAll(void)
{
	uint8_t rec = 0xFE; /* VOLT|DIAG|SCD|OCD1|OCD2|OCC|TEMP */

	return BQ76907_WriteDataMemoryOnce(BQ76907_CMD_PROT_RECOVERY, &rec, 1);
}

static void BQ76907_ClearAlarmStatus(void)
{
	/* Alarm Status: write 1 to clear latched bits (SHUTV/XCHG/XDSG/…) */
	(void)BQ76907_I2C_WriteReg(0x62, 0xFFFF);
}

static uint8_t BQ76907_FetsDriverOn(uint16_t bat)
{
	return (bat & (BQ76907_BAT_CHG | BQ76907_BAT_DSG)) ? 1 : 0;
}

/* Host: DSG off + CHG on (0x06). Do NOT write 0x00 first (both-OFF trap). */
static uint8_t BQ76907_CuvApplyFetCtrl(void)
{
	uint16_t bat;
	uint8_t ctrl = 0;
	const uint8_t want = (uint8_t)(BQ76907_FET_DSG_OFF | BQ76907_FET_CHG_ON); /* 0x06 */

	BQ76907_EnsureFetEnable();
	(void)BQ76907_WriteFetCtrl(want);
	delay_ms(15);
	(void)BQ76907_ReadFetCtrl(&ctrl);
	if(ctrl != want)
	{
		(void)BQ76907_WriteFetCtrl(want);
		delay_ms(15);
		(void)BQ76907_ReadFetCtrl(&ctrl);
	}
	if(BQ76907_ReadBatteryStatus(&bat) != 0)
		return 1;
	/*
	 * Must be CHG on AND DSG off. Accepting "any CHG" was wrong: both-ON
	 * (test[100]=3) looked like success and skipped EXIT_DEEPSLEEP → no charge.
	 */
	if((bat & BQ76907_BAT_CHG) != 0 && (bat & BQ76907_BAT_DSG) == 0)
		return 0;
	return 1;
}

/*
 * Soft/HW CUV: DSG off, CHG on — allows charger to recover the pack.
 * After deep UV / chip CUV latch, need PROT_RECOVERY + EXIT_DEEPSLEEP
 * only when CHG is still off. If CHG is already on (charging up from UV),
 * do NOT keep calling PROT_RECOVERY — cells are still below CUV so the
 * chip re-trips immediately and glitches the charge path.
 */
static uint8_t BQ76907_CuvDsgOffChgOn(void)
{
	uint16_t bat;
	uint8_t dsg_was_on = 0;
	uint8_t ctrl = 0;
	const uint8_t want = (uint8_t)(BQ76907_FET_DSG_OFF | BQ76907_FET_CHG_ON); /* 0x06 */

	if(!s_pwr_ready)
		return 1;

	IWDG_ReloadCounter();
	if(BQ76907_ReadBatteryStatus(&bat) == 0)
	{
		(void)BQ76907_ReadFetCtrl(&ctrl);
		/* Already in charge-recover — skip heavy EXIT_DEEPSLEEP */
		if((bat & BQ76907_BAT_CHG) != 0
		   && (bat & BQ76907_BAT_DSG) == 0
		   && ctrl == want)
			return 0;

		/*
		 * CHG already on while still UV: pack is charging up.
		 * Refresh FET_CTRL only — never PROT_RECOVERY here.
		 */
		if((bat & BQ76907_BAT_CHG) != 0)
		{
			BQ76907_EnsureFetEnable();
			if(ctrl != want)
				(void)BQ76907_WriteFetCtrl(want);
			return 0;
		}

		dsg_was_on = (bat & BQ76907_BAT_DSG) ? 1 : 0;
	}
	if(dsg_was_on)
		BQ76907_CaptureFetOffReason(FET_OFF_SOFT_CUV);

	/* CHG off — open charge path once */
	(void)BQ76907_ProtRecoveryAll();
	BQ76907_ClearAlarmStatus();
	if(BQ76907_CuvApplyFetCtrl() == 0)
		return 0;

	/* Soft-shutdown / latched CUV / both-ON stuck: abort then force 0x06 */
	IWDG_ReloadCounter();
	BQ76907_SubCommand(BQ76907_CMD_EXIT_DEEPSLEEP);
	delay_ms(50);
	BQ76907_I2C_BusRecovery();
	delay_ms(20);
	(void)BQ76907_EnsureFullAccess();
	(void)BQ76907_ProtRecoveryAll();
	BQ76907_ClearAlarmStatus();
	if(BQ76907_CuvApplyFetCtrl() == 0)
		return 0;

	/* FET test mode last resort — leave DSG_OFF|CHG_ON, restore FET_EN */
	if(BQ76907_ReadBatteryStatus(&bat) == 0 && (bat & BQ76907_BAT_FET_EN) != 0)
	{
		BQ76907_SubCommand(BQ76907_CMD_FET_ENABLE);
		delay_ms(10);
		(void)BQ76907_WriteFetCtrl(want);
		delay_ms(20);
		if(BQ76907_ReadBatteryStatus(&bat) == 0
		   && (bat & BQ76907_BAT_CHG) != 0
		   && (bat & BQ76907_BAT_DSG) == 0)
		{
			BQ76907_SubCommand(BQ76907_CMD_FET_ENABLE);
			delay_ms(10);
			(void)BQ76907_WriteFetCtrl(want);
			return 0;
		}
		BQ76907_SubCommand(BQ76907_CMD_FET_ENABLE);
		delay_ms(10);
		(void)BQ76907_WriteFetCtrl(want);
	}

	return 1;
}

/* Host: force CHG off + DSG on (0x09). Do NOT write 0x00 first — that can
 * leave both FETs off if the follow-up write fails (seen as test[100]=0). */
static uint8_t BQ76907_CovApplyFetCtrl(void)
{
	uint16_t bat;
	uint8_t ctrl = 0;
	const uint8_t want = (uint8_t)(BQ76907_FET_CHG_OFF | BQ76907_FET_DSG_ON); /* 0x09 */

	BQ76907_EnsureFetEnable();
	(void)BQ76907_WriteFetCtrl(want);
	delay_ms(15);
	(void)BQ76907_ReadFetCtrl(&ctrl);
	if(ctrl != want)
	{
		(void)BQ76907_WriteFetCtrl(want);
		delay_ms(15);
		(void)BQ76907_ReadFetCtrl(&ctrl);
	}
	if(BQ76907_ReadBatteryStatus(&bat) != 0)
		return 1;
	/* Success: DSG on AND CHG off (both-ON is not COV recover) */
	if((bat & BQ76907_BAT_DSG) != 0 && (bat & BQ76907_BAT_CHG) == 0)
		return 0;
	return 1;
}

/*
 * Soft/HW COV: CHG off, DSG on — allows load to pull cells below OV recover.
 * After brown-out with host both-OFF, chip may need EXIT_DEEPSLEEP + PROT_RECOVERY
 * before DSG will reopen. Always leave FET_CTRL=0x09 (never 0x00).
 */
static uint8_t BQ76907_CovChgOffDsgOn(void)
{
	uint16_t bat;
	uint8_t chg_was_on = 0;
	uint8_t ctrl = 0;

	if(!s_pwr_ready)
		return 1;

	IWDG_ReloadCounter();
	if(BQ76907_ReadBatteryStatus(&bat) == 0)
	{
		(void)BQ76907_ReadFetCtrl(&ctrl);
		/* Already forcing discharge-recover — skip heavy EXIT_DEEPSLEEP */
		if((bat & BQ76907_BAT_DSG) != 0
		   && (bat & BQ76907_BAT_CHG) == 0
		   && ctrl == (uint8_t)(BQ76907_FET_CHG_OFF | BQ76907_FET_DSG_ON))
			return 0;
		chg_was_on = (bat & BQ76907_BAT_CHG) ? 1 : 0;
	}
	if(chg_was_on)
		BQ76907_CaptureFetOffReason(FET_OFF_SOFT_COV);

	(void)BQ76907_ProtRecoveryAll();
	BQ76907_ClearAlarmStatus();
	if(BQ76907_CovApplyFetCtrl() == 0)
		return 0;

	/* Abort soft-shutdown left from prior both-OFF / brown-out */
	IWDG_ReloadCounter();
	BQ76907_SubCommand(BQ76907_CMD_EXIT_DEEPSLEEP);
	delay_ms(50);
	BQ76907_I2C_BusRecovery();
	delay_ms(20);
	(void)BQ76907_EnsureFullAccess();
	(void)BQ76907_ProtRecoveryAll();
	BQ76907_ClearAlarmStatus();
	if(BQ76907_CovApplyFetCtrl() == 0)
		return 0;

	/*
	 * Last resort: FET test mode (FET_EN=0). FET_ENABLE toggles — if we enter
	 * test mode we MUST leave CHG_OFF|DSG_ON, then toggle back to FET_EN=1.
	 * Never exit with FET_EN=0 and FET_CTRL=0 (both FETs dead).
	 */
	if(BQ76907_ReadBatteryStatus(&bat) == 0 && (bat & BQ76907_BAT_FET_EN) != 0)
	{
		BQ76907_SubCommand(BQ76907_CMD_FET_ENABLE); /* → FET_EN=0 */
		delay_ms(10);
		(void)BQ76907_WriteFetCtrl((uint8_t)(BQ76907_FET_CHG_OFF | BQ76907_FET_DSG_ON));
		delay_ms(20);
		if(BQ76907_ReadBatteryStatus(&bat) == 0 && (bat & BQ76907_BAT_DSG) != 0)
		{
			/* Restore autonomous FET_EN; keep host CHG_OFF|DSG_ON */
			BQ76907_SubCommand(BQ76907_CMD_FET_ENABLE);
			delay_ms(10);
			(void)BQ76907_WriteFetCtrl((uint8_t)(BQ76907_FET_CHG_OFF | BQ76907_FET_DSG_ON));
			return 0;
		}
		/* Restore FET_EN even on failure */
		BQ76907_SubCommand(BQ76907_CMD_FET_ENABLE);
		delay_ms(10);
		(void)BQ76907_WriteFetCtrl((uint8_t)(BQ76907_FET_CHG_OFF | BQ76907_FET_DSG_ON));
	}

	return 1;
}

/* Shared gate: OC → both off. COV → discharge-recover. CUV → charge-recover. */
static uint8_t BQ76907_FetPathBlocked(void)
{
	uint8_t safety = 0;
	uint8_t i;

	if(s_oc_active)
	{
		BQ76907_CaptureFetOffReason(FET_OFF_SOFT_OC_DSG);
		(void)BQ76907_WriteFetCtrl(BQ76907_FET_CHG_DSG_OFF);
		return 1;
	}
	if(s_cell_ov_active)
	{
		(void)BQ76907_CovApplyPolicy();
		return 1; /* not unrestricted both-ON path */
	}
	if(s_cell_uv_active)
	{
		(void)BQ76907_CuvDsgOffChgOnGated();
		return 1; /* not both-ON path */
	}

	if(BQ76907_ReadSafetyStatusA(&safety) == 0)
	{
		if(safety & BQ76907_SAFETY_A_COV)
		{
			BQ76907_LogFetOff(FET_OFF_CHIP_COV, safety);
			s_cell_ov_active = 1;
			(void)BQ76907_CovApplyPolicy();
			return 1;
		}
		if(safety & BQ76907_SAFETY_A_CUV)
		{
			BQ76907_LogFetOff(FET_OFF_CHIP_CUV, safety);
			s_cell_uv_active = 1;
			(void)BQ76907_CuvDsgOffChgOnGated();
			return 1;
		}
	}

	for(i = 0; i < BQ76907_CELL_COUNT; i++)
	{
		if(s_cell_mv[i] > POWER_CELL_OV_MV)
		{
			BQ76907_LogFetOff(FET_OFF_SOFT_COV, s_cell_mv[i]);
			s_cell_ov_active = 1;
			(void)BQ76907_CovApplyPolicy();
			return 1;
		}
		if(s_cell_mv[i] > 0U && s_cell_mv[i] < POWER_CELL_UV_MV)
		{
			BQ76907_LogFetOff(FET_OFF_SOFT_CUV, s_cell_mv[i]);
			s_cell_uv_active = 1;
			(void)BQ76907_CuvDsgOffChgOnGated();
			return 1;
		}
	}
	return 0;
}

/*
 * Open FETs. MCU runs from PACK through series FETs — never glitch both off.
 * If SHUTDOWN was armed, EXIT_DEEPSLEEP may be needed (TRM 8.5).
 * Do NOT re-run ConfigProtections here (CFGUPDATE on battery = brown-out).
 */
static uint8_t BQ76907_ForceFetsOn(void)
{
	uint16_t bat;
	uint8_t n;

	if(s_oc_active)
		return 1;
	/* COV/CUV before "already on" early-out — must not keep CHG on during COV */
	if(s_cell_ov_active)
		return BQ76907_CovChgOffDsgOn();
	if(s_cell_uv_active)
		return BQ76907_CuvDsgOffChgOnGated();
	if(!s_pwr_ready)
		return 1;

	IWDG_ReloadCounter();
	/* Already both on — do not touch FET_CTRL (unplug used to glitch → die) */
	if(BQ76907_ReadBatteryStatus(&bat) == 0
	   && (bat & BQ76907_BAT_DSG) != 0
	   && (bat & BQ76907_BAT_CHG) != 0)
		return 0;

	BQ76907_EnsureFetEnable();

	/* 1) Host force both ON first (avoid 0x00 gap that drops pack rail) */
	if(BQ76907_WriteFetCtrl(BQ76907_FET_CHG_DSG_ON) != 0)
		return 1;
	delay_ms(15);
	if(BQ76907_ReadBatteryStatus(&bat) == 0 && BQ76907_FetsDriverOn(bat))
		return 0;

	(void)BQ76907_ProtRecoveryAll();
	BQ76907_ClearAlarmStatus();
	(void)BQ76907_WriteFetCtrl(BQ76907_FET_CHG_DSG_ON);
	delay_ms(15);
	if(BQ76907_ReadBatteryStatus(&bat) == 0 && BQ76907_FetsDriverOn(bat))
		return 0;

	/* 2) Abort soft-shutdown once — no ConfigProtections (unsafe on battery) */
	if(!s_shutdown_abort_done)
	{
		s_shutdown_abort_done = 1;
		IWDG_ReloadCounter();
		BQ76907_SubCommand(BQ76907_CMD_EXIT_DEEPSLEEP);
		delay_ms(50);
		BQ76907_I2C_BusRecovery();
		delay_ms(20);
		(void)BQ76907_EnsureFullAccess();
		BQ76907_SubCommand(BQ76907_CMD_SLEEP_DISABLE);
		(void)BQ76907_ProtRecoveryAll();
		BQ76907_ClearAlarmStatus();
		BQ76907_EnsureFetEnable();
		(void)BQ76907_WriteFetCtrl(BQ76907_FET_CHG_DSG_ON);
		delay_ms(20);
		if(BQ76907_ReadBatteryStatus(&bat) == 0 && BQ76907_FetsDriverOn(bat))
			return 0;
	}

	/* 3) FET test mode briefly, then restore FET_EN + both ON */
	if(BQ76907_ReadBatteryStatus(&bat) == 0 && (bat & BQ76907_BAT_FET_EN) != 0)
	{
		BQ76907_SubCommand(BQ76907_CMD_FET_ENABLE);
		delay_ms(10);
	}
	(void)BQ76907_WriteFetCtrl(BQ76907_FET_CHG_DSG_ON);
	delay_ms(20);
	if(BQ76907_ReadBatteryStatus(&bat) == 0 && BQ76907_FetsDriverOn(bat))
	{
		/* back to FET_EN=1 if we entered test mode */
		if(BQ76907_ReadBatteryStatus(&bat) == 0 && (bat & BQ76907_BAT_FET_EN) == 0)
		{
			BQ76907_SubCommand(BQ76907_CMD_FET_ENABLE);
			delay_ms(10);
			(void)BQ76907_WriteFetCtrl(BQ76907_FET_CHG_DSG_ON);
		}
		return 0;
	}
	if(BQ76907_ReadBatteryStatus(&bat) == 0 && (bat & BQ76907_BAT_FET_EN) == 0)
	{
		BQ76907_SubCommand(BQ76907_CMD_FET_ENABLE);
		delay_ms(10);
		(void)BQ76907_WriteFetCtrl(BQ76907_FET_CHG_DSG_ON);
	}

	for(n = 0; n < 2; n++)
	{
		(void)BQ76907_WriteFetCtrl(BQ76907_FET_CHG_DSG_ON);
		delay_ms(20);
		if(BQ76907_ReadBatteryStatus(&bat) == 0 && BQ76907_FetsDriverOn(bat))
			return 0;
	}

	(void)BQ76907_ReadFetCtrl(&s_fet_ctrl_rb);
	return 1;
}

/*
 * COV: always CHG off + DSG on — stop charge while pack can still power the board.
 * (Previously skipped CHG-off when |I| looked low → cells >3600 kept charging.)
 */
static uint8_t BQ76907_CovApplyPolicy(void)
{
	return BQ76907_CovChgOffDsgOn();
}

uint8_t BQ76907_EnableDischarge(void)
{
	if(s_cell_ov_active)
		return BQ76907_CovApplyPolicy();
	if(s_cell_uv_active)
		return BQ76907_CuvDsgOffChgOnGated();
	if(BQ76907_FetPathBlocked())
		return 1;
	/* Battery path: DSG on, CHG off — otherwise plug-in charges instantly */
	return BQ76907_HoldChgOffDsgOn();
}

uint8_t BQ76907_EnableCharge(void)
{
	int16_t ma;

	if(s_cell_ov_active)
		return BQ76907_CovApplyPolicy(); /* CHG-off only if charge current present */
	if(s_cell_uv_active)
		return BQ76907_CuvDsgOffChgOnGated();

	/* Sticky soft-OC was holding host OFF and blocked reopen */
	if(s_oc_active)
	{
		if(BQ76907_ReadCurrent_mA(&ma) != 0 || BQ76907_Abs16(ma) >= (int16_t)POWER_OC_RECOVER_MA)
			return 1;
		s_oc_active = 0;
	}
	return BQ76907_ForceFetsOn();
}

/* CHGDET/current → charge; else battery. COV: both ON if no charger. */
static uint8_t PowerMgmt_SelectPowerPath(void)
{
	uint16_t bat;

	if(s_cell_ov_active)
		return BQ76907_CovApplyPolicy();
	if(s_cell_uv_active)
		return BQ76907_CuvDsgOffChgOnGated();
	if(s_oc_active)
	{
		(void)BQ76907_DisableFets();
		return 1;
	}

	/*
	 * Charge session (hold==2): keep charging until adapter clearly gone.
	 * After CHG opens, filtered I may still be 0 for a few cycles — debounce
	 * low current (~3 s) before leaving (CHGDET can stick while CHG is on).
	 */
	if(s_plug_chg_hold == 2)
	{
		int16_t ma;

		ma = BQ76907_ReadCurrentFiltered_mA();
		if(ma >= (int16_t)BQ76907_CHARGE_CURRENT_MIN_MA)
			s_chg_low_i_cnt = 0;
		else
		{
			if(s_chg_low_i_cnt < 255)
				s_chg_low_i_cnt++;
			if(s_chg_low_i_cnt >= 15) /* ~3 s at 200 ms */
			{
				BQ76907_PlugChgHoldReset();
				return BQ76907_EnableDischarge();
			}
		}
		if(BQ76907_ReadBatteryStatus(&bat) == 0
		   && (bat & BQ76907_BAT_DSG) != 0
		   && (bat & BQ76907_BAT_CHG) != 0)
			return 0;
		return BQ76907_EnableCharge();
	}

	if(BQ76907_AdapterPresent())
	{
		if(!BQ76907_PlugChgAllowed())
			return BQ76907_HoldChgOffDsgOn();

		if(BQ76907_ReadBatteryStatus(&bat) == 0
		   && (bat & BQ76907_BAT_DSG) != 0
		   && (bat & BQ76907_BAT_CHG) != 0)
			return 0;
		return BQ76907_EnableCharge();
	}

	BQ76907_PlugChgHoldReset();
	/* No charger: keep CHG off so next plug cannot charge until delay */
	return BQ76907_EnableDischarge();
}

uint8_t BQ76907_DisableFets(void)
{
	uint16_t bat;
	uint8_t was_on = 0;

	IWDG_ReloadCounter();
	if(!s_pwr_ready)
		return 1;
	if(BQ76907_ReadBatteryStatus(&bat) == 0)
		was_on = BQ76907_FetsDriverOn(bat);
	if(was_on)
		BQ76907_CaptureFetOffReason(FET_OFF_CHIP_OTHER);
	return BQ76907_WriteFetCtrl(BQ76907_FET_CHG_DSG_OFF);
}

uint8_t BQ76907_GetFetCtrl(void)
{
	(void)BQ76907_ReadFetCtrl(&s_fet_ctrl_rb);
	return s_fet_ctrl_rb;
}

uint8_t BQ76907_GetFetOptions(void)
{
	return s_fet_options_rb;
}

uint16_t BQ76907_GetFetOffReason(void)
{
	return s_fet_off_reason;
}

uint16_t BQ76907_GetFetOffDetail(void)
{
	return s_fet_off_detail;
}

uint8_t BQ76907_GetSafetyStatusB(void)
{
	return s_safety_b_rb;
}

uint16_t BQ76907_GetAlarmStatus(void)
{
	return s_alarm_status_rb;
}

/* TRM: send 0x0010 SHUTDOWN() twice within 4 seconds */
uint8_t BQ76907_CmdShutdown(void)
{
	if(BQ76907_SubCommand(BQ76907_CMD_SHUTDOWN) != 0)
		return 1;
	delay_ms(100);
	if(BQ76907_SubCommand(BQ76907_CMD_SHUTDOWN) != 0)
		return 1;
	BQ76907_LogFetOff(FET_OFF_HOST_CMD, PWR_CMD_SHUTDOWN);
	return 0;
}

/* TRM: 0x0012 RESET() — full reset, OTP reload; then force FETs off */
uint8_t BQ76907_CmdReset(void)
{
	uint8_t rc;

	IWDG_ReloadCounter();
	if(BQ76907_SubCommand(BQ76907_CMD_RESET) != 0)
		return 1;

	IWDG_ReloadCounter();
	delay_ms(200);
	BQ76907_I2C_BusRecovery();
	IWDG_ReloadCounter();
	delay_ms(100);

	/* OTP may re-enable autonomous FETs; explicitly cut pack path */
	rc = BQ76907_DisableFets();
	BQ76907_LogFetOff(FET_OFF_HOST_CMD, PWR_CMD_RESET);
	return rc;
}

/* TRM: 0x0022 FET_ENABLE() toggles Battery Status[FET_EN] */
uint8_t BQ76907_CmdFetEnable(void)
{
	return BQ76907_SubCommand(BQ76907_CMD_FET_ENABLE);
}

/*
 * Recover after soft COV/CUV/OC locked FETs off.
 * Keep charger plugged so MCU stays up. Clears software latches, chip
 * PROT_RECOVERY, then forces CHG+DSG on.
 */
uint8_t BQ76907_CmdFetRecover(void)
{
	uint8_t i;

	if(!s_pwr_ready)
		return 1;

	s_cell_uv_active = 0;
	s_cell_ov_active = 0;
	s_oc_active = 0;
	s_cuv_cov_reset_done = 0;
	s_shutdown_abort_done = 0;

	IWDG_ReloadCounter();
	(void)BQ76907_ProtRecoveryAll();
	BQ76907_ClearAlarmStatus();
	BQ76907_EnsureFetEnable();

	for(i = 0; i < 5; i++)
	{
		IWDG_ReloadCounter();
		if(BQ76907_ForceFetsOn() == 0)
			return 0;
		delay_ms(50);
	}
	return 1;
}

static uint8_t BQ76907_ParseCellBlock(const uint8_t *buf, uint32_t *sum, uint16_t *cells)
{
	uint8_t i;
	uint8_t valid = 0;
	uint16_t cv;

	*sum = 0;

	for(i = 0; i < BQ76907_CELL_COUNT; i++)
	{
		cv = (uint16_t)buf[i * 2] | ((uint16_t)buf[i * 2 + 1] << 8);
		if(cv < BQ76907_CELL_MV_MIN_PARSE || cv > 5500)
		{
			if(cells != 0)
				cells[i] = 0;
			continue;
		}
		if(cells != 0)
			cells[i] = cv;
		*sum += cv;
		valid++;
	}

	return valid;
}

static void BQ76907_StoreCellCache(const uint16_t *cells, uint8_t valid)
{
	uint8_t i;

	for(i = 0; i < BQ76907_CELL_COUNT; i++)
		s_cell_mv[i] = (cells != 0) ? cells[i] : 0;
	s_cell_valid_count = valid;
}

static uint32_t BQ76907_ReadCellSum_mV(uint8_t *valid_count)
{
	uint8_t buf[14];
	uint16_t cells[BQ76907_CELL_COUNT];
	uint16_t best_cells[BQ76907_CELL_COUNT];
	uint32_t sum;
	uint32_t best_sum = 0;
	uint8_t valid;
	uint8_t best_valid = 0;
	uint8_t attempt;
	uint8_t i;

	if(valid_count != 0)
		*valid_count = 0;

	for(i = 0; i < BQ76907_CELL_COUNT; i++)
		best_cells[i] = 0;

	for(attempt = 0; attempt < 3; attempt++)
	{
		if(BQ76907_I2C_ReadBlock(BQ76907_REG_CELL1_V, buf, 14) != 0)
		{
			delay_ms(5);
			continue;
		}

		valid = BQ76907_ParseCellBlock(buf, &sum, cells);
		/* Prefer full 7S; still accept partial so UV can see a low cell */
		if(valid >= BQ76907_CELL_COUNT)
		{
			BQ76907_StoreCellCache(cells, valid);
			if(valid_count != 0)
				*valid_count = valid;
			return sum;
		}

		if(valid > best_valid)
		{
			best_valid = valid;
			best_sum = sum;
			for(i = 0; i < BQ76907_CELL_COUNT; i++)
				best_cells[i] = cells[i];
		}
		delay_ms(5);
	}

	/* Partial pack read is enough for UV latch */
	BQ76907_StoreCellCache(best_cells, best_valid);
	if(valid_count != 0)
		*valid_count = best_valid;

	return best_sum;
}

uint8_t BQ76907_GetCellVoltages_mV(uint16_t *cells_mv, uint8_t n, uint8_t *valid_count)
{
	uint8_t i;
	uint8_t copy_n;

	if(valid_count != 0)
		*valid_count = s_cell_valid_count;

	if(cells_mv == 0 || n == 0)
		return (s_cell_valid_count > 0) ? 0 : 1;

	copy_n = (n > BQ76907_CELL_COUNT) ? BQ76907_CELL_COUNT : n;
	for(i = 0; i < copy_n; i++)
		cells_mv[i] = s_cell_mv[i];
	for(; i < n; i++)
		cells_mv[i] = 0;

	return (s_cell_valid_count > 0) ? 0 : 1;
}

uint8_t BQ76907_ReadMonitor(BQ76907_Monitor_t *mon)
{
	uint16_t u16;
	uint8_t raw4[4];
	uint8_t passq[12];
	uint8_t ok = 0;

	if(mon == 0)
		return 1;

	mon->reg18_raw = 0;
	mon->vss_raw = 0;
	mon->stack_mv = 0;
	mon->int_temp_c = 0;
	mon->ts_raw = 0;
	mon->raw_current = 0;
	mon->current_usera = 0;
	mon->cc1_current_usera = 0;
	mon->passq_lsb = 0;
	mon->passq_msb = 0;
	mon->passtime = 0;

	if(BQ76907_I2C_ReadReg(BQ76907_REG_REG18_V, &u16) == 0)
	{
		mon->reg18_raw = u16;
		ok = 1;
	}
	if(BQ76907_I2C_ReadReg(BQ76907_REG_VSS_V, &u16) == 0)
	{
		mon->vss_raw = u16;
		ok = 1;
	}
	if(BQ76907_I2C_ReadReg(BQ76907_REG_STACK_V, &u16) == 0)
	{
		mon->stack_mv = u16;
		ok = 1;
	}
	if(BQ76907_I2C_ReadReg(BQ76907_REG_INT_TEMP, &u16) == 0)
	{
		mon->int_temp_c = (int16_t)u16;
		ok = 1;
	}
	if(BQ76907_I2C_ReadReg(BQ76907_REG_TS_MEAS, &u16) == 0)
	{
		mon->ts_raw = u16;
		ok = 1;
	}

	/* 0x36 Raw Current: 24-bit in LSBs, sign-extended to 32-bit LE */
	if(BQ76907_I2C_ReadBlock(BQ76907_REG_RAW_CURRENT, raw4, 4) == 0)
	{
		mon->raw_current = (int32_t)BQ76907_Le32(raw4);
		ok = 1;
	}

	if(BQ76907_I2C_ReadReg(BQ76907_REG_CURRENT, &u16) == 0)
	{
		mon->current_usera = (int16_t)u16;
		ok = 1;
	}
	if(BQ76907_I2C_ReadReg(BQ76907_REG_CC1_CURRENT, &u16) == 0)
	{
		mon->cc1_current_usera = (int16_t)u16;
		ok = 1;
	}

	/* PASSQ(): PASSQLSB[0..3], PASSQMSB[4..7], PASSTIME[8..11] */
	if(BQ76907_ReadSubcommand(BQ76907_CMD_PASSQ, passq, 12) == 0)
	{
		mon->passq_lsb = BQ76907_Le32(&passq[0]);
		mon->passq_msb = (int32_t)BQ76907_Le32(&passq[4]);
		mon->passtime = BQ76907_Le32(&passq[8]);
		ok = 1;
	}

	return ok ? 0 : 1;
}

static uint8_t BQ76907_ReadDataMemory(uint16_t addr, uint8_t *buf, uint8_t len)
{
	uint16_t echo;
	uint8_t attempt;

	if(buf == 0 || len == 0)
		return 1;

	for(attempt = 0; attempt < 5; attempt++)
	{
		IWDG_ReloadCounter();
		if(BQ76907_I2C_WriteReg(BQ76907_REG_SUBCMD, addr) != 0)
		{
			BQ76907_I2C_BusRecovery();
			delay_ms(5);
			continue;
		}

		delay_ms(5);

		if(BQ76907_I2C_ReadReg(BQ76907_REG_SUBCMD, &echo) != 0)
		{
			BQ76907_I2C_BusRecovery();
			delay_ms(5);
			continue;
		}
		if(echo != addr)
		{
			delay_ms(5);
			continue;
		}

		if(BQ76907_I2C_ReadBlock(BQ76907_REG_SUBCMD_BUF, buf, len) == 0)
			return 0;

		BQ76907_I2C_BusRecovery();
		delay_ms(5);
	}

	return 1;
}

static uint8_t BQ76907_WriteDataMemoryOnce(uint16_t addr, const uint8_t *buf, uint8_t len)
{
	uint8_t i;
	uint8_t sum;
	uint8_t csum;

	if(BQ76907_I2C_WriteReg(BQ76907_REG_SUBCMD, addr) != 0)
		return 1;

	if(BQ76907_I2C_WriteBlock(BQ76907_REG_SUBCMD_BUF, buf, len) != 0)
		return 1;

	/* checksum = ~sum(addr_lo + addr_hi + data[0..len-1]) */
	sum = (uint8_t)(addr & 0xFF);
	sum = (uint8_t)(sum + (uint8_t)(addr >> 8));
	for(i = 0; i < len; i++)
		sum = (uint8_t)(sum + buf[i]);
	csum = (uint8_t)(~sum);

	if(BQ76907_I2C_WriteByte(BQ76907_REG_SUBCMD_CKSUM, csum) != 0)
		return 1;
	/* length = data bytes + cmd(2) + checksum(1) + length(1) */
	if(BQ76907_I2C_WriteByte(BQ76907_REG_SUBCMD_LEN, (uint8_t)(len + 4)) != 0)
		return 1;

	delay_ms(2);
	return 0;
}

static uint8_t BQ76907_WriteDataMemory(uint16_t addr, const uint8_t *buf, uint8_t len)
{
	uint8_t attempt;

	if(buf == 0 || len == 0 || len > 32)
		return 1;

	for(attempt = 0; attempt < 3; attempt++)
	{
		if(BQ76907_WriteDataMemoryOnce(addr, buf, len) == 0)
			return 0;
		BQ76907_I2C_BusRecovery();
		delay_ms(5);
	}
	return 1;
}

static uint8_t BQ76907_WaitCfgUpdate(uint8_t enter)
{
	uint8_t i;
	uint16_t bat;

	for(i = 0; i < 100; i++)
	{
		IWDG_ReloadCounter();
		if(BQ76907_ReadBatteryStatus(&bat) == 0)
		{
			if(enter)
			{
				if(bat & BQ76907_BAT_CFGUPDATE)
					return 0;
			}
			else
			{
				if((bat & BQ76907_BAT_CFGUPDATE) == 0)
					return 0;
			}
		}
		delay_ms(10);
	}
	return 1;
}

/* Wait until SEC is FULLACCESS(1) or SEALED(3); 0 means not ready yet */
static uint8_t BQ76907_WaitSecurityReady(void)
{
	uint8_t i;
	uint16_t bat;
	uint16_t sec;

	for(i = 0; i < 50; i++)
	{
		IWDG_ReloadCounter();
		if(BQ76907_ReadBatteryStatus(&bat) == 0)
		{
			sec = (uint16_t)(bat & BQ76907_BAT_SEC_MASK);
			if(sec == BQ76907_BAT_SEC_FULLACCESS || sec == BQ76907_BAT_SEC_SEALED)
				return 0;
		}
		delay_ms(20);
	}
	return 1;
}

/* TRM: write FA key1 then key2 to 0x3E/0x3F within 5 s (LE via WriteReg) */
static uint8_t BQ76907_Unseal(void)
{
	IWDG_ReloadCounter();
	if(BQ76907_I2C_WriteReg(BQ76907_REG_SUBCMD, BQ76907_KEY_FULLACCESS_1) != 0)
		return 1;
	delay_ms(20);
	if(BQ76907_I2C_WriteReg(BQ76907_REG_SUBCMD, BQ76907_KEY_FULLACCESS_2) != 0)
		return 1;
	delay_ms(20);
	return 0;
}

static uint8_t BQ76907_EnsureFullAccess(void)
{
	uint16_t bat;
	uint16_t sec;
	uint8_t attempt;

	if(BQ76907_WaitSecurityReady() != 0)
		return 1;

	for(attempt = 0; attempt < 3; attempt++)
	{
		IWDG_ReloadCounter();
		if(BQ76907_ReadBatteryStatus(&bat) != 0)
		{
			delay_ms(20);
			continue;
		}

		sec = (uint16_t)(bat & BQ76907_BAT_SEC_MASK);
		if(sec == BQ76907_BAT_SEC_FULLACCESS)
			return 0;

		/* SEALED or unexpected — try default unseal keys */
		(void)BQ76907_Unseal();
		delay_ms(50);
	}

	if(BQ76907_ReadBatteryStatus(&bat) != 0)
		return 1;
	return ((bat & BQ76907_BAT_SEC_MASK) == BQ76907_BAT_SEC_FULLACCESS) ? 0 : 1;
}

static uint8_t BQ76907_EnterCfgUpdate(void)
{
	uint8_t attempt;

	BQ76907_SubCommand(BQ76907_CMD_SLEEP_DISABLE);
	delay_ms(5);

	if(BQ76907_EnsureFullAccess() != 0)
		return 1;

	for(attempt = 0; attempt < 3; attempt++)
	{
		IWDG_ReloadCounter();
		/*
		 * Do NOT host-disable FETs here. MCU/load on battery path would
		 * brown-out ~POWER_BOOT_WAIT_SEC when BootInit runs ConfigProtections.
		 */
		if(BQ76907_SubCommand(BQ76907_CMD_SET_CFGUPDATE) != 0)
		{
			(void)BQ76907_ForceFetsOn();
			delay_ms(50);
			continue;
		}

		if(BQ76907_WaitCfgUpdate(1) == 0)
			return 0;

		/* Still not in CFGUPDATE — re-unseal, keep FETs on, retry */
		(void)BQ76907_Unseal();
		(void)BQ76907_ForceFetsOn();
		delay_ms(50);
	}

	(void)BQ76907_ForceFetsOn();
	return 1;
}

static uint8_t BQ76907_ExitCfgUpdate(void)
{
	if(BQ76907_SubCommand(BQ76907_CMD_EXIT_CFGUPDATE) != 0)
		return 1;

	if(BQ76907_WaitCfgUpdate(0) != 0)
		return 1;

	delay_ms(50);
	(void)BQ76907_ForceFetsOn();
	return 0;
}

/* Allow host FET ON/OFF; enable CUV/OCD/OCC/SCD and set trip thresholds. */
static uint32_t BQ76907_MaToSenseMv(uint16_t ma)
{
	/* V_mV = I_mA * R_uOhm / 1e6 */
	return (((uint32_t)ma * (uint32_t)BQ76907_RSENSE_UOHM) + 500000UL) / 1000000UL;
}

/* OCD: V = 2*code mV, code 2..100 → 4..200 mV */
static uint8_t BQ76907_MaToOcdCode(uint16_t ma)
{
	uint32_t vmv = BQ76907_MaToSenseMv(ma);

	if(vmv < 4U)
		vmv = 4U;
	if(vmv > 200U)
		vmv = 200U;
	return (uint8_t)(vmv / 2U);
}

/* OCC: V = 2*code - 1 mV, code 2..62 → 3..123 mV */
static uint8_t BQ76907_MaToOccCode(uint16_t ma)
{
	uint32_t vmv = BQ76907_MaToSenseMv(ma);

	if(vmv < 3U)
		vmv = 3U;
	if(vmv > 123U)
		vmv = 123U;
	return (uint8_t)((vmv + 1U) / 2U);
}

/* SCD table index 0..15 → 10,20,40,...,500 mV */
static uint8_t BQ76907_MaToScdCode(uint16_t ma)
{
	static const uint16_t scd_mv[16] = {
		10, 20, 40, 60, 80, 100, 125, 150,
		175, 200, 250, 300, 350, 400, 450, 500
	};
	uint32_t vmv = BQ76907_MaToSenseMv(ma);
	uint8_t i;

	for(i = 0; i < 16; i++)
	{
		if(vmv <= scd_mv[i])
			return i;
	}
	return 15;
}

static uint8_t s_prot_enabled_a;
static uint8_t s_prot_verify;
static uint8_t s_prot_cfg_err; /* 0=ok/not run, 1=enter, 2=write, 3=exit, 4=verify */

/* bit7=EnabledProtA rd fail; bit6=CUV thr rd fail; bit5=COV thr rd fail */
#define BQ76907_PROT_VERIFY_RD_FAIL  0xE0U

static uint8_t BQ76907_VerifyCuvCov(void)
{
	uint8_t en = 0;
	uint16_t cuv = 0;
	uint16_t cov = 0;
	uint8_t ok = 0;

	if(BQ76907_ReadDataMemory(BQ76907_DM_ENABLED_PROT_A, &en, 1) != 0)
	{
		/* Do not clear latched good values here; RefreshProtVerify decides */
		s_prot_verify = 0x80; /* bit7: DM read failed */
		return 1;
	}

	s_prot_enabled_a = en;

	if(en & BQ76907_PROT_A_CUV)
		ok |= (1U << 0);
	if(en & BQ76907_PROT_A_COV)
		ok |= (1U << 1);

	if(BQ76907_ReadCuvThreshold_mV(&cuv) == 0)
	{
		if(cuv == POWER_CELL_UV_MV)
			ok |= (1U << 2);
	}
	else
		ok |= 0x40; /* bit6: CUV thr read fail (still show other bits) */

	if(BQ76907_ReadCovThreshold_mV(&cov) == 0)
	{
		if(cov == POWER_CELL_OV_MV)
			ok |= (1U << 3);
	}
	else
		ok |= 0x20; /* bit5: COV thr read fail */

	s_prot_verify = ok;
	return ((ok & 0x0F) == 0x0F) ? 0 : 1;
}

uint8_t BQ76907_GetProtEnabledA(void)
{
	return s_prot_enabled_a;
}

uint8_t BQ76907_GetProtVerify(void)
{
	return s_prot_verify;
}

uint8_t BQ76907_GetProtCfgErr(void)
{
	return s_prot_cfg_err;
}

uint8_t BQ76907_RefreshProtVerify(void)
{
	uint8_t prev_v = s_prot_verify;
	uint8_t prev_en = s_prot_enabled_a;
	uint8_t ret = BQ76907_VerifyCuvCov();

	/* Keep last good status across transient I2C/DM read glitches */
	if(ret != 0 && (prev_v & 0x0F) == 0x0F && prev_en != 0
	   && (s_prot_verify & BQ76907_PROT_VERIFY_RD_FAIL) != 0)
	{
		s_prot_verify = prev_v;
		s_prot_enabled_a = prev_en;
		return 0;
	}
	return ret;
}

static uint8_t BQ76907_ConfigProtections(void)
{
	uint8_t opt;
	uint8_t prot_a;
	uint8_t v;
	uint8_t buf[2];
	uint8_t attempt;

	s_prot_enabled_a = 0;
	s_prot_verify = 0;
	s_prot_cfg_err = 0;

	for(attempt = 0; attempt < 3; attempt++)
	{
		IWDG_ReloadCounter();
		if(BQ76907_EnterCfgUpdate() != 0)
		{
			s_prot_cfg_err = 1;
			delay_ms(50);
			continue;
		}

		/* Disable SLEEP by default (stops Battery Status bit15 toggling) */
		if(BQ76907_ReadDataMemory(BQ76907_DM_POWER_CONFIG, &opt, 1) != 0)
			goto cfg_fail;
		opt = (uint8_t)(opt & (uint8_t)(~BQ76907_PWRCFG_SLEEP_EN));
		if(BQ76907_WriteDataMemory(BQ76907_DM_POWER_CONFIG, &opt, 1) != 0)
			goto cfg_fail;

		/* 7S: all cell inputs used so CUV applies to every cell */
		v = BQ76907_VCELL_MODE_7S;
		if(BQ76907_WriteDataMemory(BQ76907_DM_VCELL_MODE, &v, 1) != 0)
			goto cfg_fail;

		/*
		 * FET Options absolute: HOST_FETON/OFF + CHGDET + FET_EN; SFET=0
		 * (SFET would re-enable DSG under load while CHG-on/DSG-off → drain).
		 */
		opt = (uint8_t)(BQ76907_FETOPT_FET_EN
				| BQ76907_FETOPT_HOST_FETON_EN | BQ76907_FETOPT_HOST_FETOFF_EN
				| BQ76907_FETOPT_CHGDETEN);
		if(BQ76907_WriteDataMemory(BQ76907_DM_FET_OPTIONS, &opt, 1) != 0)
			goto cfg_fail;
		s_fet_options_rb = opt;

		/* Enable COV/CUV/SCD/OCD1/OCD2/OCC (+ keep REGOUT) */
		prot_a = (uint8_t)(BQ76907_PROT_A_COV | BQ76907_PROT_A_CUV | BQ76907_PROT_A_SCD
				 | BQ76907_PROT_A_OCD1 | BQ76907_PROT_A_OCD2 | BQ76907_PROT_A_OCC
				 | BQ76907_PROT_A_REGOUT);
		if(BQ76907_WriteDataMemory(BQ76907_DM_ENABLED_PROT_A, &prot_a, 1) != 0)
			goto cfg_fail;

		/*
		 * DSG FET Prot A: CUV/SCD/OCD… (no COV bit) — DSG stays on during COV.
		 * CHG FET Prot A: COV/SCD/OCC… — CHG off on COV (OTP default 0xEF).
		 * Was 0xFF/0xFF; host then both-OFF left pack stuck full (no discharge).
		 */
		v = 0xFF; /* CUV|SCD|OCD1|OCD2|HWD|OTD|UTD|OTINT */
		if(BQ76907_WriteDataMemory(BQ76907_DM_DSG_FET_PROT_A, &v, 1) != 0)
			goto cfg_fail;
		v = 0xEF; /* COV|SCD|OCC|HWD|OTC|UTC|OTINT — OTP default */
		if(BQ76907_WriteDataMemory(BQ76907_DM_CHG_FET_PROT_A, &v, 1) != 0)
			goto cfg_fail;

		/* Hardware CUV */
		buf[0] = (uint8_t)(POWER_CELL_UV_MV & 0xFF);
		buf[1] = (uint8_t)(POWER_CELL_UV_MV >> 8);
		if(BQ76907_WriteDataMemory(BQ76907_DM_CUV_THRESHOLD, buf, 2) != 0)
			goto cfg_fail;
		v = BQ76907_CUV_DELAY_CODE;
		if(BQ76907_WriteDataMemory(BQ76907_DM_CUV_DELAY, &v, 1) != 0)
			goto cfg_fail;

		/* Hardware COV */
		buf[0] = (uint8_t)(POWER_CELL_OV_MV & 0xFF);
		buf[1] = (uint8_t)(POWER_CELL_OV_MV >> 8);
		if(BQ76907_WriteDataMemory(BQ76907_DM_COV_THRESHOLD, buf, 2) != 0)
			goto cfg_fail;
		v = BQ76907_COV_DELAY_CODE;
		if(BQ76907_WriteDataMemory(BQ76907_DM_COV_DELAY, &v, 1) != 0)
			goto cfg_fail;

		v = BQ76907_MaToOccCode(BQ76907_OCC_TRIP_MA);
		if(BQ76907_WriteDataMemory(BQ76907_DM_OCC_THRESHOLD, &v, 1) != 0)
			goto cfg_fail;
		v = BQ76907_OCC_DELAY_CODE;
		if(BQ76907_WriteDataMemory(BQ76907_DM_OCC_DELAY, &v, 1) != 0)
			goto cfg_fail;

		v = BQ76907_MaToOcdCode(BQ76907_OCD1_TRIP_MA);
		if(BQ76907_WriteDataMemory(BQ76907_DM_OCD1_THRESHOLD, &v, 1) != 0)
			goto cfg_fail;
		v = BQ76907_OCD1_DELAY_CODE;
		if(BQ76907_WriteDataMemory(BQ76907_DM_OCD1_DELAY, &v, 1) != 0)
			goto cfg_fail;

		v = BQ76907_MaToOcdCode(BQ76907_OCD2_TRIP_MA);
		if(BQ76907_WriteDataMemory(BQ76907_DM_OCD2_THRESHOLD, &v, 1) != 0)
			goto cfg_fail;
		v = BQ76907_OCD2_DELAY_CODE;
		if(BQ76907_WriteDataMemory(BQ76907_DM_OCD2_DELAY, &v, 1) != 0)
			goto cfg_fail;

		v = BQ76907_MaToScdCode(BQ76907_SCD_TRIP_MA);
		if(BQ76907_WriteDataMemory(BQ76907_DM_SCD_THRESHOLD, &v, 1) != 0)
			goto cfg_fail;
		v = BQ76907_SCD_DELAY_CODE;
		if(BQ76907_WriteDataMemory(BQ76907_DM_SCD_DELAY, &v, 1) != 0)
			goto cfg_fail;

		/* Power:Shutdown — cell/stack UV → auto SHUTDOWN (~10 s delay) */
		buf[0] = (uint8_t)(POWER_SHUTDOWN_CELL_MV & 0xFF);
		buf[1] = (uint8_t)(POWER_SHUTDOWN_CELL_MV >> 8);
		if(BQ76907_WriteDataMemory(BQ76907_DM_SHUTDOWN_CELL_V, buf, 2) != 0)
			goto cfg_fail;
		buf[0] = (uint8_t)(POWER_SHUTDOWN_STACK_MV & 0xFF);
		buf[1] = (uint8_t)(POWER_SHUTDOWN_STACK_MV >> 8);
		if(BQ76907_WriteDataMemory(BQ76907_DM_SHUTDOWN_STACK_V, buf, 2) != 0)
			goto cfg_fail;
		v = (uint8_t)POWER_SHUTDOWN_TEMP_C;
		if(BQ76907_WriteDataMemory(BQ76907_DM_SHUTDOWN_TEMP_C, &v, 1) != 0)
			goto cfg_fail;
		v = (uint8_t)POWER_AUTO_SHUTDOWN_MIN;
		if(BQ76907_WriteDataMemory(BQ76907_DM_AUTO_SHUTDOWN_MIN, &v, 1) != 0)
			goto cfg_fail;

		if(BQ76907_ExitCfgUpdate() != 0)
		{
			s_prot_cfg_err = 3;
			delay_ms(50);
			continue;
		}

		delay_ms(20);
		/* Block SLEEP so cell ADC / CUV keep running */
		BQ76907_SubCommand(BQ76907_CMD_SLEEP_DISABLE);
		(void)BQ76907_ReadDataMemory(BQ76907_DM_FET_OPTIONS, &s_fet_options_rb, 1);
		if(BQ76907_VerifyCuvCov() == 0)
		{
			s_prot_cfg_err = 0;
			return 0;
		}

		s_prot_cfg_err = 4;
		delay_ms(50);
		continue;

cfg_fail:
		s_prot_cfg_err = 2;
		BQ76907_SubCommand(BQ76907_CMD_EXIT_CFGUPDATE);
		delay_ms(50);
		(void)BQ76907_ForceFetsOn();
	}

	/* Last attempt: refresh verify; if CUV/COV already correct, treat as OK */
	if(BQ76907_VerifyCuvCov() == 0)
	{
		s_prot_cfg_err = 0;
		return 0;
	}
	if(s_prot_cfg_err == 0)
		s_prot_cfg_err = 4;
	return 1;
}

static uint8_t BQ76907_ReadThresholdI2(uint16_t dm_addr, uint16_t *mv)
{
	uint8_t buf[2];
	int16_t v;
	uint8_t attempt;

	if(mv == 0)
		return 1;

	for(attempt = 0; attempt < 3; attempt++)
	{
		if(BQ76907_ReadDataMemory(dm_addr, buf, 2) != 0)
			continue;

		v = (int16_t)((uint16_t)buf[0] | ((uint16_t)buf[1] << 8));
		if(v < BQ76907_CELL_PROT_MV_MIN || v > BQ76907_CELL_PROT_MV_MAX)
			continue;

		*mv = (uint16_t)v;
		return 0;
	}
	return 1;
}

static uint8_t BQ76907_WriteThresholdI2(uint16_t dm_addr, uint16_t mv)
{
	uint8_t buf[2];

	if(mv > BQ76907_CELL_PROT_MV_MAX)
		return 1;

	buf[0] = (uint8_t)(mv & 0xFF);
	buf[1] = (uint8_t)(mv >> 8);

	if(BQ76907_EnterCfgUpdate() != 0)
		return 1;

	if(BQ76907_WriteDataMemory(dm_addr, buf, 2) != 0)
	{
		BQ76907_ExitCfgUpdate();
		return 1;
	}

	return BQ76907_ExitCfgUpdate();
}

uint8_t BQ76907_ReadCuvThreshold_mV(uint16_t *mv)
{
	return BQ76907_ReadThresholdI2(BQ76907_DM_CUV_THRESHOLD, mv);
}

uint8_t BQ76907_ReadCovThreshold_mV(uint16_t *mv)
{
	return BQ76907_ReadThresholdI2(BQ76907_DM_COV_THRESHOLD, mv);
}

uint8_t BQ76907_WriteCuvThreshold_mV(uint16_t mv)
{
	return BQ76907_WriteThresholdI2(BQ76907_DM_CUV_THRESHOLD, mv);
}

uint8_t BQ76907_WriteCovThreshold_mV(uint16_t mv)
{
	return BQ76907_WriteThresholdI2(BQ76907_DM_COV_THRESHOLD, mv);
}

uint8_t BQ76907_WriteCuvCovThreshold_mV(uint16_t cuv_mv, uint16_t cov_mv)
{
	uint8_t buf[2];

	if(cuv_mv > BQ76907_CELL_PROT_MV_MAX || cov_mv > BQ76907_CELL_PROT_MV_MAX)
		return 1;

	if(BQ76907_EnterCfgUpdate() != 0)
		return 1;

	buf[0] = (uint8_t)(cuv_mv & 0xFF);
	buf[1] = (uint8_t)(cuv_mv >> 8);
	if(BQ76907_WriteDataMemory(BQ76907_DM_CUV_THRESHOLD, buf, 2) != 0)
	{
		BQ76907_ExitCfgUpdate();
		return 1;
	}

	buf[0] = (uint8_t)(cov_mv & 0xFF);
	buf[1] = (uint8_t)(cov_mv >> 8);
	if(BQ76907_WriteDataMemory(BQ76907_DM_COV_THRESHOLD, buf, 2) != 0)
	{
		BQ76907_ExitCfgUpdate();
		return 1;
	}

	return BQ76907_ExitCfgUpdate();
}

static uint8_t PowerProt_VoltValid(uint16_t cuv_mv, uint16_t cov_mv, uint16_t shutdown_cell_mv)
{
	if(cuv_mv < 1000U || cuv_mv > BQ76907_CELL_PROT_MV_MAX)
		return 0;
	if(cov_mv < 1000U || cov_mv > BQ76907_CELL_PROT_MV_MAX)
		return 0;
	if(shutdown_cell_mv > BQ76907_CELL_PROT_MV_MAX)
		return 0;
	/* shutdown must stay below CUV (0 disables that source) */
	if(shutdown_cell_mv != 0U && shutdown_cell_mv >= cuv_mv)
		return 0;
	if(cuv_mv >= cov_mv)
		return 0;
	return 1;
}

static uint8_t PowerProt_OcValid(uint16_t oc_chg_ma, uint16_t oc_dsg_ma)
{
	if(oc_chg_ma < 500U || oc_chg_ma > POWER_OC_MAX_PLAUSIBLE_MA)
		return 0;
	if(oc_dsg_ma < 500U || oc_dsg_ma > POWER_OC_MAX_PLAUSIBLE_MA)
		return 0;
	return 1;
}

static uint8_t PowerProt_ThrValid(uint16_t cuv_mv, uint16_t cov_mv, uint16_t shutdown_cell_mv,
				  uint16_t oc_chg_ma, uint16_t oc_dsg_ma)
{
	return (PowerProt_VoltValid(cuv_mv, cov_mv, shutdown_cell_mv)
		&& PowerProt_OcValid(oc_chg_ma, oc_dsg_ma)) ? 1 : 0;
}

static uint8_t PowerProt_SaveToFlash(uint16_t cuv_mv, uint16_t cov_mv, uint16_t shutdown_cell_mv,
				     uint16_t oc_chg_ma, uint16_t oc_dsg_ma)
{
	/* ESP: update Modbus mirrors; caller persists via NVS. */
	rmc_cuv = cuv_mv;
	rmc_cov = cov_mv;
	rmc_stack = shutdown_cell_mv;
	rmc_oc_chg = oc_chg_ma;
	rmc_oc_dsg = oc_dsg_ma;
	return 0;
}

void PowerProt_LoadFromFlash(void)
{
	uint16_t cuv = rmc_cuv;
	uint16_t cov = rmc_cov;
	uint16_t sd = rmc_stack;
	uint16_t oc_chg = rmc_oc_chg;
	uint16_t oc_dsg = rmc_oc_dsg;

	if(PowerProt_ThrValid(cuv, cov, sd, oc_chg, oc_dsg))
	{
		power_cell_uv_mv = cuv;
		power_cell_ov_mv = cov;
		power_shutdown_cell_mv = sd;
		power_oc_chg_ma = oc_chg;
		power_oc_dsg_ma = oc_dsg;
	}
	else
	{
		power_cell_uv_mv = POWER_CELL_UV_MV_DEFAULT;
		power_cell_ov_mv = POWER_CELL_OV_MV_DEFAULT;
		power_shutdown_cell_mv = POWER_SHUTDOWN_CELL_MV_DEFAULT;
		power_oc_chg_ma = POWER_OC_CHG_MA_DEFAULT;
		power_oc_dsg_ma = POWER_OC_DSG_MA_DEFAULT;
	}
	cuv_threshold_mv = power_cell_uv_mv;
	cov_threshold_mv = power_cell_ov_mv;
}

static uint8_t BQ76907_WriteProtThresholds(uint16_t cuv_mv, uint16_t cov_mv, uint16_t shutdown_cell_mv,
					   uint16_t oc_chg_ma, uint16_t oc_dsg_ma)
{
	uint8_t buf[2];
	uint8_t v;
	uint16_t stack_mv = (uint16_t)(shutdown_cell_mv * BQ76907_CELL_COUNT);

	if(BQ76907_EnterCfgUpdate() != 0)
		return 1;

	buf[0] = (uint8_t)(cuv_mv & 0xFF);
	buf[1] = (uint8_t)(cuv_mv >> 8);
	if(BQ76907_WriteDataMemory(BQ76907_DM_CUV_THRESHOLD, buf, 2) != 0)
		goto fail;

	buf[0] = (uint8_t)(cov_mv & 0xFF);
	buf[1] = (uint8_t)(cov_mv >> 8);
	if(BQ76907_WriteDataMemory(BQ76907_DM_COV_THRESHOLD, buf, 2) != 0)
		goto fail;

	buf[0] = (uint8_t)(shutdown_cell_mv & 0xFF);
	buf[1] = (uint8_t)(shutdown_cell_mv >> 8);
	if(BQ76907_WriteDataMemory(BQ76907_DM_SHUTDOWN_CELL_V, buf, 2) != 0)
		goto fail;

	buf[0] = (uint8_t)(stack_mv & 0xFF);
	buf[1] = (uint8_t)(stack_mv >> 8);
	if(BQ76907_WriteDataMemory(BQ76907_DM_SHUTDOWN_STACK_V, buf, 2) != 0)
		goto fail;

	v = BQ76907_MaToOccCode(oc_chg_ma);
	if(BQ76907_WriteDataMemory(BQ76907_DM_OCC_THRESHOLD, &v, 1) != 0)
		goto fail;

	v = BQ76907_MaToOcdCode(oc_dsg_ma);
	if(BQ76907_WriteDataMemory(BQ76907_DM_OCD1_THRESHOLD, &v, 1) != 0)
		goto fail;

	return BQ76907_ExitCfgUpdate();

fail:
	BQ76907_ExitCfgUpdate();
	return 1;
}

uint8_t PowerProt_SetThresholds(uint16_t cuv_mv, uint16_t cov_mv, uint16_t shutdown_cell_mv,
				uint16_t oc_chg_ma, uint16_t oc_dsg_ma)
{
	if(!PowerProt_ThrValid(cuv_mv, cov_mv, shutdown_cell_mv, oc_chg_ma, oc_dsg_ma))
		return 1;
	if(cuv_mv == power_cell_uv_mv
	   && cov_mv == power_cell_ov_mv
	   && shutdown_cell_mv == power_shutdown_cell_mv
	   && oc_chg_ma == power_oc_chg_ma
	   && oc_dsg_ma == power_oc_dsg_ma)
		return 0;
	power_cell_uv_mv = cuv_mv;
	power_cell_ov_mv = cov_mv;
	power_shutdown_cell_mv = shutdown_cell_mv;
	power_oc_chg_ma = oc_chg_ma;
	power_oc_dsg_ma = oc_dsg_ma;
	cuv_threshold_mv = cuv_mv;
	cov_threshold_mv = cov_mv;
	(void)PowerProt_SaveToFlash(cuv_mv, cov_mv, shutdown_cell_mv, oc_chg_ma, oc_dsg_ma);

	/* BootInit ConfigProtections will program DM; rewrite only after ready */
	if(s_pwr_ready)
	{
		return BQ76907_WriteProtThresholds(cuv_mv, cov_mv, shutdown_cell_mv,
						   oc_chg_ma, oc_dsg_ma);
	}
	return 0;
}

static uint16_t BQ76907_ReadStackMedian_mV(void)
{
	uint16_t samples[3];
	uint8_t count = 0;
	uint8_t i;
	uint8_t j;
	uint16_t tmp;

	for(i = 0; i < 3; i++)
	{
		if(BQ76907_I2C_ReadReg(BQ76907_REG_STACK_V, &samples[count]) != 0)
		{
			delay_ms(5);
			continue;
		}
		if(samples[count] == 0 || samples[count] == 0xFFFF || samples[count] > 35000)
		{
			delay_ms(5);
			continue;
		}
		count++;
		delay_ms(5);
	}

	if(count == 0)
		return 0;
	if(count == 1)
		return samples[0];

	for(i = 0; i < count - 1; i++)
	{
		for(j = i + 1; j < count; j++)
		{
			if(samples[j] < samples[i])
			{
				tmp = samples[i];
				samples[i] = samples[j];
				samples[j] = tmp;
			}
		}
	}

	return samples[count / 2];
}

static uint16_t BQ76907_FilterVoltage_x10(uint16_t sample_x10)
{
	static uint16_t filtered_x10 = 0;
	static uint8_t filter_ready = 0;
	int16_t delta;

	if(sample_x10 == 0)
	{
		filter_ready = 0;
		return 0;
	}

	if(!filter_ready)
	{
		filtered_x10 = sample_x10;
		filter_ready = 1;
		return filtered_x10;
	}

	delta = (int16_t)sample_x10 - (int16_t)filtered_x10;
	/* Reject only upward spikes; always track falling voltage (UV path) */
	if(delta > 15)
		return filtered_x10;
	if(delta < -15)
		return sample_x10;

	filtered_x10 = (uint16_t)(((uint32_t)filtered_x10 * 7U + sample_x10) / 8U);
	return filtered_x10;
}

uint8_t BQ76907_ReadStackVoltage_x10(uint16_t *voltage_x10)
{
	uint16_t stack_mv;
	uint32_t cell_sum;
	uint8_t cell_valid;
	uint8_t retry;
	uint16_t v_x10;
	uint16_t raw_mv;
	static uint32_t s_cell_sum_good;

	if(voltage_x10 == 0)
		return 1;

	for(retry = 0; retry < 3; retry++)
	{
		stack_mv = BQ76907_ReadStackMedian_mV();

		cell_sum = BQ76907_ReadCellSum_mV(&cell_valid);
		if(cell_valid >= BQ76907_CELL_COUNT)
			s_cell_sum_good = cell_sum;

		/* Both zero = I2C fail / no BMS — not a valid 0V pack reading */
		if(stack_mv == 0 && cell_sum == 0)
		{
			delay_ms(20);
			continue;
		}

		if(stack_mv >= 15000U && stack_mv <= 35000U)
			raw_mv = stack_mv;
		else if(cell_valid >= BQ76907_CELL_COUNT && cell_sum >= 15000U && cell_sum <= 35000U)
			raw_mv = (uint16_t)cell_sum;
		else if(cell_sum > 0U)
			raw_mv = (uint16_t)cell_sum;
		else if(stack_mv > 0U)
			raw_mv = stack_mv;
		else
		{
			delay_ms(20);
			continue;
		}

		v_x10 = (uint16_t)(raw_mv / 100U);

		if(v_x10 > 450)
		{
			delay_ms(20);
			continue;
		}

		v_x10 = BQ76907_FilterVoltage_x10(v_x10);
		*voltage_x10 = v_x10;
		return 0;
	}

	return 1;
}

uint8_t PowerLED_BarsFromVoltage(uint16_t voltage_x10)
{
	uint8_t level;

	if(voltage_x10 < POWER_VOLT_MIN_x10)
		return 0;
	if(voltage_x10 >= POWER_VOLT_FULL_x10)
		return POWER_LED_SOC_COUNT;

	level = (uint8_t)(1 + (voltage_x10 - POWER_VOLT_MIN_x10) / POWER_VOLT_STEP_x10);
	if(level > POWER_LED_SOC_COUNT)
		level = POWER_LED_SOC_COUNT;
	return level;
}

/* Software OC: charge ≥3 A / discharge ≥4 A; latch FETs off. */
static void PowerMgmt_CheckOvercurrent(void)
{
	int16_t current;
	int16_t abs_ma;
	uint16_t bat;
	uint8_t trip = 0;
	uint8_t fets_off = 0;
	static uint8_t s_oc_debounce;
	static uint8_t s_oc_recover_debounce;

	if(BQ76907_ReadCurrent_mA(&current) != 0)
		current = BQ76907_ReadCurrentFiltered_mA();
	else
		(void)BQ76907_ReadCurrentFiltered_mA();

	abs_ma = BQ76907_Abs16(current);

	/* Reject I2C/ADC glitches (e.g. 30 A) — do not trip or hold OC latch */
	if(abs_ma > (int16_t)POWER_OC_MAX_PLAUSIBLE_MA)
	{
		s_oc_debounce = 0;
		return;
	}

	if(BQ76907_ReadBatteryStatus(&bat) == 0)
		fets_off = ((bat & (BQ76907_BAT_CHG | BQ76907_BAT_DSG)) == 0) ? 1 : 0;

	/* FETs already off: do not keep rewriting CHG_OFF (blocks charge reopen) */
	if(fets_off && abs_ma < (int16_t)POWER_OC_CHG_MA)
	{
		s_oc_active = 0;
		s_oc_debounce = 0;
		s_oc_recover_debounce = 0;
		return;
	}

	if(current >= (int16_t)POWER_OC_CHG_MA)
		trip = 1;
	else if(current <= -(int16_t)POWER_OC_DSG_MA)
		trip = 1;

	if(trip)
	{
		s_oc_recover_debounce = 0;
		if(s_oc_debounce < 255)
			s_oc_debounce++;
		if(s_oc_debounce >= POWER_OC_DEBOUNCE)
		{
			if(!s_oc_active)
			{
				BQ76907_LogFetOff(
					(current >= (int16_t)POWER_OC_CHG_MA)
						? FET_OFF_SOFT_OC_CHG : FET_OFF_SOFT_OC_DSG,
					(uint16_t)abs_ma);
			}
			s_oc_active = 1;
			(void)BQ76907_DisableFets();
		}
		return;
	}

	s_oc_debounce = 0;

	if(!s_oc_active)
		return;

	if(abs_ma < (int16_t)POWER_OC_RECOVER_MA)
	{
		if(s_oc_recover_debounce < 255)
			s_oc_recover_debounce++;
		if(s_oc_recover_debounce >= POWER_OC_RECOVER_DEBOUNCE)
			s_oc_active = 0;
	}
	else
	{
		s_oc_recover_debounce = 0;
		(void)BQ76907_DisableFets();
	}
}

uint16_t BQ76907_GetCellMin_mV(void)
{
	return s_cell_min_mv;
}

/*
 * Charger plug-in gate: wait POWER_BOOT_WAIT_SEC after adapter appears
 * before opening CHG. BootInit may AllowNow (cold boot already waited).
 */
static void BQ76907_PlugChgHoldReset(void)
{
	s_plug_chg_hold = 0;
	s_plug_chg_deadline = 0;
	s_adapter_latched = 0;
	s_adapter_gone_cnt = 0;
	s_chg_low_i_cnt = 0;
}

static void BQ76907_PlugChgHoldAllowNow(void)
{
	s_plug_chg_hold = 2;
	s_plug_chg_deadline = 0;
	s_adapter_latched = 1;
	s_adapter_gone_cnt = 0;
	s_chg_low_i_cnt = 0;
}

/*
 * Adapter sense while CHG is off: CDRAW | CHGDET, or a short CHG probe
 * (some boards have weak/missing CDRAW — without probe we never open CHG).
 * Once latched / in charge session, stay true until clear unplug debounce.
 *
 * Important: with CHG off the pack still discharges (often -20 mA or more).
 * Do NOT treat negative current as "adapter gone" — that blocked re-plug
 * detection and aborted the POWER_BOOT_WAIT_SEC gate.
 */
static uint8_t BQ76907_AdapterPresent(void)
{
	uint16_t bat;
	uint8_t cdraw = 0;
	uint8_t sense = 0;
	int16_t ma;
	TickType_t now;

	if(BQ76907_ReadBatteryStatus(&bat) != 0)
		return s_adapter_latched;

	ma = BQ76907_ReadCurrentFiltered_mA();
	if(ma >= (int16_t)BQ76907_CHARGE_LED_MIN_MA)
	{
		s_adapter_latched = 1;
		s_adapter_gone_cnt = 0;
		return 1;
	}

	/* Waiting for plug delay: keep true the whole POWER_BOOT_WAIT_SEC */
	if(s_plug_chg_hold == 1)
	{
		s_adapter_latched = 1;
		s_adapter_gone_cnt = 0;
		return 1;
	}

	if((bat & BQ76907_BAT_CHG) == 0)
	{
		if(BQ76907_ReadChgDetRaw(&cdraw) == 0)
			sense = cdraw;
		if(bat & BQ76907_BAT_CHGDET)
			sense = 1;

		/* No CDRAW/CHGDET: brief CHG pulse to see if charger can push current */
		now = xTaskGetTickCount();
		if(!sense && s_pwr_ready && s_plug_chg_hold == 0
		   && (s_chg_probe_next == 0 || (int32_t)(now - s_chg_probe_next) >= 0))
		{
			s_chg_probe_next = now + pdMS_TO_TICKS(2000);
			BQ76907_EnsureFetEnable();
			(void)BQ76907_WriteFetCtrl(BQ76907_FET_CHG_DSG_ON);
			delay_ms(80);
			ma = BQ76907_ReadCurrentFiltered_mA();
			(void)BQ76907_WriteFetCtrl((uint8_t)(BQ76907_FET_CHG_OFF | BQ76907_FET_DSG_ON));
			if(ma >= (int16_t)BQ76907_CHARGE_LED_MIN_MA)
				sense = 1;
		}
	}
	else
	{
		/*
		 * CHG on: solid charge → present; else CDRAW/CHGDET.
		 * Negative / idle current alone is not enough to clear — debounce below.
		 */
		if(ma >= (int16_t)BQ76907_CHARGE_CURRENT_MIN_MA)
			sense = 1;
		else if(BQ76907_ReadChgDetRaw(&cdraw) == 0 && cdraw)
			sense = 1;
		else if(bat & BQ76907_BAT_CHGDET)
			sense = 1;
	}

	if(sense)
	{
		s_adapter_latched = 1;
		s_adapter_gone_cnt = 0;
		return 1;
	}

	/* Debounce clear — don't drop latch on one bad sample */
	if(s_adapter_latched || s_plug_chg_hold == 2)
	{
		if(s_adapter_gone_cnt < 255)
			s_adapter_gone_cnt++;
		if(s_adapter_gone_cnt < 15) /* ~3 s at 200 ms */
			return 1;
		s_adapter_latched = 0;
		s_adapter_gone_cnt = 0;
		return 0;
	}

	return 0;
}

/* 1 = may open CHG; 0 = still in plug delay (or no adapter). */
static uint8_t BQ76907_PlugChgAllowed(void)
{
	/* Active / waiting session: never Reset on sense flicker */
	if(s_plug_chg_hold == 2)
		return 1;

	if(s_plug_chg_hold == 1)
	{
		if((int32_t)(xTaskGetTickCount() - s_plug_chg_deadline) < 0)
			return 0;
		s_plug_chg_hold = 2;
		s_adapter_latched = 1;
		s_chg_low_i_cnt = 0;
		return 1;
	}

	/* hold == 0: need adapter to start delay */
	if(!BQ76907_AdapterPresent())
	{
		BQ76907_PlugChgHoldReset();
		return 0;
	}

	s_plug_chg_hold = 1;
	s_plug_chg_deadline = xTaskGetTickCount()
		+ pdMS_TO_TICKS((uint32_t)POWER_BOOT_WAIT_SEC * 1000U);
	return 0;
}

/* Keep pack rail up (DSG on) but block charge while plug delay runs. */
static uint8_t BQ76907_HoldChgOffDsgOn(void)
{
	if(!s_pwr_ready)
		return 1;
	BQ76907_EnsureFetEnable();
	return BQ76907_WriteFetCtrl((uint8_t)(BQ76907_FET_CHG_OFF | BQ76907_FET_DSG_ON));
}

static uint8_t BQ76907_CuvDsgOffChgOnGated(void)
{
	if(!s_cell_uv_active)
	{
		BQ76907_PlugChgHoldReset();
		return BQ76907_CuvDsgOffChgOn();
	}

	if(s_plug_chg_hold != 2 && !BQ76907_AdapterPresent())
	{
		BQ76907_PlugChgHoldReset();
		return BQ76907_DisableFets();
	}

	if(!BQ76907_PlugChgAllowed())
		return BQ76907_DisableFets();

	return BQ76907_CuvDsgOffChgOn();
}

/* Soft/HW CUV: DSG off, CHG on (charge recovery) — gated by plug delay. */
static void PowerMgmt_OnCuvFault(void)
{
	IWDG_ReloadCounter();
	s_cuv_cov_reset_done = 1;
	(void)BQ76907_CuvDsgOffChgOnGated();
}

/* COV: CHG-off only if charging; else both ON so MCU stays on battery */
static void PowerMgmt_OnCovFault(void)
{
	IWDG_ReloadCounter();
	s_cuv_cov_reset_done = 1;
	(void)BQ76907_CovApplyPolicy();
}

/* Trip from the same cell_mv[] the UI displays (CUV and COV). */
void PowerMgmt_TripCellUvFromCells(const uint16_t *cells_mv, uint8_t n)
{
	uint8_t i;
	uint8_t low = 0;
	uint8_t high = 0;
	uint8_t uv_recovered = 1;
	uint8_t ov_recovered = 1;
	uint8_t valid = 0;
	uint16_t min_mv = 0xFFFF;
	uint16_t max_mv = 0;
	static uint8_t s_uv_recover_debounce;
	static uint8_t s_ov_recover_debounce;

	if(cells_mv == 0 || n == 0)
		return;

	for(i = 0; i < n; i++)
	{
		if(cells_mv[i] == 0U)
			continue;
		valid++;
		if(cells_mv[i] < min_mv)
			min_mv = cells_mv[i];
		if(cells_mv[i] > max_mv)
			max_mv = cells_mv[i];
		if(cells_mv[i] < POWER_CELL_UV_MV)
			low = 1;
		if(cells_mv[i] > POWER_CELL_OV_MV)
			high = 1;
		if(cells_mv[i] < (uint16_t)(POWER_CELL_UV_MV + POWER_CELL_UV_RECOVER_DELTA_MV))
			uv_recovered = 0;
		/* Need OV - DELTA: 50 mV was inside post-CHG IR drop → recover → re-COV loop */
		if(cells_mv[i] > (uint16_t)(POWER_CELL_OV_MV - POWER_CELL_OV_RECOVER_DELTA_MV))
			ov_recovered = 0;
	}

	if(min_mv != 0xFFFF)
		s_cell_min_mv = min_mv;
	if(max_mv != 0)
		s_cell_max_mv = max_mv;

	if(valid == 0)
		return;

	if(low)
	{
		s_uv_recover_debounce = 0;
		if(!s_cell_uv_active)
		{
			BQ76907_LogFetOff(FET_OFF_SOFT_CUV, min_mv);
			s_cell_uv_active = 1;
			PowerMgmt_OnCuvFault(); /* first trip: open CHG path */
		}
		else
		{
			/* Still below UV — hold/open CHG only after plug delay */
			s_cell_uv_active = 1;
			(void)BQ76907_CuvDsgOffChgOnGated();
		}
		return;
	}

	if(high)
	{
		s_ov_recover_debounce = 0;
		if(!s_cell_ov_active)
			BQ76907_LogFetOff(FET_OFF_SOFT_COV, max_mv);
		s_cell_ov_active = 1;
		PowerMgmt_OnCovFault();
		return;
	}

	/* UV latch: keep DSG off / CHG on until cells recover */
	if(s_cell_uv_active)
	{
		if(!uv_recovered)
		{
			s_uv_recover_debounce = 0;
			(void)BQ76907_CuvDsgOffChgOnGated();
		}
		else
		{
			if(s_uv_recover_debounce < 255)
				s_uv_recover_debounce++;
			if(s_uv_recover_debounce >= POWER_CELL_UV_RECOVER_DEBOUNCE)
			{
				s_cell_uv_active = 0;
				s_uv_recover_debounce = 0;
				BQ76907_PlugChgHoldReset();
			}
			else
				(void)BQ76907_CuvDsgOffChgOnGated();
		}
	}

	if(s_cell_ov_active)
	{
		if(!ov_recovered)
		{
			s_ov_recover_debounce = 0;
			(void)BQ76907_CovApplyPolicy();
		}
		else if(BQ76907_IsExternalPowerPresent())
		{
			/*
			 * Charge complete: charger still in. IR drop after CHG-off looks
			 * "recovered", then reopening CHG re-trips (test[104] 2↔4).
			 * Hold CHG off until adapter is removed.
			 */
			s_ov_recover_debounce = 0;
			(void)BQ76907_CovApplyPolicy();
		}
		else
		{
			if(s_ov_recover_debounce < 255)
				s_ov_recover_debounce++;
			if(s_ov_recover_debounce >= POWER_CELL_OV_RECOVER_DEBOUNCE)
			{
				s_cell_ov_active = 0;
				s_ov_recover_debounce = 0;
			}
			else
				(void)BQ76907_CovApplyPolicy();
		}
	}

	if(!s_cell_uv_active && !s_cell_ov_active)
		s_cuv_cov_reset_done = 0;
}

/* UV/OV using cached cells; also honor chip Safety Status A CUV/COV. */
static void PowerMgmt_CheckCellUv(void)
{
	uint8_t cell_valid = 0;
	uint8_t safety = 0;
	uint16_t cells[BQ76907_CELL_COUNT];
	uint16_t bat;

	if(BQ76907_ReadBatteryStatus(&bat) == 0)
	{
		if(bat & BQ76907_BAT_SLEEP)
			BQ76907_SubCommand(BQ76907_CMD_SLEEP_DISABLE);
	}

	if(BQ76907_GetCellVoltages_mV(cells, BQ76907_CELL_COUNT, &cell_valid) == 0)
		PowerMgmt_TripCellUvFromCells(cells, BQ76907_CELL_COUNT);

	if(BQ76907_ReadSafetyStatusA(&safety) == 0)
	{
		if(safety & BQ76907_SAFETY_A_CUV)
		{
			if(!s_cell_uv_active)
			{
				BQ76907_LogFetOff(FET_OFF_CHIP_CUV, safety);
				s_cell_uv_active = 1;
				PowerMgmt_OnCuvFault(); /* open CHG once */
			}
			else
				s_cell_uv_active = 1; /* stay latched; maintain below */
		}
		if(safety & BQ76907_SAFETY_A_COV)
		{
			if(!s_cell_ov_active)
				BQ76907_LogFetOff(FET_OFF_CHIP_COV, safety);
			s_cell_ov_active = 1;
			PowerMgmt_OnCovFault();
		}
	}

	/* Keep recover paths (do not force both-OFF). Light if CHG already on. */
	if(s_cell_uv_active)
		(void)BQ76907_CuvDsgOffChgOnGated();
	if(s_cell_ov_active)
		(void)BQ76907_CovApplyPolicy();
}

uint8_t PowerMgmt_AutoControl(uint16_t *voltage_x10, uint8_t *bars)
{
	uint16_t voltage = 0;
	uint16_t alarm = 0;
	uint8_t level;
	uint8_t volt_ok;

	IWDG_ReloadCounter();
	s_ext_cache_valid = 0; /* one external-power sense per AutoControl cycle */
	volt_ok = (BQ76907_ReadStackVoltage_x10(&voltage) == 0) ? 1 : 0;

	/* Diagnostics each cycle */
	if(BQ76907_I2C_ReadReg(0x04, &alarm) == 0) /* Alert B | Status B */
		s_safety_b_rb = (uint8_t)(alarm >> 8);
	if(BQ76907_I2C_ReadReg(0x62, &alarm) == 0)
		s_alarm_status_rb = alarm;
	(void)BQ76907_ReadFetCtrl(&s_fet_ctrl_rb);

	/* Before BootInit CFG: only publish voltage — host FET OFF is ignored until HOST_FETOFF */
	if(!s_pwr_ready)
	{
		if(!volt_ok)
			return 1;
		level = PowerLED_BarsFromVoltage(voltage);
		if(voltage_x10 != 0)
			*voltage_x10 = voltage;
		if(bars != 0)
			*bars = level;
		return 0;
	}

	/* UV/OC must run even when pack voltage read fails */
	PowerMgmt_CheckCellUv();
	PowerMgmt_CheckOvercurrent();

	/* Pack 0.1V backup: 7 * 2550 mV = 17.85 V → x10 < 178 */
	if(volt_ok && voltage > 0U && voltage < (uint16_t)((POWER_CELL_UV_MV * BQ76907_CELL_COUNT) / 100U))
	{
		if(!s_cell_uv_active)
		{
			BQ76907_LogFetOff(FET_OFF_PACK_UV, voltage);
			s_cell_uv_active = 1;
			PowerMgmt_OnCuvFault();
		}
		else
		{
			s_cell_uv_active = 1;
			(void)BQ76907_CuvDsgOffChgOnGated(); /* charging up: after plug delay */
		}
	}

	/* Auto: ext power (CHGDET) → charge; else battery; CUV keeps CHG */
	(void)PowerMgmt_SelectPowerPath();

	/* Any FET/DSG turn-off edge → refresh sticky reason (not limited to UV voltage) */
	BQ76907_WatchFetOffEdge();

	IWDG_ReloadCounter();

	if(!volt_ok)
		return 1;

	level = PowerLED_BarsFromVoltage(voltage);

	if(voltage_x10 != 0)
		*voltage_x10 = voltage;
	if(bars != 0)
		*bars = level;

	return 0;
}

void BMS_HwReset_Init(void)
{
	gpio_config_t io = {
		.pin_bit_mask = (1ULL << BMS_HW_RESET_GPIO),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE,
	};
	gpio_config(&io);
	gpio_set_level(BMS_HW_RESET_GPIO, 0); /* hold low = run */

	BQ76907_FetOffLog_Restore();
}

void BMS_HwReset_Pulse(void)
{
	gpio_set_level(BMS_HW_RESET_GPIO, 1);
	delay_ms(BMS_HW_RESET_PULSE_MS);
	gpio_set_level(BMS_HW_RESET_GPIO, 0);
	delay_ms(BMS_HW_RESET_SETTLE_MS);
}

uint8_t PowerMgmt_BootInit(void)
{
	uint8_t i;
	uint16_t voltage_x10;

	IWDG_ReloadCounter();
	/* Restore CUV/COV/shutdown from Flash before CFG write */
	PowerProt_LoadFromFlash();
	cuv_threshold_mv = power_cell_uv_mv;
	cov_threshold_mv = power_cell_ov_mv;

	/* One HW reset of BMS board (PB0 high), then bring-up over I2C */
	BMS_HwReset_Pulse();
	IWDG_ReloadCounter();

	s_pwr_ready = 0;
	s_shutdown_abort_done = 0;
	if(BQ76907_Init() != 0)
		return 1;

	BQ76907_SubCommand(BQ76907_CMD_SLEEP_DISABLE);
	/* Abort any pending SHUTDOWN / soft-shutdown from prior UV */
	BQ76907_SubCommand(BQ76907_CMD_EXIT_DEEPSLEEP);
	delay_ms(50);
	IWDG_ReloadCounter();

	(void)BQ76907_EnsureFullAccess();

	if(BQ76907_ConfigProtections() != 0)
	{
		/* test[91..93] show enable/verify/err — still allow FET tries */
	}

	/* HOST_FETON/OFF now valid — enable host FET path */
	s_pwr_ready = 1;

	BQ76907_SubCommand(BQ76907_CMD_SLEEP_DISABLE);
	(void)BQ76907_ProtRecoveryAll();
	BQ76907_ClearAlarmStatus();
	(void)BQ76907_RefreshProtVerify();
	IWDG_ReloadCounter();

	delay_ms(POWER_BOOT_DELAY_MS);

	/* UV → charge-recover (CHG on); OV → discharge-recover; else both ON */
	{
		uint16_t cells[BQ76907_CELL_COUNT];
		uint8_t cell_valid = 0;

		s_cell_uv_active = 0;
		s_cell_ov_active = 0;
		if(BQ76907_GetCellVoltages_mV(cells, BQ76907_CELL_COUNT, &cell_valid) == 0
		   && cell_valid > 0)
		{
			for(i = 0; i < BQ76907_CELL_COUNT; i++)
			{
				if(cells[i] == 0U)
					continue;
				if(cells[i] < POWER_CELL_UV_MV)
					s_cell_uv_active = 1;
				if(cells[i] > POWER_CELL_OV_MV)
					s_cell_ov_active = 1;
			}
		}

		/*
		 * Cold boot already waited POWER_BOOT_WAIT_SEC.
		 * If adapter already present → allow CHG; else keep CHG off on battery.
		 */
		s_ext_cache_valid = 0;
		if(BQ76907_AdapterPresent())
			BQ76907_PlugChgHoldAllowNow();
		else
			BQ76907_PlugChgHoldReset();

		for(i = 0; i < 5; i++)
		{
			IWDG_ReloadCounter();
			if(s_cell_uv_active)
			{
				if(BQ76907_CuvDsgOffChgOnGated() == 0)
					break;
			}
			else if(s_cell_ov_active)
			{
				if(BQ76907_CovApplyPolicy() == 0)
					break;
			}
			else if(BQ76907_AdapterPresent())
			{
				if(BQ76907_ForceFetsOn() == 0)
					break;
			}
			else if(BQ76907_HoldChgOffDsgOn() == 0)
				break;
			delay_ms(50);
		}
		if(PowerMgmt_SelectPowerPath() != 0)
		{
			if(s_cell_uv_active)
				(void)BQ76907_CuvDsgOffChgOnGated();
			else if(BQ76907_AdapterPresent())
				(void)BQ76907_ForceFetsOn();
			else
				(void)BQ76907_HoldChgOffDsgOn();
		}
	}

	delay_ms(300);

	for(i = 0; i < POWER_BOOT_RETRY; i++)
	{
		IWDG_ReloadCounter();
		if(PowerMgmt_AutoControl(&voltage_x10, 0) == 0)
			return 0;
		delay_ms(50);
	}

	return 1;
}
