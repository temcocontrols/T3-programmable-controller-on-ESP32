#include "mini_bms.h"
#include "bq76907.h"
#include "define.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_strip.h"
#include "driver/rmt_tx.h"
#include "driver/gpio.h"

static const char *TAG = "mini_bms";

#define MINI_BMS_LED_GPIO      GPIO_NUM_15
#define MINI_BMS_LED_COUNT     6
#define MINI_BMS_LED_PERIOD_MS 200
#define MINI_BMS_LED_YELLOW_R  50
#define MINI_BMS_LED_YELLOW_G  50
#define MINI_BMS_LED_YELLOW_B  0

static led_strip_handle_t s_led_strip;
static volatile uint8_t s_led_bootloader;

uint16_t mini_bms_voltage_x10;
uint8_t  mini_bms_bars;
uint8_t  mini_bms_charging;
uint8_t  mini_bms_comm_ok;
uint8_t  mini_bms_ext_power = 1; /* assume 48V present until BMS current says otherwise */
int16_t  mini_bms_current_ma;
uint16_t mini_bms_cell_mv[BQ76907_CELL_COUNT];
volatile uint8_t mini_bms_prot_apply;

extern STR_PLC plc_power;
extern uint16 rmc_cuv;
extern uint16 rmc_cov;
extern uint16 rmc_stack;
extern uint16 rmc_oc_chg;
extern uint16 rmc_oc_dsg;

/*
 * LED layout (same roles as RMC refresh_power_leds):
 *  0: green=ext power, red=battery
 *  1: green=BQ comm OK, red=fail
 *  2..5: SOC 25/50/75/100% — red / yellow / green / green; top bar blinks when charging
 */
static void mini_bms_bq_task(void *pvParameters)
{
	uint16_t voltage_x10 = 0;
	uint8_t bars = 0;
	uint16_t boot_ticks = 0;
	uint8_t boot_done = 0;
	const uint16_t boot_need = (uint16_t)(POWER_BOOT_WAIT_SEC * (1000 / MINI_BMS_LED_PERIOD_MS));
	uint16_t last_cuv = 0, last_cov = 0, last_sd = 0, last_oc_c = 0, last_oc_d = 0;

	(void)pvParameters;

	BMS_HwReset_Init();
	ESP_LOGI(TAG, "BQ76907 task start (SDA=4 SCL=14 RST=32)");

	for (;;) {
		if (!boot_done) {
			if (boot_ticks < boot_need) {
				boot_ticks++;
			} else {
				if (PowerMgmt_BootInit() == 0) {
					ESP_LOGI(TAG, "PowerMgmt_BootInit OK");
				} else {
					ESP_LOGW(TAG, "PowerMgmt_BootInit failed, will retry");
					boot_ticks = 0;
					vTaskDelay(pdMS_TO_TICKS(MINI_BMS_LED_PERIOD_MS));
					continue;
				}
				boot_done = 1;
				last_cuv = rmc_cuv;
				last_cov = rmc_cov;
				last_sd = rmc_stack;
				last_oc_c = rmc_oc_chg;
				last_oc_d = rmc_oc_dsg;
			}
			vTaskDelay(pdMS_TO_TICKS(MINI_BMS_LED_PERIOD_MS));
			continue;
		}

		/* Host changed protection thresholds via Modbus → apply to chip */
		if (mini_bms_prot_apply
		    || last_cuv != rmc_cuv || last_cov != rmc_cov
		    || last_sd != rmc_stack
		    || last_oc_c != rmc_oc_chg || last_oc_d != rmc_oc_dsg) {
			if (PowerProt_SetThresholds(rmc_cuv, rmc_cov, rmc_stack,
						    rmc_oc_chg, rmc_oc_dsg) == 0) {
				last_cuv = rmc_cuv;
				last_cov = rmc_cov;
				last_sd = rmc_stack;
				last_oc_c = rmc_oc_chg;
				last_oc_d = rmc_oc_dsg;
				mini_bms_prot_apply = 0;
			}
		}

		if (PowerMgmt_AutoControl(&voltage_x10, &bars) == 0 && voltage_x10 > 0U) {
			uint8_t cell_valid = 0;
			uint8_t i;
			int16_t current_ma = 0;

			mini_bms_comm_ok = 1;
			mini_bms_voltage_x10 = voltage_x10;
			mini_bms_bars = bars;

			if (BQ76907_GetCellVoltages_mV(mini_bms_cell_mv, BQ76907_CELL_COUNT, &cell_valid) == 0) {
				PowerMgmt_TripCellUvFromCells(mini_bms_cell_mv, BQ76907_CELL_COUNT);
				for (i = 0; i < BQ76907_CELL_COUNT; i++) {
					/* REG1100..1106: cell V in 0.1 V units (mV/100), same as RMC I2C */
					plc_power.battery[i] = (uint8_t)(mini_bms_cell_mv[i] / 100U);
				}
			}

			mini_bms_charging = BQ76907_IsCharging(&current_ma);
			mini_bms_current_ma = current_ma;
			/*
			 * flag_48V_exist — same idea as RMC1232 calculate_plc_power():
			 *  current >= 0 → 1
			 *  current < 0 continuously for 10 s → 0
			 *  (while still inside the 10 s negative window, keep 1)
			 */
			{
				static uint8_t bms_neg_timing;
				static TickType_t t_bms_neg_start;

				if(mini_bms_current_ma < 0)
				{
					if(bms_neg_timing == 0)
					{
						bms_neg_timing = 1;
						t_bms_neg_start = xTaskGetTickCount();
					}
					if((xTaskGetTickCount() - t_bms_neg_start)
					   >= pdMS_TO_TICKS(5000))
						mini_bms_ext_power = 0;
					else
						mini_bms_ext_power = 1;
				}
				else
				{
					bms_neg_timing = 0;
					mini_bms_ext_power = 1;
				}
			}

			plc_power.battery_sum = voltage_x10; /* REG1107 pack 0.1 V */
			plc_power.flag_bms_comm = 1;           /* REG1113 */
			plc_power.flag_48V_exist = mini_bms_ext_power; /* REG1114 */
		} else {
			mini_bms_comm_ok = 0;
			plc_power.flag_bms_comm = 0;
			/* No BMS comm → assume 48V present (cannot judge from current) */
			plc_power.flag_48V_exist = 1;
			mini_bms_ext_power = 1;
			mini_bms_charging = 0;
		}

		vTaskDelay(pdMS_TO_TICKS(MINI_BMS_LED_PERIOD_MS));
	}
}

static void mini_bms_led_task(void *pvParameters)
{
	led_strip_handle_t strip;
	uint8_t charge_blink = 0;
	uint8_t i;

	(void)pvParameters;

	led_strip_config_t strip_config = {
		.strip_gpio_num = MINI_BMS_LED_GPIO,
		.max_leds = MINI_BMS_LED_COUNT,
		.led_model = LED_MODEL_WS2812,
		.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
		.flags.invert_out = false,
	};
	led_strip_rmt_config_t rmt_config = {
		.clk_src = RMT_CLK_SRC_DEFAULT,
		.resolution_hz = 10 * 1000 * 1000,
		.mem_block_symbols = 64,
		.flags.with_dma = false,
	};

	if (led_strip_new_rmt_device(&strip_config, &rmt_config, &strip) != ESP_OK) {
		ESP_LOGE(TAG, "led_strip_new_rmt_device failed (GPIO%d)", MINI_BMS_LED_GPIO);
		vTaskDelay(portMAX_DELAY);
	}
	s_led_strip = strip;

	ESP_LOGI(TAG, "LED task start (GPIO%d x%d)", MINI_BMS_LED_GPIO, MINI_BMS_LED_COUNT);

	for (;;) {
		if (s_led_bootloader) {
			for (i = 0; i < MINI_BMS_LED_COUNT; i++) {
				led_strip_set_pixel(strip, i,
					MINI_BMS_LED_YELLOW_R, MINI_BMS_LED_YELLOW_G, MINI_BMS_LED_YELLOW_B);
			}
			led_strip_refresh(strip);
			vTaskDelay(pdMS_TO_TICKS(MINI_BMS_LED_PERIOD_MS));
			continue;
		}

		uint8_t bars = mini_bms_bars;
		uint8_t charging = mini_bms_charging;
		uint8_t bat_ok = mini_bms_comm_ok;
		uint8_t ext_power = mini_bms_ext_power;
		uint8_t r = 0, g = 0, b = 0;

		if (charging) {
			charge_blink = (uint8_t)((xTaskGetTickCount() / pdMS_TO_TICKS(200)) & 1U);
		} else {
			charge_blink = 0;
		}

		/* LED0: ext power */
		if (ext_power) {
			led_strip_set_pixel(strip, 0, 0, 30, 0);
		} else {
			led_strip_set_pixel(strip, 0, 30, 0, 0);
		}

		/* LED1: BMS I2C OK */
		if (bat_ok) {
			led_strip_set_pixel(strip, 1, 0, 30, 0);
		} else {
			led_strip_set_pixel(strip, 1, 30, 0, 0);
		}

		/* LED2..5: SOC bars */
		if (bat_ok && bars > 0) {
			if (bars <= 1) {
				r = 30; g = 0; b = 0;
			} else if (bars <= 2) {
				r = 30; g = 30; b = 0;
			} else {
				r = 0; g = 30; b = 0;
			}

			for (i = 0; i < POWER_LED_SOC_COUNT; i++) {
				uint8_t idx = (uint8_t)(2 + i);
				if (i >= bars) {
					led_strip_set_pixel(strip, idx, 0, 0, 0);
				} else if (charging && i == (bars - 1) && !charge_blink) {
					led_strip_set_pixel(strip, idx, 0, 0, 0);
				} else {
					led_strip_set_pixel(strip, idx, r, g, b);
				}
			}
		} else {
			for (i = 0; i < POWER_LED_SOC_COUNT; i++) {
				led_strip_set_pixel(strip, (uint8_t)(2 + i), 0, 0, 0);
			}
		}

		led_strip_refresh(strip);
		vTaskDelay(pdMS_TO_TICKS(MINI_BMS_LED_PERIOD_MS));
	}
}

void mini_bms_clear_bootloader_indication(void)
{
	s_led_bootloader = 0;
}

void mini_bms_indicate_bootloader(void)
{
	uint8_t i;

	s_led_bootloader = 1;
	if (s_led_strip != NULL) {
		for (i = 0; i < MINI_BMS_LED_COUNT; i++) {
			led_strip_set_pixel(s_led_strip, i,
				MINI_BMS_LED_YELLOW_R, MINI_BMS_LED_YELLOW_G, MINI_BMS_LED_YELLOW_B);
		}
		led_strip_refresh(s_led_strip);
	}
	ESP_LOGI(TAG, "LEDs yellow — entering bootloader");
	vTaskDelay(pdMS_TO_TICKS(500));
}

void mini_bms_start_tasks(void)
{
	xTaskCreate(mini_bms_bq_task, "bms_bq", 4096, NULL, 10, NULL);
	xTaskCreate(mini_bms_led_task, "bms_led", 3072, NULL, 14, NULL);
	ESP_LOGI(TAG, "MINI_BMS tasks created");
}
