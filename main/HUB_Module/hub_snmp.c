/**
 * @file hub_snmp.c
 * @brief Hub SNMP agent wrapper — WAN wait, config, minimal system MIBs.
 */

#include "hub_snmp.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "snmp_agent.h"
#include "snmp_traps.h"
#include "snmpdefs.h"
#include "mib.h"
#include "miblist.h"
#include "oid.h"

#include "hub_network_manager.h"
#include "modbus.h"
#include "user_data.h"

static const char *TAG = "hub_snmp";

#define WIFI_CONNECTED_BIT          BIT0
#define HUB_SNMP_STACK_SIZE         (8192 + 4096)
#define HUB_SNMP_TASK_PRIO          5
#define HUB_SNMP_ENTERPRISE_OID     "P.64991.30"
#define HUB_SNMP_PRIVATE_STUB_OID   "P.64991.30.1.0"

extern EventGroupHandle_t s_wifi_event_group;

static char s_enterprise_oid[] = HUB_SNMP_ENTERPRISE_OID;
static char s_ro_community[32];
static char s_rw_community[32];
static char s_sys_descr[] = "Temco Hub ESP32";
static char s_sys_contact[32];
static char s_sys_name[32];
static char s_sys_location[32];
static char s_sys_ip[16] = "0.0.0.0";
static unsigned char s_ent_oid_ber[MIB_DATA_SIZE];

static TaskHandle_t s_snmp_task;
static volatile bool s_running;
static volatile bool s_reload_req;
static volatile bool s_deinit_req;
static bool s_inited;

static MIB *s_mib;
static uint32_t s_u32;

static bool hub_snmp_wifi_ready(void)
{
	return (s_wifi_event_group != NULL) &&
	       ((xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT) != 0);
}

static bool hub_snmp_gsm_ready(void)
{
	hub_network_manager_status_t status;

	if (hub_network_manager_get_status(&status) != ESP_OK) {
		return false;
	}
	return status.lte_connected && (status.lte_ip_addr[0] != '\0');
}

static bool hub_snmp_auto_underlay_ready(const char **underlay_name)
{
	if (hub_snmp_wifi_ready()) {
		if (underlay_name) {
			*underlay_name = "wifi";
		}
		return true;
	}
	if (hub_snmp_gsm_ready()) {
		if (underlay_name) {
			*underlay_name = "gsm";
		}
		return true;
	}
	return false;
}

static void hub_snmp_refresh_local_ip(void)
{
	esp_netif_t *netif = esp_netif_get_default_netif();
	esp_netif_ip_info_t ip_info;

	if (netif == NULL) {
		return;
	}
	if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) {
		return;
	}
	snprintf(s_sys_ip, sizeof(s_sys_ip), IPSTR, IP2STR(&ip_info.ip));
}

static int hub_snmp_get_uptime(MIB *thismib)
{
	thismib->u.intval = sysUpTime();
	return SUCCESS;
}

static int hub_snmp_get_ipaddress(MIB *thismib)
{
	/* uSNMP exposes agent IP as OCTET_STRING at B.1.4.20 (not ASN.1 IpAddress). */
	hub_snmp_refresh_local_ip();
	thismib->u.octetstring = (unsigned char *)s_sys_ip;
	thismib->dataLen = (int)strlen(s_sys_ip);
	return SUCCESS;
}

static void hub_snmp_copy_config_buffers(void)
{
	memset(s_ro_community, 0, sizeof(s_ro_community));
	memset(s_rw_community, 0, sizeof(s_rw_community));
	memset(s_sys_contact, 0, sizeof(s_sys_contact));
	memset(s_sys_name, 0, sizeof(s_sys_name));
	memset(s_sys_location, 0, sizeof(s_sys_location));

	strncpy(s_ro_community,
	        snmp_point.reg.ro_community[0] ? snmp_point.reg.ro_community : "public",
	        sizeof(s_ro_community) - 1);
	strncpy(s_rw_community,
	        snmp_point.reg.rw_community[0] ? snmp_point.reg.rw_community : "private",
	        sizeof(s_rw_community) - 1);
	strncpy(s_sys_contact,
	        snmp_point.reg.sys_contact[0] ? snmp_point.reg.sys_contact : "support@temcocontrols.com",
	        sizeof(s_sys_contact) - 1);
	strncpy(s_sys_name,
	        snmp_point.reg.sys_name[0] ? snmp_point.reg.sys_name : "Temco Hub",
	        sizeof(s_sys_name) - 1);
	strncpy(s_sys_location,
	        snmp_point.reg.sys_location[0] ? snmp_point.reg.sys_location : "Temco Controls",
	        sizeof(s_sys_location) - 1);
}

static MIB *hub_snmp_add_mib(const char *oidstr, unsigned char dataType, char access,
                             void *data, int size)
{
	MIB *mib;

	if (mibTree == NULL) {
		ESP_LOGE(TAG, "mibTree is NULL — cannot add %s", oidstr ? oidstr : "?");
		return NULL;
	}

	mib = miblistadd(mibTree, (char *)oidstr, dataType, access, data, size);
	if (mib == NULL) {
		ESP_LOGE(TAG, "miblistadd failed for %s (duplicate or OOM)", oidstr ? oidstr : "?");
	}
	return mib;
}

static void hub_snmp_init_standard_mibs(void)
{
	int ber_len;

	s_mib = hub_snmp_add_mib("B.1.1.0", OCTET_STRING, RD_ONLY,
	                         s_sys_descr, (int)strlen(s_sys_descr));

	/* Encode enterprise OID first, then register sysObjectID with that length. */
	memset(s_ent_oid_ber, 0, sizeof(s_ent_oid_ber));
	ber_len = str2ber(s_enterprise_oid, s_ent_oid_ber);
	if (ber_len <= 0) {
		ESP_LOGE(TAG, "str2ber failed for enterprise OID %s", s_enterprise_oid);
		ber_len = 0;
	}
	s_mib = hub_snmp_add_mib("B.1.2.0", OBJECT_IDENTIFIER, RD_ONLY,
	                         s_ent_oid_ber, ber_len);
	if (s_mib != NULL && ber_len > 0) {
		mibsetvalue(s_mib, (void *)s_ent_oid_ber, ber_len);
	}

	s_u32 = 0;
	s_mib = hub_snmp_add_mib("B.1.3.0", TIMETICKS, RD_ONLY, NULL, 0);
	if (s_mib != NULL) {
		mibsetvalue(s_mib, &s_u32, 0);
		mibsetcallback(s_mib, hub_snmp_get_uptime, NULL);
	}

	s_mib = hub_snmp_add_mib("B.1.4.0", OCTET_STRING, RD_WR,
	                         s_sys_contact, (int)strlen(s_sys_contact));

	s_mib = hub_snmp_add_mib("B.1.4.20", OCTET_STRING, RD_ONLY,
	                         s_sys_ip, (int)strlen(s_sys_ip));
	if (s_mib != NULL) {
		mibsetcallback(s_mib, hub_snmp_get_ipaddress, NULL);
	}

	s_mib = hub_snmp_add_mib("B.1.5.0", OCTET_STRING, RD_WR,
	                         s_sys_name, (int)strlen(s_sys_name));
	s_mib = hub_snmp_add_mib("B.1.6.0", OCTET_STRING, RD_WR,
	                         s_sys_location, (int)strlen(s_sys_location));

	s_mib = hub_snmp_add_mib("B.1.7.0", INTEGER, RD_ONLY, NULL, 0);
	if (s_mib != NULL) {
		s_u32 = 5;
		mibsetvalue(s_mib, &s_u32, 0);
	}
}

static void hub_snmp_init_private_stub(void)
{
	s_mib = hub_snmp_add_mib(HUB_SNMP_PRIVATE_STUB_OID, INTEGER, RD_ONLY, NULL, 0);
	if (s_mib != NULL) {
		s_u32 = 1;
		mibsetvalue(s_mib, &s_u32, 0);
	}
}

static bool hub_snmp_trap_dest_configured(char *dest, size_t dest_len, uint16_t *port_out)
{
	const uint8_t *ip = snmp_point.reg.trap_dest_ip;

	if (dest == NULL || dest_len < 8 || port_out == NULL) {
		return false;
	}
	if (ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0) {
		return false;
	}
	snprintf(dest, dest_len, "%u.%u.%u.%u",
	         (unsigned)ip[0], (unsigned)ip[1], (unsigned)ip[2], (unsigned)ip[3]);
	*port_out = snmp_point.reg.trap_dest_port ? snmp_point.reg.trap_dest_port : (uint16_t)TRAP_DST_PORT;
	return true;
}

static void hub_snmp_send_cold_start_trap(void)
{
	char dest[16];
	uint16_t tport;

	if (!hub_snmp_trap_dest_configured(dest, sizeof(dest), &tport)) {
		ESP_LOGI(TAG, "Trap dest not set — skip coldStart");
		return;
	}
	hub_snmp_refresh_local_ip();
	send_snmp_trap_cold_start(s_sys_ip, dest, tport, s_ro_community);
}

static void hub_snmp_init_mib_tree(void)
{
	hub_snmp_refresh_local_ip();
	hub_snmp_init_standard_mibs();
	hub_snmp_init_private_stub();
}

static void hub_snmp_stop_agent(void)
{
	if (!s_running) {
		return;
	}
	exitSnmpAgent();
	s_running = false;
	snmp_point.reg.running = 0;
	ESP_LOGI(TAG, "SNMP agent stopped");
}

static bool hub_snmp_start_agent(void)
{
	uint16_t port = snmp_point.reg.port ? snmp_point.reg.port : (uint16_t)SNMP_PORT;

	hub_snmp_copy_config_buffers();

	if (initSnmpAgent((int)port, s_enterprise_oid, s_ro_community, s_rw_community) != SUCCESS) {
		ESP_LOGE(TAG, "initSnmpAgent failed (port=%u)", (unsigned)port);
		return false;
	}

	if (mibTree == NULL) {
		ESP_LOGE(TAG, "initSnmpAgent OK but mibTree is NULL");
		exitSnmpAgent();
		return false;
	}

	hub_snmp_init_mib_tree();
	s_running = true;
	snmp_point.reg.running = 1;

	ESP_LOGI(TAG, "SNMP agent started port=%u ro=%s rw=%s sysName=%s mibs=%d",
	         (unsigned)port, s_ro_community, s_rw_community, s_sys_name,
	         miblistsize(mibTree));

	hub_snmp_send_cold_start_trap();
	return true;
}

static void hub_snmp_task(void *pvParameters)
{
	const char *underlay = NULL;
	uint8_t wait_log = 0;

	(void)pvParameters;
	ESP_LOGI(TAG, "SNMP task started");

	while (!s_deinit_req) {
		vTaskDelay(pdMS_TO_TICKS(200));

		if (s_reload_req) {
			s_reload_req = false;
			hub_snmp_stop_agent();
		}

		if (snmp_point.reg.enable == 0) {
			if (s_running) {
				hub_snmp_stop_agent();
			}
			continue;
		}

		if (!hub_snmp_auto_underlay_ready(&underlay)) {
			if (s_running) {
				ESP_LOGW(TAG, "Underlay lost — stopping SNMP");
				hub_snmp_stop_agent();
			} else if ((++wait_log % 25) == 1) {
				ESP_LOGI(TAG, "Waiting for WAN (WiFi or LTE)...");
			}
			continue;
		}

		if (!s_running) {
			ESP_LOGI(TAG, "AUTO underlay ready (%s) — starting SNMP", underlay ? underlay : "?");
			if (!hub_snmp_start_agent()) {
				vTaskDelay(pdMS_TO_TICKS(2000));
				continue;
			}
		}

		(void)processSNMP();
	}

	hub_snmp_stop_agent();
	s_snmp_task = NULL;
	ESP_LOGI(TAG, "SNMP task exiting");
	vTaskDelete(NULL);
}

esp_err_t hub_snmp_init(void)
{
	if (s_inited) {
		return ESP_OK;
	}

	s_deinit_req = false;
	s_reload_req = false;

	BaseType_t ok = xTaskCreate(hub_snmp_task, "hub_snmp", HUB_SNMP_STACK_SIZE,
	                            NULL, HUB_SNMP_TASK_PRIO, &s_snmp_task);
	if (ok != pdPASS) {
		ESP_LOGE(TAG, "Failed to create hub_snmp task");
		return ESP_ERR_NO_MEM;
	}

	s_inited = true;
	ESP_LOGI(TAG, "hub_snmp_init ok (enable=%u)", (unsigned)snmp_point.reg.enable);
	return ESP_OK;
}

esp_err_t hub_snmp_apply_config(void)
{
	if (!s_inited) {
		return hub_snmp_init();
	}
	s_reload_req = true;
	ESP_LOGI(TAG, "Config apply requested (enable=%u port=%u)",
	         (unsigned)snmp_point.reg.enable,
	         (unsigned)(snmp_point.reg.port ? snmp_point.reg.port : SNMP_PORT));
	return ESP_OK;
}

esp_err_t hub_snmp_deinit(void)
{
	if (!s_inited) {
		return ESP_OK;
	}

	s_deinit_req = true;
	for (int i = 0; i < 50 && s_snmp_task != NULL; i++) {
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	if (s_snmp_task != NULL) {
		hub_snmp_stop_agent();
		vTaskDelete(s_snmp_task);
		s_snmp_task = NULL;
	}

	s_inited = false;
	s_deinit_req = false;
	ESP_LOGI(TAG, "hub_snmp_deinit done");
	return ESP_OK;
}

bool hub_snmp_is_running(void)
{
	return s_running;
}

static uint16_t snmp_pack_chars(const char *buf, size_t buf_size, uint8_t item)
{
	if ((item * 2u + 1u) < buf_size) {
		return ((uint16_t)(uint8_t)buf[item * 2u] << 8) | (uint8_t)buf[item * 2u + 1u];
	}
	return 0;
}

static void snmp_unpack_chars(char *buf, size_t buf_size, uint8_t item, uint16_t value)
{
	if ((item * 2u + 1u) < buf_size) {
		buf[item * 2u] = (char)((value >> 8) & 0xff);
		buf[item * 2u + 1u] = (char)(value & 0xff);
	}
}

uint16_t snmp_config_read_by_block(uint16_t addr)
{
	uint8_t item;

	snmp_point.reg.running = s_running ? 1 : 0;

	if (addr == MODBUS_SNMP_ENABLE) {
		return snmp_point.reg.enable;
	}
	if (addr == MODBUS_SNMP_RUNNING) {
		return snmp_point.reg.running;
	}
	if (addr == MODBUS_SNMP_PORT) {
		return snmp_point.reg.port;
	}
	if ((addr >= MODBUS_SNMP_RO_COMMUNITY_START) && (addr <= MODBUS_SNMP_RO_COMMUNITY_END)) {
		item = (uint8_t)(addr - MODBUS_SNMP_RO_COMMUNITY_START);
		return snmp_pack_chars(snmp_point.reg.ro_community, sizeof(snmp_point.reg.ro_community), item);
	}
	if ((addr >= MODBUS_SNMP_RW_COMMUNITY_START) && (addr <= MODBUS_SNMP_RW_COMMUNITY_END)) {
		item = (uint8_t)(addr - MODBUS_SNMP_RW_COMMUNITY_START);
		return snmp_pack_chars(snmp_point.reg.rw_community, sizeof(snmp_point.reg.rw_community), item);
	}
	if ((addr >= MODBUS_SNMP_TRAP_IP1) && (addr <= MODBUS_SNMP_TRAP_IP4)) {
		item = (uint8_t)(addr - MODBUS_SNMP_TRAP_IP1);
		return snmp_point.reg.trap_dest_ip[item];
	}
	if (addr == MODBUS_SNMP_TRAP_PORT) {
		return snmp_point.reg.trap_dest_port;
	}
	if ((addr >= MODBUS_SNMP_SYS_NAME_START) && (addr <= MODBUS_SNMP_SYS_NAME_END)) {
		item = (uint8_t)(addr - MODBUS_SNMP_SYS_NAME_START);
		return snmp_pack_chars(snmp_point.reg.sys_name, sizeof(snmp_point.reg.sys_name), item);
	}
	if ((addr >= MODBUS_SNMP_SYS_LOCATION_START) && (addr <= MODBUS_SNMP_SYS_LOCATION_END)) {
		item = (uint8_t)(addr - MODBUS_SNMP_SYS_LOCATION_START);
		return snmp_pack_chars(snmp_point.reg.sys_location, sizeof(snmp_point.reg.sys_location), item);
	}
	if ((addr >= MODBUS_SNMP_SYS_CONTACT_START) && (addr <= MODBUS_SNMP_SYS_CONTACT_END)) {
		item = (uint8_t)(addr - MODBUS_SNMP_SYS_CONTACT_START);
		return snmp_pack_chars(snmp_point.reg.sys_contact, sizeof(snmp_point.reg.sys_contact), item);
	}
	return 0;
}

static bool snmp_write_register(uint16_t addr, uint16_t value)
{
	uint8_t item;

	if (addr == MODBUS_SNMP_ENABLE) {
		snmp_point.reg.enable = (U8_T)(value ? 1 : 0);
		return true;
	}
	if (addr == MODBUS_SNMP_RUNNING) {
		return false;
	}
	if (addr == MODBUS_SNMP_PORT) {
		snmp_point.reg.port = value ? value : 161;
		return true;
	}
	if ((addr >= MODBUS_SNMP_RO_COMMUNITY_START) && (addr <= MODBUS_SNMP_RO_COMMUNITY_END)) {
		item = (uint8_t)(addr - MODBUS_SNMP_RO_COMMUNITY_START);
		snmp_unpack_chars(snmp_point.reg.ro_community, sizeof(snmp_point.reg.ro_community), item, value);
		return true;
	}
	if ((addr >= MODBUS_SNMP_RW_COMMUNITY_START) && (addr <= MODBUS_SNMP_RW_COMMUNITY_END)) {
		item = (uint8_t)(addr - MODBUS_SNMP_RW_COMMUNITY_START);
		snmp_unpack_chars(snmp_point.reg.rw_community, sizeof(snmp_point.reg.rw_community), item, value);
		return true;
	}
	if ((addr >= MODBUS_SNMP_TRAP_IP1) && (addr <= MODBUS_SNMP_TRAP_IP4)) {
		item = (uint8_t)(addr - MODBUS_SNMP_TRAP_IP1);
		snmp_point.reg.trap_dest_ip[item] = (U8_T)(value & 0xff);
		return true;
	}
	if (addr == MODBUS_SNMP_TRAP_PORT) {
		snmp_point.reg.trap_dest_port = value ? value : 162;
		return true;
	}
	if ((addr >= MODBUS_SNMP_SYS_NAME_START) && (addr <= MODBUS_SNMP_SYS_NAME_END)) {
		item = (uint8_t)(addr - MODBUS_SNMP_SYS_NAME_START);
		snmp_unpack_chars(snmp_point.reg.sys_name, sizeof(snmp_point.reg.sys_name), item, value);
		return true;
	}
	if ((addr >= MODBUS_SNMP_SYS_LOCATION_START) && (addr <= MODBUS_SNMP_SYS_LOCATION_END)) {
		item = (uint8_t)(addr - MODBUS_SNMP_SYS_LOCATION_START);
		snmp_unpack_chars(snmp_point.reg.sys_location, sizeof(snmp_point.reg.sys_location), item, value);
		return true;
	}
	if ((addr >= MODBUS_SNMP_SYS_CONTACT_START) && (addr <= MODBUS_SNMP_SYS_CONTACT_END)) {
		item = (uint8_t)(addr - MODBUS_SNMP_SYS_CONTACT_START);
		snmp_unpack_chars(snmp_point.reg.sys_contact, sizeof(snmp_point.reg.sys_contact), item, value);
		return true;
	}
	return false;
}

void snmp_config_write_by_block(uint16_t addr, uint8_t HeadLen, uint8_t *pData)
{
	bool changed = false;
	extern esp_err_t save_snmp_config_to_flash(void);

	if (pData == NULL) {
		return;
	}

	if (pData[HeadLen + 1] == MULTIPLE_WRITE_VARIABLES) {
		uint16_t quantity = ((uint16_t)pData[HeadLen + 4] << 8) | pData[HeadLen + 5];
		uint8_t byte_count = pData[HeadLen + 6];
		for (uint16_t index = 0; index < quantity; index++) {
			uint16_t data_offset = HeadLen + 7 + (index * 2);
			if ((index * 2 + 1) >= byte_count) {
				break;
			}
			uint16_t value_word = ((uint16_t)pData[data_offset] << 8) | pData[data_offset + 1];
			changed |= snmp_write_register((uint16_t)(addr + index), value_word);
		}
	} else {
		uint16_t value_word = ((uint16_t)pData[HeadLen + 4] << 8) | pData[HeadLen + 5];
		changed = snmp_write_register(addr, value_word);
	}

	if (changed) {
		save_snmp_config_to_flash();
		hub_snmp_apply_config();
	}
}
