#ifndef __WIFI_H
#define __WIFI_H


#define DEBUG_INFO_UART0	true//

typedef enum
{
	WIFI_NONE,
	WIFI_NO_WIFI,
	WIFI_NORMAL,
	WIFI_CONNECTED,
	WIFI_DISCONNECTED,
	WIFI_NO_CONNECT,
	WIFI_SSID_FAIL,
}WIFI_STATUS_ENUM;


typedef struct
{
	uint8_t MANUEL_EN;
	uint8_t IP_Auto_Manual; //  0 Auto DHCP   1 static IP
	uint16_t modbus_port;
	uint16_t bacnet_port;
	uint8_t IP_Wifi_Status;  // 0 no-Wifi
	uint8_t rev;
	uint8_t	reserved[2];

	char name[64];
	char password[32];
	uint8_t ip_addr[4];
	uint8_t net_mask[4];
	uint8_t getway[4];
	uint8_t mac_addr[6];  // read-only
}STR_SSID;

extern bool re_init_wifi;

extern STR_SSID	SSID_Info;
extern void wifi_init_sta();
extern void debug_info(char *string);
extern void wifi_task(void *pvParameters);
extern void connect_wifi(void);

#endif
