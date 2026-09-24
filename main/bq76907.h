#ifndef __BQ76907_H
#define __BQ76907_H

#include <stdint.h>

/* BQ76907 I2C 7-bit address = 0x08, GPIO14=SCL GPIO4=SDA, open-drain */
#define BQ76907_I2C_ADDR_7BIT    0x08

/* BQ769x2 direct command registers (16-bit, little-endian on bus) */
#define BQ76907_REG_CTRL_STATUS  0x0000
#define BQ76907_REG_SAFETY_STATUS_A 0x03  /* COV CUV SCD OCD1 OCD2 OCC ... */
#define BQ76907_REG_BAT_STATUS   0x0012
#define BQ76907_SAFETY_A_CUV     (1U << 6)
#define BQ76907_SAFETY_A_COV     (1U << 7)
#define BQ76907_REG_CELL1_V      0x0014  /* Cell 1 Voltage, unit: 1 mV per LSB */
#define BQ76907_REG_CELL7_V      0x0020  /* Cell 7 Voltage, unit: 1 mV per LSB */
#define BQ76907_REG_REG18_V      0x0022  /* REG18 Voltage, 16-bit ADC codes */
#define BQ76907_REG_VSS_V        0x0024  /* VSS Voltage, 16-bit ADC codes */
#define BQ76907_REG_STACK_V      0x0026  /* Stack Voltage, unit: 1 mV per LSB (TRM) */
#define BQ76907_REG_INT_TEMP     0x0028  /* Int Temperature, unit: 1 °C */
#define BQ76907_REG_TS_MEAS      0x002A  /* TS Measurement, 16-bit ADC codes */
#define BQ76907_REG_RAW_CURRENT  0x0036  /* Raw Current, 24-bit sign-ext to 32-bit */
#define BQ76907_REG_CURRENT      0x003A  /* Current (CC2), unit: userA */
#define BQ76907_REG_CC1_CURRENT  0x003C  /* CC1 Current, unit: userA */

#define BQ76907_CELL_COUNT       7   /* chip cell voltage registers */

/* Subcommands written to 0x003E (ref: BQ76907 TRM SLUUCJ0) */
#define BQ76907_CMD_PASSQ           0x0004  /* Accumulated charge + time */
#define BQ76907_CMD_EXIT_DEEPSLEEP  0x000E
#define BQ76907_CMD_SHUTDOWN        0x0010  /* must send twice within 4 s */
#define BQ76907_CMD_RESET           0x0012
#define BQ76907_CMD_FET_ENABLE      0x0022  /* toggles Battery Status[FET_EN] */
#define BQ76907_CMD_SET_CFGUPDATE    0x0090
#define BQ76907_CMD_EXIT_CFGUPDATE   0x0092
#define BQ76907_CMD_SLEEP_ENABLE    0x0099
#define BQ76907_CMD_SLEEP_DISABLE   0x009A
#define BQ76907_CMD_PROT_RECOVERY   0x009B  /* recover CUV/COV/OC/temp/… */
#define BQ76907_CMD_SEAL            0x0030

/* Unseal: write keys to 0x3E/0x3F (TRM default Full Access Key Step 1/2) */
#define BQ76907_KEY_FULLACCESS_1    0x0414
#define BQ76907_KEY_FULLACCESS_2    0x3672

#define BQ76907_BAT_SEC_SHIFT       10
#define BQ76907_BAT_SEC_MASK        (3U << BQ76907_BAT_SEC_SHIFT)
#define BQ76907_BAT_SEC_FULLACCESS  (1U << BQ76907_BAT_SEC_SHIFT) /* SEC1:0 = 01 */
#define BQ76907_BAT_SEC_SEALED      (3U << BQ76907_BAT_SEC_SHIFT) /* SEC1:0 = 11 */

#define BQ76907_REG_SUBCMD       0x003E
#define BQ76907_REG_SUBCMD_BUF   0x0040
#define BQ76907_REG_SUBCMD_CKSUM 0x0060
#define BQ76907_REG_SUBCMD_LEN   0x0061

/* Data memory: Protections:Cell Voltage (TRM 12.6) */
#define BQ76907_DM_CUV_THRESHOLD 0x902E  /* I2, mV, 0..5500, default 2500 */
#define BQ76907_DM_CUV_DELAY     0x9030  /* U1, ADSCAN intervals, default 10 */
#define BQ76907_DM_COV_THRESHOLD 0x9032  /* I2, mV, 0..5500, default 4200 */
#define BQ76907_DM_COV_DELAY     0x9034  /* U1, ADSCAN intervals, default 10 */
#define BQ76907_CELL_PROT_MV_MIN 0
#define BQ76907_CELL_PROT_MV_MAX 5500

/* Data memory: Settings:Protection + Protections:Current */
#define BQ76907_DM_ENABLED_PROT_A    0x9024  /* H1: COV CUV SCD OCD1 OCD2 OCC ... */
#define BQ76907_DM_DSG_FET_PROT_A    0x9026  /* which faults turn DSG off */
#define BQ76907_DM_CHG_FET_PROT_A    0x9027  /* which faults turn CHG off */
#define BQ76907_DM_OCC_THRESHOLD     0x9036  /* U1, code: V=(2*code-1) mV */
#define BQ76907_DM_OCC_DELAY         0x9037
#define BQ76907_DM_OCD1_THRESHOLD    0x9038  /* U1, code: V=2*code mV, 2..100 */
#define BQ76907_DM_OCD1_DELAY        0x9039
#define BQ76907_DM_OCD2_THRESHOLD    0x903A
#define BQ76907_DM_OCD2_DELAY        0x903B
#define BQ76907_DM_SCD_THRESHOLD     0x903C  /* H1 index 0..15 → 10..500 mV */
#define BQ76907_DM_SCD_DELAY         0x903D

/* Data memory: Power:Shutdown (TRM 12.4.2) — auto SHUTDOWN after ~10 s */
#define BQ76907_DM_SHUTDOWN_CELL_V   0x9053  /* I2, mV; 0=disabled */
#define BQ76907_DM_SHUTDOWN_STACK_V  0x9055  /* U2, mV; 0=disabled */
#define BQ76907_DM_SHUTDOWN_TEMP_C   0x9057  /* U1, °C; 0=disabled */
#define BQ76907_DM_AUTO_SHUTDOWN_MIN 0x9058  /* U1, minutes; 0=disabled */

#define BQ76907_PROT_A_COV           (1U << 7)
#define BQ76907_PROT_A_CUV           (1U << 6)
#define BQ76907_PROT_A_SCD           (1U << 5)
#define BQ76907_PROT_A_OCD1          (1U << 4)
#define BQ76907_PROT_A_OCD2          (1U << 3)
#define BQ76907_PROT_A_OCC           (1U << 2)
#define BQ76907_PROT_A_REGOUT        (1U << 0)

/* Sense resistor and trip currents → comparator mV thresholds.
 * OCD min ~4 mV → ~4 A @ 1 mOhm. OCC min ~3 mV → ~3 A @ 1 mOhm. */
#define BQ76907_RSENSE_UOHM          1000   /* 1.000 mOhm */
#define BQ76907_OCD2_TRIP_MA         6000   /* discharge OC level 2: 6 A */
#define BQ76907_SCD_TRIP_MA          15000  /* short-circuit discharge ~15 A */
#define BQ76907_OCD1_DELAY_CODE      20     /* longer debounce for mild OC */
#define BQ76907_OCD2_DELAY_CODE      6
#define BQ76907_OCC_DELAY_CODE       58     /* keep near OTP default */
#define BQ76907_SCD_DELAY_CODE       2      /* ~15-30 us */

/* Software OC defaults — runtime copies in power_oc_* (Flash + I2C) */
#define POWER_OC_CHG_MA_DEFAULT      3000   /* charge ≥ 3 A */
#define POWER_OC_DSG_MA_DEFAULT      4000   /* discharge ≥ 4 A */
extern uint16_t power_oc_chg_ma;
extern uint16_t power_oc_dsg_ma;
#define POWER_OC_CHG_MA              power_oc_chg_ma
#define POWER_OC_DSG_MA              power_oc_dsg_ma
/* HW OCC/OCD1 trip currents track the same runtime thresholds */
#define BQ76907_OCC_TRIP_MA          power_oc_chg_ma
#define BQ76907_OCD1_TRIP_MA         power_oc_dsg_ma
#define POWER_OC_RECOVER_MA          1500   /* clear OC latch only below this */
#define POWER_OC_MAX_PLAUSIBLE_MA    15000  /* |I| above this = glitch, ignore for soft OC */
#define POWER_OC_DEBOUNCE            3
#define POWER_OC_RECOVER_DEBOUNCE    6      /* stay off until current quiet */

#define BQ76907_REG_FET_CTRL        0x0068  /* FET CONTROL: ON/OFF bits */
#define BQ76907_FET_DSG_ON          0x01
#define BQ76907_FET_CHG_ON          0x02
#define BQ76907_FET_CHG_DSG_ON      0x03
#define BQ76907_FET_DSG_OFF         0x04
#define BQ76907_FET_CHG_OFF         0x08
#define BQ76907_FET_CHG_DSG_OFF     0x0C

#define BQ76907_DM_POWER_CONFIG     0x9014  /* Settings:Configuration:Power Config */
#define BQ76907_PWRCFG_SLEEP_EN        (1U << 0)
#define BQ76907_DM_VCELL_MODE       0x901B  /* 0/1/7 = all 7 cells used */
#define BQ76907_VCELL_MODE_7S          7U

#define BQ76907_DM_FET_OPTIONS      0x901E  /* Settings:Configuration:FET Options */
#define BQ76907_FETOPT_FET_EN          (1U << 2)  /* autonomous FET default after CFG exit */
#define BQ76907_FETOPT_SFET            (1U << 3)  /* 1=series body-diode protect */
#define BQ76907_FETOPT_SLEEPCHG        (1U << 4)  /* keep CHG on in SLEEP — disable */
#define BQ76907_FETOPT_HOST_FETON_EN   (1U << 5)
#define BQ76907_FETOPT_HOST_FETOFF_EN  (1U << 6)
#define BQ76907_FETOPT_CHGDETEN        (1U << 7)  /* enable CHG detector → Battery Status CHGDETFLAG */

#define BQ76907_CUV_DELAY_CODE         2     /* short delay so HW CUV reacts faster */
#define BQ76907_COV_DELAY_CODE         2

#define BQ76907_BAT_FET_EN          (1U << 8)
#define BQ76907_BAT_SLEEP           (1U << 15)
#define BQ76907_BAT_SS              (1U << 12) /* enabled safety fault present */
#define BQ76907_BAT_SA              (1U << 13) /* enabled safety alert present */
#define BQ76907_BAT_CFGUPDATE         (1U << 5)
#define BQ76907_BAT_DSG             (1U << 2)  /* DSG driver on */
#define BQ76907_BAT_CHG             (1U << 3)  /* CHG driver on */
#define BQ76907_BAT_CHGDET          (1U << 1)  /* CHGDETFLAG; debounced ≥100 ms */

#define BQ76907_REG_ALARM_RAW       0x0064  /* Alarm Raw Status */
#define BQ76907_ALARM_CDRAW         (1U << 1)  /* instant CHG detector (not debounced) */

/* TRM: charge = SRP above SRN = positive mA. Flip raw if PCB sense wiring is reversed.
 * OTP gain assumes 1 mOhm; if Rsense differs, scale reported userA → mA here. */
#define BQ76907_CURRENT_SIGN        1
#define BQ76907_CURRENT_SCALE_NUM   1   /* mA = raw_userA * NUM / DEN */
#define BQ76907_CURRENT_SCALE_DEN   1
#define BQ76907_CHARGE_CURRENT_MIN_MA   10
#define BQ76907_CURRENT_MAX_MA          30000 /* reject only near int16 glitch */
#define BQ76907_CHARGE_ON_CYCLES        6   /* need sustained charge before LED blink */
#define BQ76907_CHARGE_OFF_CYCLES       3
#define BQ76907_CHARGE_LED_MIN_MA       50  /* SOC top-LED blink threshold (mA) */
/* Slow charge ±I: probe CDRAW after I<=0 quiet; CDRAW distinguishes adapter vs battery */
#define BQ76907_EXT_PWR_PROBE_CYCLES       12  /* sense cycles with I<=0 before probe */
#define BQ76907_EXT_PWR_PROBE_COOLDOWN     8
#define BQ76907_EXT_PWR_PROBE_SETTLE_MS    20
#define BQ76907_EXT_PWR_CDRAW_LOW_NEED     2   /* consecutive CDRAW=0 to clear green */

#define BQ76907_CELL_MV_MIN_PARSE    1    /* keep very-low cells (0 mV reserved = invalid) */

#define POWER_VOLT_SHUTDOWN_x10  175   /* 17.5V, cut external power */
#define POWER_VOLT_MIN_x10       175   /* 17.5V, 25% / 1 of 4 SOC LEDs */
#define POWER_VOLT_STEP_x10      18    /* ~1.8V per 25% step (4 LEDs → 24.5V) */
#define POWER_VOLT_FULL_x10      245   /* 24.5V, 100% / 4 SOC LEDs */
#define POWER_LED_SOC_COUNT      4     /* LEDs 3..6: 25/50/75/100% */
/* Defaults — runtime copies in power_cell_* / power_shutdown_* (Flash + I2C) */
#define POWER_CELL_UV_MV_DEFAULT         2550  /* CUV + software: any cell below → FET off */
/* Clear soft UV latch only after all cells >= UV + this (hysteresis while charging up) */
#define POWER_CELL_UV_RECOVER_DELTA_MV   300
#define POWER_CELL_UV_RECOVER_DEBOUNCE   20    /* AutoControl cycles above recover before clear */
#define POWER_CELL_OV_MV_DEFAULT         3600  /* COV + software: any cell above → FET off */
/* Clear soft OV only after all cells <= OV - this (IR drop after CHG-off is ~50–100 mV) */
#define POWER_CELL_OV_RECOVER_DELTA_MV   150
#define POWER_CELL_OV_RECOVER_DEBOUNCE   20    /* cycles below recover before clear (charger unplugged) */
#define POWER_SHUTDOWN_CELL_MV_DEFAULT   2500  /* last-resort; must stay below CUV */
/* Auto SHUTDOWN (chip): must be BELOW CUV, else CUV starts 10 s irreversible
 * SHUTDOWN sequence → FETs stuck off (soft-shutdown) while cells recover.
 * 0 in any of these disables that source. */
extern uint16_t power_cell_uv_mv;
extern uint16_t power_cell_ov_mv;
extern uint16_t power_shutdown_cell_mv;
#define POWER_CELL_UV_MV         power_cell_uv_mv
#define POWER_CELL_OV_MV         power_cell_ov_mv
#define POWER_SHUTDOWN_CELL_MV   power_shutdown_cell_mv
#define POWER_SHUTDOWN_STACK_MV  ((uint16_t)(power_shutdown_cell_mv * BQ76907_CELL_COUNT))
#define POWER_SHUTDOWN_TEMP_C    0     /* 0=temp auto-shutdown off */
#define POWER_AUTO_SHUTDOWN_MIN  5     /* re-SHUTDOWN if wake w/o comm/current */
#define POWER_BOOT_RETRY         10
#define POWER_BOOT_DELAY_MS      200
#define POWER_BOOT_WAIT_SEC      10    /* wait after power-up / any charger plug before CHG */
#define BMS_HW_RESET_PULSE_MS    20   /* GPIO32 high pulse width (active-high reset) */
#define BMS_HW_RESET_SETTLE_MS   100   /* wait after release before I2C */

/* Modbus test[] for power/BQ76907; must not overlap AI_TEST_BASE..AI_TEST_LAST (30..61) */
#define PWR_TEST_VOLTAGE_X10       16   /* pack voltage, 0.1 V */
#define PWR_TEST_BARS              17   /* SOC LED bars 0..4 (25% steps) */
#define PWR_TEST_BAT_STATUS        18   /* BQ76907 reg 0x12 */
#define PWR_TEST_CURRENT_MA        23   /* filtered mA, charge = positive */
#define PWR_TEST_CHARGING          24   /* 0/1 charging LED flag */
#define PWR_TEST_CURRENT_RAW       25   /* CC1 raw from chip before sign fix */
#define PWR_TEST_CELL1_MV          63   /* cell1..cell7 mV, contiguous */
#define PWR_TEST_CELL2_MV          64
#define PWR_TEST_CELL3_MV          65
#define PWR_TEST_CELL4_MV          66
#define PWR_TEST_CELL5_MV          67
#define PWR_TEST_CELL6_MV          68
#define PWR_TEST_CELL7_MV          69
#define PWR_TEST_CELL_VALID_COUNT  70   /* valid cell count from last read */
#define PWR_TEST_CELL_SUM_MV       71   /* sum of valid cells, mV */
#define PWR_TEST_REG18_RAW         72   /* REG18 Voltage, ADC codes */
#define PWR_TEST_VSS_RAW           73   /* VSS Voltage, ADC codes */
#define PWR_TEST_STACK_MV          74   /* Stack Voltage, mV */
#define PWR_TEST_INT_TEMP_C        75   /* Int Temperature, °C */
#define PWR_TEST_TS_RAW            76   /* TS Measurement, ADC codes */
#define PWR_TEST_RAW_CURRENT_LO    77   /* Raw Current low 16 bits */
#define PWR_TEST_RAW_CURRENT_HI    78   /* Raw Current high 16 bits */
#define PWR_TEST_CURRENT_USERA     79   /* 0x3A Current, userA */
#define PWR_TEST_CC1_USERA         80   /* 0x3C CC1 Current, userA */
#define PWR_TEST_PASSQ_LSB_LO      81   /* PASSQLSB low 16 */
#define PWR_TEST_PASSQ_LSB_HI      82   /* PASSQLSB high 16 */
#define PWR_TEST_PASSQ_MSB_LO      83   /* PASSQMSB low 16 */
#define PWR_TEST_PASSQ_MSB_HI      84   /* PASSQMSB high 16 */
#define PWR_TEST_PASSTIME_LO       85   /* PASSTIME low 16, unit 250 ms */
#define PWR_TEST_PASSTIME_HI       86   /* PASSTIME high 16 */
#define PWR_TEST_CUV_THR_MV        87   /* CUV threshold mV; Modbus R/W */
#define PWR_TEST_COV_THR_MV        88   /* COV threshold mV; Modbus R/W */
#define PWR_TEST_PROT_APPLY        89   /* write 1 = apply test[87]/88] to chip */
#define PWR_TEST_CMD               90   /* 1=SHUTDOWN 2=RESET 3=FET_ENABLE 4=FET_RECOVER; 0=ok 0xFF=fail */
#define PWR_TEST_PROT_ENABLED_A    91   /* Enabled Protections A readback (expect 0xFD) */
#define PWR_TEST_PROT_VERIFY       92   /* bit0..3 CUV/COV en+thr; 0x0F=ok; +0x40 CUV thr rd fail; +0x20 COV thr rd fail; 0x80=DM read fail */
#define PWR_TEST_PROT_CFG_ERR      93   /* 0=ok 1=enter CFG 2=write 3=exit CFG 4=verify */
#define PWR_TEST_SAFETY_STATUS_A   94   /* chip Safety Status A (bit6=CUV bit7=COV) */
#define PWR_TEST_SAFETY_ALERT_A    95   /* chip Safety Alert A */
#define PWR_TEST_RESET_CAUSE       96   /* bit0 PIN 1 POR 2 SFT 3 IWDG 4 WWDG 5 LPWR; hi=CSR[31:24] */
#define PWR_TEST_UV_LATCH          97   /* 1=soft CUV/COV/OC latch active */
#define PWR_TEST_CELL_MIN_MV       98   /* minimum valid cell mV (debug UV) */
#define PWR_TEST_EXT_POWER         99   /* 1=ext/charger present (sticky); 0=battery */
#define PWR_TEST_FET_CTRL          100  /* reg 0x68 FET CONTROL readback */
#define PWR_TEST_FET_OPTIONS       101  /* DM 0x901E FET Options readback */
#define PWR_TEST_SAFETY_STATUS_B   102  /* reg 0x05 Safety Status B */
#define PWR_TEST_ALARM_STATUS      103  /* reg 0x62 Alarm Status */
#define PWR_TEST_FET_OFF_REASON    104  /* sticky: last FET-off cause (see FET_OFF_*) */
#define PWR_TEST_FET_OFF_DETAIL    105  /* sticky: context for reason (mV / mA / SafetyA) */
#define PWR_TEST_FET_OFF_HB_V      106  /* last heartbeat pack V x10 (Flash; survives death) */
#define PWR_TEST_FET_OFF_HB_CELL   107  /* last heartbeat min cell mV */

#define PWR_CMD_SHUTDOWN           1
#define PWR_CMD_RESET              2
#define PWR_CMD_FET_ENABLE         3
#define PWR_CMD_FET_RECOVER        4   /* clear soft CUV/COV/OC latch + both FETs ON */

/* Last FET-off reason codes → test[PWR_TEST_FET_OFF_REASON] */
#define FET_OFF_NONE               0
#define FET_OFF_SOFT_CUV           1   /* any cell < POWER_CELL_UV_MV; detail=min mV */
#define FET_OFF_SOFT_COV           2   /* any cell > POWER_CELL_OV_MV; detail=max mV */
#define FET_OFF_CHIP_CUV           3   /* Safety Status A CUV; detail=SafetyA */
#define FET_OFF_CHIP_COV           4   /* Safety Status A COV; detail=SafetyA */
#define FET_OFF_SOFT_OC_CHG        5   /* software charge OC; detail=|mA| */
#define FET_OFF_SOFT_OC_DSG        6   /* software discharge OC; detail=|mA| */
#define FET_OFF_PACK_UV            7   /* pack V backup UV; detail=voltage_x10 */
#define FET_OFF_CHIP_OC            8   /* SafetyA SCD/OCD1/OCD2/OCC; detail=SafetyA */
#define FET_OFF_CHIP_OTHER         9   /* FETs off, no soft latch; detail=SafetyA */
#define FET_OFF_HOST_CMD           10  /* host RESET/SHUTDOWN path; detail=cmd */
#define FET_OFF_POWER_LOSS         11  /* no host log; inferred after reboot (detail=SafetyA/0) */

/* Yellow-box monitor snapshot (same roles as voltage_x10 / cell_mv) */
typedef struct {
	uint16_t reg18_raw;          /* 0x22 REG18 Voltage, ADC codes */
	uint16_t vss_raw;            /* 0x24 VSS Voltage, ADC codes */
	uint16_t stack_mv;           /* 0x26 Stack Voltage, mV */
	int16_t  int_temp_c;         /* 0x28 Int Temperature, °C */
	uint16_t ts_raw;             /* 0x2A TS Measurement, ADC codes */
	int32_t  raw_current;        /* 0x36 Raw Current, 24-bit sign-ext */
	int16_t  current_usera;      /* 0x3A Current, userA */
	int16_t  cc1_current_usera;  /* 0x3C CC1 Current, userA */
	uint32_t passq_lsb;          /* PASSQ offset 0, userA·s lower 32 */
	int32_t  passq_msb;          /* PASSQ offset 4, sign-ext upper */
	uint32_t passtime;           /* PASSQ offset 8, unit 250 ms */
} BQ76907_Monitor_t;

uint8_t BQ76907_Init(void);
uint8_t BQ76907_ReadBatteryStatus(uint16_t *status);
uint8_t BQ76907_IsCharging(int16_t *current_ma);
uint8_t BQ76907_ReadCurrent_mA(int16_t *current_ma);
uint8_t BQ76907_ReadCurrentRaw_mA(int16_t *current_ma);
uint8_t BQ76907_EnableDischarge(void); /* battery; CUV → DSG off, CHG on */
uint8_t BQ76907_EnableCharge(void);    /* charge; CUV keeps CHG on */
uint8_t BQ76907_DisableFets(void);
uint8_t BQ76907_ReadSafetyStatusA(uint8_t *status);
uint8_t BQ76907_ReadSafetyAlertA(uint8_t *alert);
uint8_t BQ76907_IsCellUvLatched(void);
uint8_t BQ76907_IsExternalPowerPresent(void);
void PowerMgmt_TripCellUvFromCells(const uint16_t *cells_mv, uint8_t n);
uint8_t BQ76907_ReadStackVoltage_x10(uint16_t *voltage_x10);
/* Copy last-read cell voltages (mV). cells_mv may be NULL; n <= BQ76907_CELL_COUNT. */
uint8_t BQ76907_GetCellVoltages_mV(uint16_t *cells_mv, uint8_t n, uint8_t *valid_count);
uint16_t BQ76907_GetCellMin_mV(void);
/* Read yellow-box direct/subcommand values into *mon (and optional globals via caller). */
uint8_t BQ76907_ReadMonitor(BQ76907_Monitor_t *mon);
/* Cell UV/OV protection thresholds (data memory), unit mV */
uint8_t BQ76907_ReadCuvThreshold_mV(uint16_t *mv);
uint8_t BQ76907_ReadCovThreshold_mV(uint16_t *mv);
uint8_t BQ76907_WriteCuvThreshold_mV(uint16_t mv);
uint8_t BQ76907_WriteCovThreshold_mV(uint16_t mv);
uint8_t BQ76907_WriteCuvCovThreshold_mV(uint16_t cuv_mv, uint16_t cov_mv);
/* Load UV/OV/shutdown/OC from Flash (defaults if blank); call before BootInit CFG */
void PowerProt_LoadFromFlash(void);
/* Update runtime + Flash; rewrite BQ DM if power path already configured */
uint8_t PowerProt_SetThresholds(uint16_t cuv_mv, uint16_t cov_mv, uint16_t shutdown_cell_mv,
				uint16_t oc_chg_ma, uint16_t oc_dsg_ma);
/* After BootInit: Enabled Protections A + CUV/COV verify bits (see PWR_TEST_PROT_*) */
uint8_t BQ76907_GetProtEnabledA(void);
uint8_t BQ76907_GetProtVerify(void);
uint8_t BQ76907_GetProtCfgErr(void);
uint8_t BQ76907_GetFetCtrl(void);
uint8_t BQ76907_GetFetOptions(void);
uint8_t BQ76907_GetSafetyStatusB(void);
uint16_t BQ76907_GetAlarmStatus(void);
uint16_t BQ76907_GetFetOffReason(void); /* sticky last FET-off code */
uint16_t BQ76907_GetFetOffDetail(void); /* sticky context for that code */
void BQ76907_FetOffLog_Restore(void);   /* load last reason from Flash after reboot */
void BQ76907_FetOffFlash_Prepare(void); /* erase page while power stable (BootInit) */
uint8_t BQ76907_RefreshProtVerify(void);
/* bqStudio Commands pane: SHUTDOWN / RESET / FET_ENABLE */
uint8_t BQ76907_CmdShutdown(void);
uint8_t BQ76907_CmdReset(void);
uint8_t BQ76907_CmdFetEnable(void);
/* Clear soft UV/OV/OC latches, PROT_RECOVERY, force CHG+DSG on (keep charger plugged) */
uint8_t BQ76907_CmdFetRecover(void);
uint8_t PowerLED_BarsFromVoltage(uint16_t voltage_x10);
uint8_t PowerMgmt_AutoControl(uint16_t *voltage_x10, uint8_t *bars);
/* GPIO32: BMS HW reset, active-high pulse; init holds low */
void BMS_HwReset_Init(void);
void BMS_HwReset_Pulse(void);
uint8_t PowerMgmt_BootInit(void);

#endif
