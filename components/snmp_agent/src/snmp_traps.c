#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <lwip/netdb.h>
#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_timer.h>

#include "snmp_traps.h"

static const char *TAG = "SNMP_TRAP";

#define ASN_INTEGER   0x02
#define ASN_OCTET_STR 0x04
#define ASN_OID       0x06
#define ASN_SEQUENCE  0x30
#define ASN_TIMETICKS 0x43
#define PDU_INFORM    0xA6
#define PDU_TRAP      0xA7

typedef struct {
	uint32_t *oid;
	int oid_len;
	char *value;
} ExtraVarbind;

/* Large scratch — BSS, not task stack (~4.5 KB total). */
static uint8_t s_trap_vblist[512];
static uint8_t s_trap_vbl_wrapped[600];
static uint8_t s_trap_pdu_body[700];
static uint8_t s_trap_pdu_final[750];
static uint8_t s_trap_msg_body[800];
static uint8_t s_trap_packet[900];
static uint8_t s_trap_temp_oid[64];
static uint8_t s_trap_temp_inner[256];
static SemaphoreHandle_t s_trap_mutex;

static void trap_mutex_ensure(void)
{
	if (s_trap_mutex == NULL) {
		s_trap_mutex = xSemaphoreCreateMutex();
	}
}

static int encode_length(uint8_t *buf, int len)
{
	if (len < 128) {
		buf[0] = (uint8_t)len;
		return 1;
	}
	buf[0] = 0x81;
	buf[1] = (uint8_t)len;
	return 2;
}

static int encode_oid(const uint32_t *oid, int len, uint8_t *buf)
{
	int pos = 0;

	if (len < 2) {
		return 0;
	}
	buf[pos++] = (uint8_t)(oid[0] * 40 + oid[1]);
	for (int i = 2; i < len; i++) {
		uint32_t val = oid[i];
		if (val < 128) {
			buf[pos++] = (uint8_t)val;
		} else {
			uint8_t tmp[5];
			int j = 0;
			tmp[j++] = (uint8_t)(val & 0x7F);
			while (val >>= 7) {
				tmp[j++] = (uint8_t)((val & 0x7F) | 0x80);
			}
			while (j > 0) {
				buf[pos++] = tmp[--j];
			}
		}
	}
	return pos;
}

static int build_tlv(uint8_t type, int p_len, uint8_t *p_data, uint8_t *out)
{
	out[0] = type;
	int l_bytes = encode_length(out + 1, p_len);
	memcpy(out + 1 + l_bytes, p_data, p_len);
	return 1 + l_bytes + p_len;
}

static void snmp_send_v2(const char *local_ip, const char *dst_ip, uint16_t dst_port,
                         const char *community, uint32_t *trap_oid, int trap_oid_len,
                         ExtraVarbind *extras, int extra_count, int is_inform)
{
	int vbl_pos = 0;
	const char *comm = (community && community[0]) ? community : "public";
	size_t comm_len = strlen(comm);
	uint16_t port = dst_port ? dst_port : 162;
	int packet_len;
	int sock;
	struct sockaddr_in dest_addr = {0};

	if (dst_ip == NULL || dst_ip[0] == '\0' || trap_oid == NULL || trap_oid_len < 2) {
		return;
	}
	if (comm_len == 0 || comm_len > 32) {
		comm = "public";
		comm_len = 6;
	}

	trap_mutex_ensure();
	if (s_trap_mutex == NULL || xSemaphoreTake(s_trap_mutex, pdMS_TO_TICKS(2000)) != pdTRUE) {
		ESP_LOGW(TAG, "trap mutex busy/unavailable");
		return;
	}

	memset(s_trap_temp_oid, 0, sizeof(s_trap_temp_oid));
	memset(s_trap_temp_inner, 0, sizeof(s_trap_temp_inner));

	/* 1. sysUpTime.0 */
	{
		uint32_t uptime_oid[] = {1, 3, 6, 1, 2, 1, 1, 3, 0};
		int l = encode_oid(uptime_oid, 9, s_trap_temp_oid);
		int inner = build_tlv(ASN_OID, l, s_trap_temp_oid, s_trap_temp_inner);
		uint32_t ticks_val = (uint32_t)(esp_timer_get_time() / 10000ULL);
		uint8_t ticks[4] = {
			(uint8_t)((ticks_val >> 24) & 0xFF),
			(uint8_t)((ticks_val >> 16) & 0xFF),
			(uint8_t)((ticks_val >> 8) & 0xFF),
			(uint8_t)(ticks_val & 0xFF)
		};
		inner += build_tlv(ASN_TIMETICKS, 4, ticks, s_trap_temp_inner + inner);
		vbl_pos += build_tlv(ASN_SEQUENCE, inner, s_trap_temp_inner, s_trap_vblist + vbl_pos);
	}

	/* 2. snmpTrapOID.0 */
	{
		uint32_t snmp_trap_p[] = {1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0};
		int l = encode_oid(snmp_trap_p, 11, s_trap_temp_oid);
		int inner = build_tlv(ASN_OID, l, s_trap_temp_oid, s_trap_temp_inner);
		l = encode_oid(trap_oid, trap_oid_len, s_trap_temp_oid);
		inner += build_tlv(ASN_OID, l, s_trap_temp_oid, s_trap_temp_inner + inner);
		vbl_pos += build_tlv(ASN_SEQUENCE, inner, s_trap_temp_inner, s_trap_vblist + vbl_pos);
	}

	for (int i = 0; i < extra_count; i++) {
		int l = encode_oid(extras[i].oid, extras[i].oid_len, s_trap_temp_oid);
		int inner = build_tlv(ASN_OID, l, s_trap_temp_oid, s_trap_temp_inner);
		inner += build_tlv(ASN_OCTET_STR, (int)strlen(extras[i].value),
		                   (uint8_t *)extras[i].value, s_trap_temp_inner + inner);
		vbl_pos += build_tlv(ASN_SEQUENCE, inner, s_trap_temp_inner, s_trap_vblist + vbl_pos);
	}

	{
		int vbl_len = build_tlv(ASN_SEQUENCE, vbl_pos, s_trap_vblist, s_trap_vbl_wrapped);
		uint8_t pdu_hdr[] = {0x02, 0x01, 0x01, 0x02, 0x01, 0x00, 0x02, 0x01, 0x00};
		int msg_hdr_len = 0;
		int pdu_len;

		memcpy(s_trap_pdu_body, pdu_hdr, 9);
		memcpy(s_trap_pdu_body + 9, s_trap_vbl_wrapped, vbl_len);
		pdu_len = build_tlv(is_inform ? PDU_INFORM : PDU_TRAP, 9 + vbl_len, s_trap_pdu_body, s_trap_pdu_final);

		s_trap_msg_body[msg_hdr_len++] = 0x02;
		s_trap_msg_body[msg_hdr_len++] = 0x01;
		s_trap_msg_body[msg_hdr_len++] = 0x01; /* SNMPv2c */
		s_trap_msg_body[msg_hdr_len++] = ASN_OCTET_STR;
		s_trap_msg_body[msg_hdr_len++] = (uint8_t)comm_len;
		memcpy(s_trap_msg_body + msg_hdr_len, comm, comm_len);
		msg_hdr_len += (int)comm_len;
		memcpy(s_trap_msg_body + msg_hdr_len, s_trap_pdu_final, pdu_len);
		packet_len = build_tlv(ASN_SEQUENCE, msg_hdr_len + pdu_len, s_trap_msg_body, s_trap_packet);
	}

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(port);
	if (inet_addr(dst_ip) == (uint32_t)0xFFFFFFFF) {
		ESP_LOGW(TAG, "Invalid trap destination IP %s", dst_ip);
		xSemaphoreGive(s_trap_mutex);
		return;
	}
	dest_addr.sin_addr.s_addr = inet_addr(dst_ip);

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock < 0) {
		ESP_LOGE(TAG, "trap socket create failed");
		xSemaphoreGive(s_trap_mutex);
		return;
	}

	if (local_ip && local_ip[0] && strcmp(local_ip, "0.0.0.0") != 0) {
		struct sockaddr_in src_addr = {0};
		src_addr.sin_family = AF_INET;
		src_addr.sin_port = htons(0);
		src_addr.sin_addr.s_addr = inet_addr(local_ip);
		(void)bind(sock, (struct sockaddr *)&src_addr, sizeof(src_addr));
	}

	{
		struct timeval tv = {.tv_sec = 2, .tv_usec = 0};
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	}

	{
		int sent = sendto(sock, s_trap_packet, packet_len, 0,
		                  (struct sockaddr *)&dest_addr, sizeof(dest_addr));
		if (sent < 0) {
			ESP_LOGW(TAG, "SNMP %s sendto %s:%u failed",
			         is_inform ? "INFORM" : "TRAP", dst_ip, (unsigned)port);
		} else {
			ESP_LOGI(TAG, "SNMP %s sent to %s:%u (%d bytes)",
			         is_inform ? "INFORM" : "TRAP", dst_ip, (unsigned)port, sent);
		}
	}

	if (is_inform) {
		uint8_t rx_buf[128];
		int len = recv(sock, rx_buf, sizeof(rx_buf), 0);
		if (len > 0) {
			ESP_LOGI(TAG, "INFORM ACK received");
		} else {
			ESP_LOGW(TAG, "INFORM Timeout");
		}
	}
	close(sock);
	xSemaphoreGive(s_trap_mutex);
}

void send_snmp_trap_cold_start(const char *l_ip, const char *d_ip, uint16_t d_port, const char *community)
{
	uint32_t oid[] = {1, 3, 6, 1, 6, 3, 1, 1, 5, 1};
	snmp_send_v2(l_ip, d_ip, d_port, community, oid, 10, NULL, 0, 0);
}

void send_snmp_trap_link_up(const char *l_ip, const char *d_ip, uint16_t d_port, const char *community)
{
	uint32_t oid[] = {1, 3, 6, 1, 6, 3, 1, 1, 5, 4};
	snmp_send_v2(l_ip, d_ip, d_port, community, oid, 10, NULL, 0, 0);
}

void send_snmp_trap_warm_start(const char *l_ip, const char *d_ip, uint16_t d_port, const char *community)
{
	uint32_t oid[] = {1, 3, 6, 1, 6, 3, 1, 1, 5, 2};
	snmp_send_v2(l_ip, d_ip, d_port, community, oid, 10, NULL, 0, 0);
}

void send_snmp_trap_link_down(const char *l_ip, const char *d_ip, uint16_t d_port, const char *community)
{
	uint32_t oid[] = {1, 3, 6, 1, 6, 3, 1, 1, 5, 3};
	snmp_send_v2(l_ip, d_ip, d_port, community, oid, 10, NULL, 0, 0);
}

void snmp_trap_auth_failure(const char *l_ip, const char *d_ip, uint16_t d_port, const char *community)
{
	uint32_t oid[] = {1, 3, 6, 1, 6, 3, 1, 1, 5, 5};
	snmp_send_v2(l_ip, d_ip, d_port, community, oid, 10, NULL, 0, 0);
}

void snmp_trap_enterprise(const char *l_ip, const char *d_ip, uint16_t d_port,
                          const char *community, const char *msg)
{
	uint32_t ent_oid[] = {1, 3, 6, 1, 4, 1, 64991, 30, 0};
	uint32_t msg_oid[] = {1, 3, 6, 1, 4, 1, 64991, 30, 1, 0};
	ExtraVarbind ex = {
		.oid = msg_oid,
		.oid_len = 10,
		.value = (char *)(msg ? msg : "Temco Hub event")
	};
	snmp_send_v2(l_ip, d_ip, d_port, community, ent_oid, 9, &ex, 1, 0);
}
