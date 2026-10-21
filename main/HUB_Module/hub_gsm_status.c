#include "hub_gsm_status.h"

#include <stdio.h>
#include <string.h>

#include "a7608.h"
#include "hub_lte_pppos.h"
#include "modbus.h"
#include "user_data.h"

static void hub_gsm_parse_ipv4(const char *ip_str, uint8_t out[4])
{
	unsigned a = 0;
	unsigned b = 0;
	unsigned c = 0;
	unsigned d = 0;

	out[0] = out[1] = out[2] = out[3] = 0;
	if ((ip_str == NULL) || (ip_str[0] == '\0')) {
		return;
	}
	if (sscanf(ip_str, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) {
		return;
	}
	if ((a > 255u) || (b > 255u) || (c > 255u) || (d > 255u)) {
		return;
	}
	out[0] = (uint8_t)a;
	out[1] = (uint8_t)b;
	out[2] = (uint8_t)c;
	out[3] = (uint8_t)d;
}

void hub_gsm_status_refresh(void)
{
	const a7608_status_t *st = a7608_get_status();
	hub_lte_pppos_status_t ppp;
	const char *ip_str;
	uint8_t flags = 0;
	esp_err_t last_err;

	memset(&gsm_status_point, 0, sizeof(gsm_status_point));
	memset(&ppp, 0, sizeof(ppp));

	if (st == NULL) {
		return;
	}

	(void)hub_lte_pppos_get_status(&ppp);

	gsm_status_point.reg.modem_state = (U8_T)st->state;
	gsm_status_point.reg.ppp_state = (U8_T)ppp.state;
	gsm_status_point.reg.active_sim = (U8_T)st->active_sim_slot;
	gsm_status_point.reg.csq = (U8_T)((st->csq < 0) ? 99 : ((st->csq > 255) ? 255 : st->csq));
	gsm_status_point.reg.rssi_dbm = st->rssi_valid ? (S8_T)st->rssi_dbm : (S8_T)0;
	gsm_status_point.reg.creg_stat = (U8_T)((st->creg_stat < 0) ? 0 : st->creg_stat);
	gsm_status_point.reg.cereg_stat = (U8_T)((st->cereg_stat < 0) ? 0 : st->cereg_stat);
	gsm_status_point.reg.status_age_s = (U16_T)(st->status_age_ms / 1000u);

	last_err = st->last_refresh_result;
	if (last_err == ESP_OK) {
		last_err = hub_lte_pppos_get_last_error();
	}
	gsm_status_point.reg.last_error = (U16_T)((uint32_t)last_err & 0xffffu);

	if (st->sim_ready) {
		flags |= GSM_FLAG_SIM_READY;
	}
	if (st->registered_home || st->registered_roaming || a7608_status_is_registered()) {
		flags |= GSM_FLAG_REGISTERED;
	}
	if (st->attached) {
		flags |= GSM_FLAG_ATTACHED;
	}
	if (st->connected || ppp.connected || hub_lte_pppos_is_connected()) {
		flags |= GSM_FLAG_CONNECTED;
	}
	if (st->at_ready) {
		flags |= GSM_FLAG_AT_READY;
	}
	/* Live GPIO detect on each status read (not only cached a7608_status). */
	if (a7608_sim_slot_detected(A7608_SIM_SLOT_1) || st->sim1_present) {
		flags |= GSM_FLAG_SIM1_PRESENT;
	}
	if (a7608_sim_slot_detected(A7608_SIM_SLOT_2) || st->sim2_present) {
		flags |= GSM_FLAG_SIM2_PRESENT;
	}
	if (st->status_valid) {
		flags |= GSM_FLAG_STATUS_VALID;
	}
	gsm_status_point.reg.flags = flags;

	ip_str = hub_lte_pppos_get_ip_addr();
	if ((ip_str == NULL) || (ip_str[0] == '\0')) {
		ip_str = st->ip_addr;
	}
	hub_gsm_parse_ipv4(ip_str, gsm_status_point.reg.ip);

	strncpy((char *)gsm_status_point.reg.operator_name,
		st->operator_name,
		sizeof(gsm_status_point.reg.operator_name) - 1u);
	if (st->apn[0] != '\0') {
		strncpy((char *)gsm_status_point.reg.apn,
			st->apn,
			sizeof(gsm_status_point.reg.apn) - 1u);
	} else if (ppp.connected) {
		hub_lte_pppos_config_t cfg;
		if (hub_lte_pppos_get_config(&cfg) == ESP_OK) {
			strncpy((char *)gsm_status_point.reg.apn,
				cfg.apn,
				sizeof(gsm_status_point.reg.apn) - 1u);
		}
	}
}

uint16_t gsm_status_read_by_block(uint16_t addr)
{
	uint8_t item;

	hub_gsm_status_refresh();

	if (addr == MODBUS_GSM_MODEM_STATE) {
		return gsm_status_point.reg.modem_state;
	}
	if (addr == MODBUS_GSM_PPP_STATE) {
		return gsm_status_point.reg.ppp_state;
	}
	if (addr == MODBUS_GSM_FLAGS) {
		return gsm_status_point.reg.flags;
	}
	if (addr == MODBUS_GSM_ACTIVE_SIM) {
		return gsm_status_point.reg.active_sim;
	}
	if (addr == MODBUS_GSM_CSQ) {
		return gsm_status_point.reg.csq;
	}
	if (addr == MODBUS_GSM_RSSI_DBM) {
		return (uint16_t)(int16_t)gsm_status_point.reg.rssi_dbm;
	}
	if (addr == MODBUS_GSM_CREG) {
		return gsm_status_point.reg.creg_stat;
	}
	if (addr == MODBUS_GSM_CEREG) {
		return gsm_status_point.reg.cereg_stat;
	}
	if ((addr >= MODBUS_GSM_IP1) && (addr <= MODBUS_GSM_IP4)) {
		item = (uint8_t)(addr - MODBUS_GSM_IP1);
		return gsm_status_point.reg.ip[item];
	}
	if (addr == MODBUS_GSM_STATUS_AGE_S) {
		return gsm_status_point.reg.status_age_s;
	}
	if (addr == MODBUS_GSM_LAST_ERROR) {
		return gsm_status_point.reg.last_error;
	}
	if ((addr >= MODBUS_GSM_OPERATOR_START) && (addr <= MODBUS_GSM_OPERATOR_END)) {
		item = (uint8_t)(addr - MODBUS_GSM_OPERATOR_START);
		if ((item * 2u + 1u) < sizeof(gsm_status_point.reg.operator_name)) {
			return ((uint16_t)gsm_status_point.reg.operator_name[item * 2u] << 8) |
			       gsm_status_point.reg.operator_name[item * 2u + 1u];
		}
	}
	if ((addr >= MODBUS_GSM_APN_START) && (addr <= MODBUS_GSM_APN_END)) {
		item = (uint8_t)(addr - MODBUS_GSM_APN_START);
		if ((item * 2u + 1u) < sizeof(gsm_status_point.reg.apn)) {
			return ((uint16_t)gsm_status_point.reg.apn[item * 2u] << 8) |
			       gsm_status_point.reg.apn[item * 2u + 1u];
		}
	}

	return 0;
}
