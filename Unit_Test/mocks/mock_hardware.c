#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "esp_event.h"
#include "bacnet.h"
#include "cov.h"
#include "rtc.h"
#include "dlmstp.h"
#include "handlers.h"
#include "user_data.h"
#include "scan.h"
#include "wifi.h"
#include "sht3x.h"
#include "driver/spi_master.h"

// Mock BACnet Send UCOV Notify
int Send_UCOV_Notify(uint8_t * buffer, BACNET_COV_DATA * cov_data, uint8_t protocal)
{
    (void)buffer; (void)cov_data; (void)protocal;
    return 0;
}

// Mock UDP client send
void udp_client_send(uint16_t time)
{
    (void)time;
}

// Mock sensirion_i2c_write
esp_err_t sensirion_i2c_write(uint8_t address, const uint8_t *data, uint16_t count)
{
    (void)address; (void)data; (void)count;
    return ESP_OK;
}

// Mock sensirion_i2c_read
int16_t sensirion_i2c_read(uint8_t address, uint8_t *data, uint16_t count)
{
    (void)address;
    for (uint16_t i = 0; i < count; i++) {
        data[i] = 0;
    }
    return 0;
}

void esp_restart(void) {}
void sensirion_sleep_usec(uint32_t useconds) { (void)useconds; }

// Stubs for modbus.c linking
void Count_com_config(void) {}
void debug_info(char *string) { (void)string; }

void handler_private_transfer(
    uint8_t * service_request,
    unsigned int service_len,
    BACNET_ADDRESS * src,
    uint8_t protocal)
{
    (void)service_request; (void)service_len; (void)src; (void)protocal;
}

void Timer_Silence_Reset(void) {}
uint16_t read_airlab_by_block(uint16_t addr) { (void)addr; return 0; }
uint16_t read_lightswitch_by_block(uint16_t addr) { (void)addr; return 0; }
uint16_t read_co2_by_block(uint16_t addr) { (void)addr; return 0; }
uint16_t wireguard_read_by_block(uint16_t addr) { (void)addr; return 0; }

Str_points_ptr put_io_buf(Point_type_equate type, uint8 point)
{
    (void)type; (void)point;
    Str_points_ptr p = {0};
    return p;
}

void check_output_priority_array(U8_T i, U8_T HOA) { (void)i; (void)HOA; }
void save_block(void) {}
void esp_retboot(void) {}
void save_wifi_info(void) {}
void save_uint8_to_flash(const char* key, uint8_t value) { (void)key; (void)value; }
void Save_Lcd_config(void) {}

void write_airlab_by_block(uint16_t addr, uint8_t HeadLen, uint8_t *pData) { (void)addr; (void)HeadLen; (void)pData; }
void write_lightswitch_by_block(uint16_t addr, uint8_t HeadLen, uint8_t *pData) { (void)addr; (void)HeadLen; (void)pData; }
void wireguard_write_by_block(uint16_t addr, uint8_t HeadLen, uint8_t *pData) { (void)addr; (void)HeadLen; (void)pData; }
void save_point_info(void) {}
void write_co2_by_block(uint16_t addr, uint8_t HeadLen, uint8_t *pData) { (void)addr; (void)HeadLen; (void)pData; }

extern uint32_t Object_Instance_Number;
bool Device_Set_Object_Instance_Number(uint32_t object_id) {
    Object_Instance_Number = object_id;
    return true;
}
bool dlmstp_init(char *ifname) { (void)ifname; return true; }
void sntp_select_time_server(uint8_t type) { (void)type; }
void save_uint16_to_flash(const char* key, uint16_t value) { (void)key; (void)value; }
void Recievebuf_Initialize(void) {}
void set_default_parameters(void) {}
void clear_scan_db(void) {}
void LcdThemeMarkForUpdate(void) {}
void change_panel_number_in_code(U8_T old, U8_T new_panel) { (void)old; (void)new_panel; }
void start_fw_update(void) {}
int PCF_SetDateTime(PCF_DateTime *dt) { (void)dt; return 0; }
void update_timers(void) {}
BACNET_BINARY_PV Binary_Output_Present_Value(uint32_t instance) { (void)instance; return (BACNET_BINARY_PV)0; }

void push_expansion_out_stack(Str_out_point* ptr, uint8_t point, uint8_t type) { (void)ptr; (void)point; (void)type; }
float Analog_Output_Present_Value(uint32_t instance) { (void)instance; return 0.0f; }
void Set_AO_raw(uint8_t point, uint16_t value) { (void)point; (void)value; }
void push_expansion_in_stack(Str_in_point* ptr) { (void)ptr; }
void Set_Object_Name(char *name) { (void)name; }
void Sync_timestamp(uint32_t time) { (void)time; }
void Save_SNTP_sever(char *server) { (void)server; }
void Calculate_DSL_Time(void) {}

void Save_SPD_CNT(void) {}
int cov_subscribe_encode_apdu(uint8_t *apdu, uint8_t invoke_id, BACNET_SUBSCRIBE_COV_DATA *cov_data) { (void)apdu; (void)invoke_id; (void)cov_data; return 0; }
void handler_cov_subscribe(uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src, BACNET_CONFIRMED_SERVICE_DATA *service_data, uint8_t protocal) { (void)service_request; (void)service_len; (void)src; (void)service_data; (void)protocal; }
const char *bactext_object_type_name(int type) {
    if (type == 0) return "Analog Input";
    if (type == 3) return "Binary Input";
    return "Unknown";
}
void uart_send_string(U8_T *p, U16_T length, U8_T port) { (void)p; (void)length; (void)port; }
void delay_ms(uint32_t ms) { (void)ms; }

// Missing globals and stubs for link
uint8_t flag_Updata_Clock;
uint8_t mtu[1500];

uint32_t bip_get_addr(void) { return 0; }
uint16_t bip_get_port(void) { return 0; }
uint32_t bip_get_broadcast_addr(void) { return 0; }
int bip_socket(void) { return -1; }

void Device_getCurrentDateTime(void *DateTime) { (void)DateTime; }
int32_t Device_UTC_Offset(void) { return 0; }
bool Device_Daylight_Savings_Status(void) { return false; }

// spi handle stub
spi_device_handle_t hy3131_handle;

// SHT3X stubs
etError SHT3X_Read2BytesAndCrc(uint16 *data, etI2cAck finaleAck, uint8 timeout) {
    (void)data; (void)finaleAck; (void)timeout;
    return NO_ERROR;
}
etError SHT3X_WriteCommand(etCommands cmd) {
    (void)cmd;
    return NO_ERROR;
}
void SHT3X_StopAccess(void) {}
etError SHT3X_StartWriteAccess(void) { return NO_ERROR; }
etError SHT3X_StartReadAccess(void) { return NO_ERROR; }

// send / recv stubs
int send(int s, const void *dataptr, size_t size, int flags) { (void)s; (void)dataptr; (void)size; (void)flags; return 0; }
int recv(int s, void *mem, size_t len, int flags) { (void)s; (void)mem; (void)len; (void)flags; return 0; }

const char *SGP_DRV_VERSION_STR = "mock";

