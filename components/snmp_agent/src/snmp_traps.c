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
#include <freertos/task.h>
#include <esp_system.h>

#include "snmp_traps.h"

static const char *TAG = "SNMP_TRAP";

// ASN.1 & SNMP Constants
#define ASN_INTEGER   0x02
#define ASN_OCTET_STR 0x04
#define ASN_OID       0x06
#define ASN_SEQUENCE  0x30
#define ASN_TIMETICKS 0x43
#define PDU_INFORM    0xA6
#define PDU_TRAP      0xA7

typedef struct
{
    uint32_t* oid;
    int oid_len;
    char* value;
} ExtraVarbind;

// --- BER Encoding Helpers ---

int encode_length(uint8_t *buf, int len)
{
    if (len < 128)
    {
        buf[0] = (uint8_t)len;
        return 1;
    }
    else
    {
        buf[0] = 0x81; 
        buf[1] = (uint8_t)len;
        return 2;
    }
}

int encode_oid(const uint32_t *oid, int len, uint8_t *buf)
{
    int pos = 0;
    buf[pos++] = (uint8_t)(oid[0] * 40 + oid[1]);
    for (int i = 2; i < len; i++)
    {
        uint32_t val = oid[i];
        if (val < 128)
        {
            buf[pos++] = (uint8_t)val;
        }
        else
        {
            uint8_t tmp[5]; int j = 0;
            tmp[j++] = (uint8_t)(val & 0x7F);
            while (val >>= 7) tmp[j++] = (uint8_t)((val & 0x7F) | 0x80);
            while (j > 0) buf[pos++] = tmp[--j];
        }
    }
    return pos;
}

int build_tlv(uint8_t type, int p_len, uint8_t *p_data, uint8_t *out)
{
    out[0] = type;
    int l_bytes = encode_length(out + 1, p_len);
    memcpy(out + 1 + l_bytes, p_data, p_len);
    return 1 + l_bytes + p_len;
}

// --- Core Sender Function ---
// --- SNMP Packet Construction & Send ---

/**
 * @param local_ip: The IP assigned to the ESP32 (source)
 * @param dst_ip: The IP of the SNMP Manager (destination)
 * @param trap_oid: The OID identifying the trap type
 * @param is_inform: 1 for INFORM (expects ACK), 0 for TRAP
 */
void snmp_send_v2(const char* local_ip, const char* dst_ip, uint32_t* trap_oid, int trap_oid_len, ExtraVarbind* extras, int extra_count, int is_inform)
{    
    uint8_t vblist_raw[512];
    int vbl_pos = 0;
    uint8_t temp_oid[64] = {0}, temp_inner[256] = {0};

    // 1. Varbind: sysUpTime.0 (.1.3.6.1.2.1.1.3.0)
    uint32_t uptime_oid[] = {1,3,6,1,2,1,1,3,0};
    int l = encode_oid(uptime_oid, 9, temp_oid);
    int inner = build_tlv(ASN_OID, l, temp_oid, temp_inner);
    uint32_t ticks_val = xTaskGetTickCount() * portTICK_PERIOD_MS / 10;
    uint8_t ticks[4] = { (ticks_val >> 24) & 0xFF, (ticks_val >> 16) & 0xFF, (ticks_val >> 8) & 0xFF, ticks_val & 0xFF };
    inner += build_tlv(ASN_TIMETICKS, 4, ticks, temp_inner + inner);
    vbl_pos += build_tlv(ASN_SEQUENCE, inner, temp_inner, vblist_raw + vbl_pos);

    // 2. Varbind: snmpTrapOID.0 (.1.3.6.1.6.3.1.1.4.1.0)
    uint32_t snmp_trap_p[] = {1,3,6,1,6,3,1,1,4,1,0};
    l = encode_oid(snmp_trap_p, 11, temp_oid);
    inner = build_tlv(ASN_OID, l, temp_oid, temp_inner);
    l = encode_oid(trap_oid, trap_oid_len, temp_oid);
    inner += build_tlv(ASN_OID, l, temp_oid, temp_inner + inner);
    vbl_pos += build_tlv(ASN_SEQUENCE, inner, temp_inner, vblist_raw + vbl_pos);

    // 3. Optional Extra Varbinds
    for(int i = 0; i < extra_count; i++) {
        l = encode_oid(extras[i].oid, extras[i].oid_len, temp_oid);
        inner = build_tlv(ASN_OID, l, temp_oid, temp_inner);
        inner += build_tlv(ASN_OCTET_STR, strlen(extras[i].value), (uint8_t*)extras[i].value, temp_inner + inner);
        vbl_pos += build_tlv(ASN_SEQUENCE, inner, temp_inner, vblist_raw + vbl_pos);
    }

    // Wrap List -> PDU -> Message
    uint8_t vbl_wrapped[600], pdu_body[700], pdu_final[750], msg_body[800], packet[900];
    int vbl_len = build_tlv(ASN_SEQUENCE, vbl_pos, vblist_raw, vbl_wrapped);
    
    uint8_t pdu_hdr[] = {0x02, 0x01, 0x01, 0x02, 0x01, 0x00, 0x02, 0x01, 0x00}; // ReqID 1
    memcpy(pdu_body, pdu_hdr, 9);
    memcpy(pdu_body + 9, vbl_wrapped, vbl_len);
    int pdu_len = build_tlv(is_inform ? PDU_INFORM : PDU_TRAP, 9 + vbl_len, pdu_body, pdu_final);

    uint8_t msg_hdr[] = {0x02, 0x01, 0x01, 0x04, 0x06, 'p', 'u', 'b', 'l', 'i', 'c'};
    memcpy(msg_body, msg_hdr, 11);
    memcpy(msg_body + 11, pdu_final, pdu_len);
    int packet_len = build_tlv(ASN_SEQUENCE, 11 + pdu_len, msg_body, packet);

    // --- Networking Layer ---
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(dst_ip);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(162);

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) return;

    // Set source identity (Local IP)
    struct sockaddr_in src_addr;
    src_addr.sin_family = AF_INET;
    src_addr.sin_port = htons(0); // Auto-assign port
    src_addr.sin_addr.s_addr = inet_addr(local_ip);
    bind(sock, (struct sockaddr *)&src_addr, sizeof(src_addr));

    // Set receive timeout for Informs
    struct timeval tv = { .tv_sec = 2, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    sendto(sock, packet, packet_len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    ESP_LOGI(TAG, "SNMP %s sent to %s", is_inform ? "INFORM" : "TRAP", dst_ip);

    if (is_inform) {
        uint8_t rx_buf[128];
        int len = recv(sock, rx_buf, sizeof(rx_buf), 0);
        if (len > 0) ESP_LOGI(TAG, "INFORM ACK received");
        else ESP_LOGW(TAG, "INFORM Timeout");
    }
    close(sock);
}

// --- Specific Trap Wrappers ---

void send_snmp_trap_cold_start(const char* l_ip, const char* d_ip)
{
    uint32_t oid[] = {1,3,6,1,6,3,1,1,5,1};
    snmp_send_v2(l_ip, d_ip, oid, 10, NULL, 0, 0);
}

void send_snmp_trap_link_up(const char* l_ip, const char* d_ip)
{
    uint32_t oid[] = {1,3,6,1,6,3,1,1,5,4};
    snmp_send_v2(l_ip, d_ip, oid, 10, NULL, 0, 0);
}

void send_snmp_trap_warm_start(const char* l_ip, const char* d_ip)
{
    uint32_t oid[] = {1,3,6,1,6,3,1,1,5,2};
    snmp_send_v2(l_ip, d_ip, oid, 10, NULL, 0, 0);
}

void send_snmp_trap_link_down(const char* l_ip, const char* d_ip)
{
    uint32_t oid[] = {1,3,6,1,6,3,1,1,5,3};
    snmp_send_v2(l_ip, d_ip, oid, 10, NULL, 0, 0);
}

void snmp_trap_auth_failure(const char* l_ip, const char* d_ip)
{
    uint32_t oid[] = {1,3,6,1,6,3,1,1,5,5};
    snmp_send_v2(l_ip, d_ip, oid, 10, NULL, 0, 0);
}

void snmp_trap_enterprise(const char* l_ip, const char* d_ip, const char* msg)
{
    uint32_t ent_oid[] = {1,3,6,1,4,1,999,1};
    uint32_t msg_oid[] = {1,3,6,1,4,1,999,1,1};
    ExtraVarbind ex = { .oid = msg_oid, .oid_len = 9, .value = (char*)msg };
    snmp_send_v2(l_ip, d_ip, ent_oid, 8, &ex, 1, 1);
}
