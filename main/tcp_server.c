/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "esp_ota_ops.h"
//#include "nvs_flash.h"
#include "tcpip_adapter.h"
//#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <store.h>


#include "define.h"
#include "modbus.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "wifi.h"
#include "ethernet_task.h"
#include "flash.h"
#include "rtc.h"

#include "i2c_task.h"
//#include "microphone.h"
//#include "pyq1548.h"
#include "led_pwm.h"
//#include "ble_mesh.h"
//#include "ud_str.h"
//#include "controls.h"
#include "bacnet.h"
#include "ud_str.h"
#include "user_data.h"
#include "controls.h"
#include "commsub.h"
#include "scan.h"

//#include "point.h""
#include "rs485.h"
#include "fifo.h"
#include "freertos/event_groups.h"
#include "airlab.h"
#include "mppt_task.h"
#include "lwip/dns.h"
#include "sntp_app.h"
#include "multiMeter.h"
#include "mm_spi.h"
#include "co2.h"
#include "lowPower.h"

//#include "types.h"


#define PORT CONFIG_EXAMPLE_PORT

#define S_ALL_NEW  0x15
#define G_ALL_NEW  0x25

xTaskHandle main_task_handle[20];
extern xTaskHandle Handle_Scan;
char debug_array[100];
//static const char *TAG = "Example";
static const char *TCP_TASK_TAG = "TCP_TASK";
static const char *UDP_TASK_TAG = "UDP_TASK";
void udp_client_send(uint16 time);
STR_SCAN_CMD Scan_Infor;
//uint8_t tcp_send_packet[1024];
//uint8_t modbus_wifi_buf[500];
//uint16_t modbus_wifi_len;
uint8_t reg_num;
/*static bool isSocketCreated = false;
extern double ambient;
extern double object;
extern float mlx90614_ambient;
extern float mlx90614_object;*/
uint32_t Instance;
uint16_t flag_ethernet_initial = 0;
extern uint8_t Eth_IP_Change;
extern uint8_t ip_change_count;
extern uint8_t count_reboot;

uint16_t crc16(uint8_t *p, uint8_t length);
STR_Task_Test task_test;

extern uint8_t gIdentify;
extern uint8_t count_gIdentify;
extern U8_T max_dos;
extern U8_T max_aos;
//extern U16_T qKey;
extern uint32_t ether_rx;

uint8_t flag_clear_count_reboot;
void uart0_rx_task(void);
void uart2_rx_task(void);
void Scan_network_bacnet_Task(void);
void bip_client_Task(void);
extern uint8_t bip_client_send_buf[MAX_MPDU_IP];
extern int bip_client_send_len;
extern U8_T Send_bip_address[6];
void Test_Array(void);

uint8_t uart0_config;  // no used
//void modbus0_task(void *arg);
//void modbus2_task(void *arg);
uint16_t input_cal[16];

void Bacnet_Control(void);


void Lcd_task(void *arg);
void LCD_TEST(void);
void vPM25Task(void *pvParameters);
void vInputTask(void *pvParameters);
void lightswitch_adc_init(void);
void Light_Switch_IO_Init(void);
//閿熸枻鎷烽敓鏂ゆ嫹閿熻剼鐚存嫹閿熸枻鎷烽敓鏂ゆ嫹閿燂拷 閿熻剼鐚存嫹閿熸枻鎷烽敓鏂ゆ嫹閿燂拷 閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷峰�奸敓鏂ゆ嫹閿熸枻鎷峰閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷峰�� 閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓鑴氱尨鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓鏂ゆ嫹閿熻鍑ゆ嫹閿熸枻鎷烽敓鏂ゆ嫹婧愰敓鏂ゆ嫹閿熺煫鈽呮嫹閿熸枻鎷�
//MAX_COUNT 閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷峰睉閿熸枻鎷烽敓鏂ゆ嫹閿熺殕锟�
xSemaphoreHandle CountHandle;

uint8_t flag_suspend_scan = 0;
uint8_t	suspend_scan_count = 0;
uint8_t TemcoVars;

U8_T BIP_src_addr[6];

#define INIT_SOC_COUNT	7

#define MAX_SOC_COUNT	7
int my_listen_sock;
EXT_RAM_ATTR char addr_str[128];
struct sockinfo{
	int 		sock;
	sa_family_t	sa_familyType;
	char  		remoteIp[32];
	u16_t		remotePort;
};
static void tcp_server_dealwith0(void *args);
static void tcp_server_dealwith1(void *args);
static void tcp_server_dealwith2(void *args);
static void tcp_server_dealwith3(void *args);
static void tcp_server_dealwith4(void *args);
static void tcp_server_dealwith5(void *args);
static void tcp_server_dealwith6(void *args);
TaskFunction_t taskList[MAX_SOC_COUNT] = {tcp_server_dealwith0,
		                                  tcp_server_dealwith1,
										  tcp_server_dealwith2,
										  tcp_server_dealwith3,
										  tcp_server_dealwith4,
										  tcp_server_dealwith5,
										  tcp_server_dealwith6};
TaskHandle_t Task_handle[MAX_SOC_COUNT] = {0};
//WIFI閿熼摪纭锋嫹閿熸枻鎷峰織閿熸枻鎷� 涔熼敓鏂ゆ嫹閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓鍙唻鎷峰織閿熶粙锛堥敓鏂ゆ嫹閿熶粖琚揪鎷烽敓鏂ゆ嫹閿熸枻鎷穝et 閿熸枻鎷峰垹閿熸枻鎷烽敓鏂ゆ嫹clean 閿熸磥褰撻敓鏂ゆ嫹閿熸枻鎷锋簮閿熻棄鍗曢敓鏂ゆ嫹浜涢敓鏂ゆ嫹婧愰敓瑙掑尅鎷烽敓鐭殑锝忔嫹
EventGroupHandle_t network_EventHandle = NULL;
const int CONNECTED_BIT = BIT0;
const int TASK1_BIT		= BIT1;
const int TASK2_BIT		= BIT2;
const int TASK3_BIT		= BIT3;
const int TASK4_BIT		= BIT4;
const int TASK5_BIT		= BIT5;
const int TASK6_BIT		= BIT6;
const int TASK7_BIT		= BIT7;
int task_sock[MAX_SOC_COUNT] = {0};
void ENALBE_LSW_Ethernet(void);
void Save_SPD_CNT(void);
void start_fw_update(void)
{
   const esp_partition_t *factory = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);

//   save_point_info(0);
   Save_SPD_CNT();

   esp_ota_set_boot_partition(factory);
#if LSW_ON_OFF
   if(Modbus.mini_type == PROJECT_LSW_SENSOR)
	   ENALBE_LSW_Ethernet();
#endif
   esp_retboot();
}


void esp_retboot(void)
{

   esp_restart();
}

void UdpData(unsigned char type)
{
   // header 2 bytes
   memset(&Scan_Infor,0,sizeof(STR_SCAN_CMD));
   if(type == 0)
      Scan_Infor.cmd = 0x0065;
   else if(type == 1)
      Scan_Infor.cmd = 0x0067;

   Scan_Infor.len = 0x001d;

   //serialnumber 4 bytes
   Scan_Infor.own_sn[0] = (unsigned short int)Modbus.serialNum[0];
   Scan_Infor.own_sn[1] = (unsigned short int)Modbus.serialNum[1];
   Scan_Infor.own_sn[2] = (unsigned short int)Modbus.serialNum[2];
   Scan_Infor.own_sn[3] = (unsigned short int)Modbus.serialNum[3];

   Scan_Infor.product = Modbus.product_model&0xff;//PM_TSTAT_AQ;//PRODUCT_MINI_ARM;  // only for test now

   //modbus address
   Scan_Infor.address = Modbus.address;//laddress;//(unsigned short int)Modbus.address;

   //Ip
   if(Modbus.ethernet_status == 4)
   {
      Scan_Infor.ipaddr[0] = Modbus.ip_addr[0];//(unsigned short int)SSID_Info.ip_addr[0];
      Scan_Infor.ipaddr[1] = Modbus.ip_addr[1];//(unsigned short int)SSID_Info.ip_addr[1];
      Scan_Infor.ipaddr[2] = Modbus.ip_addr[2];//(unsigned short int)SSID_Info.ip_addr[2];
      Scan_Infor.ipaddr[3] = Modbus.ip_addr[3];//(unsigned short int)SSID_Info.ip_addr[3];
   }
   else
   {
      Scan_Infor.ipaddr[0] = SSID_Info.ip_addr[0];
      Scan_Infor.ipaddr[1] = SSID_Info.ip_addr[1];
      Scan_Infor.ipaddr[2] = SSID_Info.ip_addr[2];
      Scan_Infor.ipaddr[3] = SSID_Info.ip_addr[3];
   }

   //port
   Scan_Infor.modbus_port = Modbus.tcp_port;//502;//SSID_Info.modbus_port;  //tbd :????????????????

   // software rev
   Scan_Infor.firmwarerev = SW_REV;
   // hardware rev
   Scan_Infor.hardwarerev = Modbus.hardRev;//HardwareVersion;

   Scan_Infor.instance_low = htons(Instance); // hight byte first
   Scan_Infor.panel_number = Modbus.address;//laddress; //  36
   Scan_Infor.instance_hi = htons(Instance >> 16); // hight byte first

   Scan_Infor.bootloader = 0;  // 0 - app, 1 - bootloader, 2 - wrong bootloader

   Scan_Infor.BAC_port = 47808;//SSID_Info.bacnet_port;//((Modbus.Bip_port & 0x00ff) << 8) + (Modbus.Bip_port >> 8);  //
   Scan_Infor.zigbee_exist = 0; // 0 - inexsit, 1 - exist
   Scan_Infor.subnet_protocal = 0;
   Scan_Infor.master_sn[0] = 0;
   Scan_Infor.master_sn[1] = 0;
   Scan_Infor.master_sn[2] = 0;
   Scan_Infor.master_sn[3] = 0;
   //if(Modbus.product_model == 88)
   	   memcpy(Scan_Infor.panelname,panelname,20);
   //else
   	   //memcpy(Scan_Infor.panelname,(char*)"AirLab-esp32",12);
   Scan_Infor.command_version = 1;
   Scan_Infor.mini_type = Modbus.mini_type;
//   state = 1;
//   scanstart = 0;

}


uint32_t get_ip_addr(void)
{
	if(Modbus.ethernet_status == 4) // wifi is disconnected
	{
		return ((uint32_t)Modbus.ip_addr[3] << 24) + ((uint32_t)Modbus.ip_addr[2] << 16) + ((uint16_t)Modbus.ip_addr[1] << 8) + Modbus.ip_addr[0];
	}
	else
	{
		return ((uint32_t)SSID_Info.ip_addr[3] << 24) + ((uint32_t)SSID_Info.ip_addr[2] << 16) + ((uint16_t)SSID_Info.ip_addr[1] << 8) + SSID_Info.ip_addr[0];
	}
}

EXT_RAM_ATTR uint8_t PDUBuffer_BIP[MAX_APDU];
uint16_t  bip_len;
uint8_t * bip_Data;
extern uint8_t far bip_send_buf[MAX_MPDU_IP];
extern int bip_send_len;

uint16_t Get_bip_len(void)
{
	return bip_len;
}

int bip_sock;
struct sockaddr_in6 bip_source_addr; // Large enough for both IPv4 or IPv6

void Send_MSTP_to_BIPsocket(uint8_t * buf,uint16_t len)
{
	// Send_SUB_I_Am 閿熸枻鎷烽敓绔府鎷稭STP閿熸枻鎷疯浆閿熸埅纰夋嫹閿熸枻鎷烽敓鏂ゆ嫹
	if(len > 0)
	{
		sendto(bip_sock, (uint8_t *)buf, len, 0, (struct sockaddr *)&bip_source_addr, sizeof(bip_source_addr));
		len = 0;
	}

}
#if 0
char udp_debug_str[100] = "udp test";
uint8_t flag_debug_rx = 1;
uint16_t debug_rx_len = 100;

static void udp_debug_task(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    uint8_t ip;
    flag_debug_rx = 1;
    memcpy(&udp_debug_str,(char *)"udp test",9); debug_rx_len = 100;
    while (1) {

#ifdef CONFIG_EXAMPLE_IPV4
        struct sockaddr_in dest_addr;
        ip = 145;
        dest_addr.sin_addr.s_addr =	((uint32_t)ip << 24) + ((uint32_t)Modbus.ip_addr[2] << 16) + \
        							((uint16_t)Modbus.ip_addr[1] << 8) + Modbus.ip_addr[0];
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(DEBUG_PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
        struct sockaddr_in6 dest_addr;
        inet6_aton(HOST_IP_ADDR, &dest_addr.sin6_addr);
        dest_addr.sin6_family = AF_INET6;
        dest_addr.sin6_port = htons(PORT);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
        inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {//debug_info("Unable to create socket");
            //ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        //ESP_LOGI(TAG, "Socket created, sending to %s:%d", HOST_IP_ADDR, PORT);
        //debug_info("create socket");
        while (1) {
        	if(flag_debug_rx == 1)
        	{
				int err = sendto(sock, udp_debug_str, debug_rx_len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
				if (err < 0) {//debug_info("Error occurred");
					//ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
					break;
				}
				else
				{
					flag_debug_rx = 0;
				}

        	}
        	/*struct sockaddr_in source_addr; // Large enough for both IPv4 or IPv6
			socklen_t socklen = sizeof(source_addr);
			int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

			// Error occurred during receiving
			if (len < 0) {Test[3] = 6;
			   // ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
				break;
			}
			// Data received
			else {Test[3] = 7;
				rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
				//ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
				//ESP_LOGI(TAG, "%s", rx_buffer);
			}
*/
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }

        if (sock != -1) {
            //ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }

    }
    vTaskDelete(NULL);
}

#endif

static void bip_task(void *pvParameters)
{
   // char rx_buffer[600];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    uint16_t pdu_len = 0;
    BACNET_ADDRESS far src; /* source address */
    bip_set_socket(47808);
    task_test.enable[5] = 1;
    while (1) {task_test.count[5]++;
       //if(SSID_Info.IP_Wifi_Status == WIFI_CONNECTED)
       {
   #ifdef CONFIG_EXAMPLE_IPV4
    	struct sockaddr_in dest_addr;
         dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
         dest_addr.sin_family = AF_INET;
         dest_addr.sin_port = htons(47808);
         addr_family = AF_INET;
         ip_protocol = IPPROTO_IP;
         inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
   #else // IPV6
         struct sockaddr_in6 dest_addr;
         bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
         dest_addr.sin6_family = AF_INET6;
         dest_addr.sin6_port = htons(PORT);
         addr_family = AF_INET6;
         ip_protocol = IPPROTO_IPV6;
         inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
   #endif

         bip_sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
         if (bip_sock < 0) {
            //ESP_LOGE(UDP_TASK_TAG, "Unable to create socket: errno %d", errno);
            break;
         }
         ESP_LOGI(UDP_TASK_TAG, "Socket created");



         int err = bind(bip_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
         if (err < 0) {
            //ESP_LOGE(UDP_TASK_TAG, "Socket unable to bind: errno %d", errno);
         }
         ESP_LOGI(UDP_TASK_TAG, "Socket bound, port %d", PORT);

         while (1) {

           // ESP_LOGI(UDP_TASK_TAG, "Waiting for data");
            //struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(bip_source_addr);
            task_test.count[5]++;
            int len = recvfrom(bip_sock, PDUBuffer_BIP, sizeof(PDUBuffer_BIP) - 1, 0, (struct sockaddr *)&bip_source_addr, &socklen);

            bip_len = len;

            flagLED_ether_rx = 1;
            bip_Data = PDUBuffer_BIP;
            // Error occurred during receiving
            if (len < 0) {
              // ESP_LOGE(UDP_TASK_TAG, "recvfrom failed: errno %d", errno);
               break;
            }
            // Data received
            else
            {
            	ether_rx += len;
               // Get the sender's ip address as string
               if (bip_source_addr.sin6_family == PF_INET) {
                  inet_ntoa_r(((struct sockaddr_in *)&bip_source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                 // ESP_LOGI(UDP_TASK_TAG, "IPV4 receive data");
               } else if (bip_source_addr.sin6_family == PF_INET6) {
                  inet6_ntoa_r(bip_source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                 // ESP_LOGI(UDP_TASK_TAG, "IPV6 receive data");
               }

               //memcpy(&BIP_src_addr[0],&bip_source_addr.sin6_flowinfo,4);
               //memcpy(&BIP_src_addr[4],&bip_source_addr.sin6_port,2);

               pdu_len = datalink_receive(&src, &PDUBuffer_BIP[0], sizeof(PDUBuffer_BIP), 0,BAC_IP);
               {
					if(pdu_len)
					{
						npdu_handler(&src, &PDUBuffer_BIP[0], pdu_len, BAC_IP);
						if(bip_send_len > 0)
						{
							sendto(bip_sock, (uint8_t *)&bip_send_buf, bip_send_len, 0, (struct sockaddr *)&bip_source_addr, sizeof(bip_source_addr));

							bip_send_len = 0;
							memset(bip_send_buf,0,MAX_MPDU_IP);
						}
					}
				}

             }

         }

         if (bip_sock != -1) {
           // ESP_LOGE(UDP_TASK_TAG, "Shutting down socket and restarting...");
            shutdown(bip_sock, 0);
            close(bip_sock);
         }
      }
      vTaskDelete(NULL);
    }
}

uint32 multicast_addr;
uint32 Get_multicast_addr(uint8 *ip_addr)
{
	U8_T temp[4];
	temp[0] = ip_addr[0];
	temp[1] = ip_addr[1];
	temp[2] = ip_addr[2];
	temp[3] = ip_addr[3];

	if(Modbus.ethernet_status == 4)
	{
		temp[0] |= (255 - Modbus.subnet[0]);
		temp[1] |= (255 - Modbus.subnet[1]);
		temp[2] |= (255 - Modbus.subnet[2]);
		temp[3] |= (255 - Modbus.subnet[3]);
	}
	else
	{
		temp[0] |= (255 - SSID_Info.net_mask[0]);
		temp[1] |= (255 - SSID_Info.net_mask[1]);
		temp[2] |= (255 - SSID_Info.net_mask[2]);
		temp[3] |= (255 - SSID_Info.net_mask[3]);
	}


	//uip_ipaddr(uip_hostaddr_submask,temp[0], temp[1],temp[2] ,temp[3]);

	return (temp[3] + (U16_T)(temp[2] << 8) \
		+ ((U32_T)temp[1] << 16) + ((U32_T)temp[0] << 24));
}
uint8_t flag_boardcast;
static void udp_scan_task(void *pvParameters)
{
    char rx_buffer[600];
    char addr_str[128];
	struct sockaddr_in sDestAddr;
    int addr_family;
    int ip_protocol;
    task_test.enable[4] = 1;

    while (1) {task_test.count[4]++;
       //if(SSID_Info.IP_Wifi_Status == WIFI_CONNECTED)
       {
   #ifdef CONFIG_EXAMPLE_IPV4
         struct sockaddr_in dest_addr;
         dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
         dest_addr.sin_family = AF_INET;
         dest_addr.sin_port = htons(1234);
         addr_family = AF_INET;
         ip_protocol = IPPROTO_IP;
         inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
   #else // IPV6
         struct sockaddr_in6 dest_addr;
         bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
         dest_addr.sin6_family = AF_INET6;
         dest_addr.sin6_port = htons(PORT);
         addr_family = AF_INET6;
         ip_protocol = IPPROTO_IPV6;
         inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
   #endif

         int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
         if (sock < 0) {
            //ESP_LOGE(UDP_TASK_TAG, "Unable to create socket: errno %d", errno);
            break;
         }
         //ESP_LOGI(UDP_TASK_TAG, "Socket created");

         int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
         if (err < 0) {
            //ESP_LOGE(UDP_TASK_TAG, "Socket unable to bind: errno %d", errno);
         }
         //ESP_LOGI(UDP_TASK_TAG, "Socket bound, port %d", PORT);

         while (1) {

            //ESP_LOGI(UDP_TASK_TAG, "Waiting for data");
            struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            struct sockaddr tempaddr;
            uint8_t src_ip_byte[4];
            uint32 src_multicast_addr;
            uint16 src_port = 0;
            task_test.count[4]++;
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

			memcpy(&tempaddr,&source_addr, socklen);
			memcpy(&src_ip_byte,&tempaddr.sa_data[2], 4);
			src_multicast_addr = Get_multicast_addr(src_ip_byte);

			if(src_multicast_addr != multicast_addr)
			{ // if in differnet subnet, response broadcast, only for temco scan port 1234
				//uip_ipaddr_copy(uip_udp_conn->ripaddr, all_ones_addr);
				flag_boardcast = 1;
				memcpy(&src_port,&tempaddr.sa_data[0], 2);
				src_port = htons(src_port);
			}
			else
				flag_boardcast = 0;

            flagLED_ether_rx = 1;
            // Error occurred during receiving
            if (len < 0) {//debug_info("udp1234 recv error\r\n");
               //ESP_LOGE(UDP_TASK_TAG, "recvfrom failed: errno %d", errno);
               break;
            }
            // Data received
            else 
            {//debug_info("udp1234 recv ok\r\n");
            	ether_rx += len;
               // Get the sender's ip address as string
               if (source_addr.sin6_family == PF_INET) {
            	   if(flag_boardcast == 1)
				 {
					sDestAddr.sin_family = AF_INET;
					sDestAddr.sin_len = sizeof(sDestAddr);
					sDestAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
					sDestAddr.sin_port = htons(src_port);
				 }
            	 else
            		 inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                  //ESP_LOGI(UDP_TASK_TAG, "IPV4 receive data");
               } else if (source_addr.sin6_family == PF_INET6) {
                  inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                  //ESP_LOGI(UDP_TASK_TAG, "IPV6 receive data");
               }

               //rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
              // ESP_LOGI(UDP_TASK_TAG, "Received %d bytes from %s:", len, addr_str);
              // ESP_LOG_BUFFER_HEX(UDP_TASK_TAG, rx_buffer, len);
               if(rx_buffer[0] == 0x64)
               {
                  UdpData(0);
                  if(flag_boardcast == 1)
                  {
					  sendto(sock, (uint8_t *)&Scan_Infor, sizeof(STR_SCAN_CMD), 0,  (struct sockaddr *)&sDestAddr, sizeof(sDestAddr));
                  }
                  else
                  {
                  //ESP_LOGI(UDP_TASK_TAG, "receive data buffer[0] = 0x64");
                	  sendto(sock, (uint8_t *)&Scan_Infor, sizeof(STR_SCAN_CMD), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
                  }


  				//serialnumber 4 bytes
  				Scan_Infor.master_sn[0] = 0;
  				Scan_Infor.master_sn[1] = 0;
  				Scan_Infor.master_sn[2] = 0;
  				Scan_Infor.master_sn[3] = 0;

  			// for MODBUS device
  				for(u8 i = 0;i < sub_no;i++)
  				{
  					if((scan_db[i].product_model >= CUSTOMER_PRODUCT) || (current_online[scan_db[i].id / 8] & (1 << (scan_db[i].id % 8))))	  	 // in database but not on_line
  					{
  						if(scan_db[i].product_model != 74)
  						{
  						if(scan_db[i].sn !=
  							Modbus.serialNum[0] + (U16_T)(Modbus.serialNum[1] << 8)	+ ((U32_T)Modbus.serialNum[2] << 16) + ((U32_T)Modbus.serialNum[3] << 24))
  							{

  								Scan_Infor.own_sn[0] = (unsigned short int)scan_db[i].sn & 0x00ff;
  								Scan_Infor.own_sn[1] = (unsigned short int)(scan_db[i].sn >> 8) & 0x00ff;
  								Scan_Infor.own_sn[2] = (unsigned short int)(scan_db[i].sn >> 16) & 0x00ff;
  								Scan_Infor.own_sn[3] = (unsigned short int)(scan_db[i].sn >> 24) & 0x00ff;

  								Scan_Infor.product = (unsigned short int)scan_db[i].product_model & 0x00ff;
  								Scan_Infor.address = (unsigned short int)scan_db[i].id & 0x00ff;

  								Scan_Infor.instance_low = 0;
  								Scan_Infor.instance_hi = 0;
  								Scan_Infor.subnet_protocal = 1;  // modbus device

  								Scan_Infor.master_sn[0] = Modbus.serialNum[0];
  								Scan_Infor.master_sn[1] = Modbus.serialNum[1];
  								Scan_Infor.master_sn[2] = Modbus.serialNum[2];
  								Scan_Infor.master_sn[3] = Modbus.serialNum[3];

  								Scan_Infor.subnet_port = (scan_db[i].port & 0x0f) - 1;
  								Scan_Infor.subnet_baudrate = ((scan_db[i].port & 0xf0) >> 4);

  								memcpy(&Scan_Infor.panelname,tstat_name[i],20);

  								int err = sendto(sock, (uint8_t *)&Scan_Infor, sizeof(STR_SCAN_CMD), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
								  if (err < 0) {
									 ESP_LOGE(UDP_TASK_TAG, "Error occurred during sending: errno %d", errno);
									 break;
								  }

  							}
  						}
  					}
  				}
  				// for MSTP device
					for(u8 i = 0;i < remote_panel_num;i++)
					{
						if((remote_panel_db[i].protocal == BAC_MSTP) && (remote_panel_db[i].sn != 0))
						{
	//						BACNET_ADDRESS dest = { 0 };
	//						uint16 max_apdu = 0;
	//						bool status = false;

							/* is the device bound? */
							//status = address_get_by_device(remote_panel_db[i].device_id, &max_apdu, &dest);

	//						if(status > 0)
							{
								char temp_name[20];

								Scan_Infor.own_sn[0] = (U16_T)remote_panel_db[i].sn;
								Scan_Infor.own_sn[1] = (U16_T)(remote_panel_db[i].sn >> 8);
								Scan_Infor.own_sn[2] = (U16_T)(remote_panel_db[i].sn >> 16);
								Scan_Infor.own_sn[3] = (U16_T)(remote_panel_db[i].sn >> 24);


								Scan_Infor.product = remote_panel_db[i].product_model;
								Scan_Infor.address = remote_panel_db[i].sub_id;

								Scan_Infor.master_sn[0] = Modbus.serialNum[0];
								Scan_Infor.master_sn[1] = Modbus.serialNum[1];
								Scan_Infor.master_sn[2] = Modbus.serialNum[2];
								Scan_Infor.master_sn[3] = Modbus.serialNum[3];


								Scan_Infor.instance_low = htons(remote_panel_db[i].device_id); // hight byte first
								Scan_Infor.panel_number = remote_panel_db[i].sub_id;
								Scan_Infor.instance_hi = htons(remote_panel_db[i].device_id >> 16); // hight byte first

								memset(temp_name,0,20);
	//							strcmp(temp_name, "panel:"/*, remote_panel_db[i].sub_id*/);
								temp_name[0] = 'M';
								temp_name[1] = 'S';
								temp_name[2] = 'T';
								temp_name[3] = 'P';
								temp_name[4] = ':';
	//							temp_name[5] = Scan_Infor.station_num / 10 + '0';
	//							temp_name[6] = Scan_Infor.station_num % 10 + '0';

								temp_name[19] = '\0';
								memcpy(&Scan_Infor.panelname,temp_name,20);

								Scan_Infor.subnet_protocal = 12;  // MSTP device

								Scan_Infor.subnet_port = get_current_mstp_port();
								if(Scan_Infor.subnet_port == 0)
									Scan_Infor.subnet_baudrate = Modbus.baudrate[0];
								else if(Scan_Infor.subnet_port == 2)
									Scan_Infor.subnet_baudrate = Modbus.baudrate[2];
								else
									Scan_Infor.subnet_baudrate = 0;

								int err = sendto(sock, (uint8_t *)&Scan_Infor, sizeof(STR_SCAN_CMD), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
								if (err < 0) {
									//ESP_LOGE(UDP_TASK_TAG, "Error occurred during sending: errno %d", errno);
									break;
								}
							}
						}
					}
               }
               else if((rx_buffer[0] == 0x66) && (rx_buffer[1] == Modbus.ip_addr[0]) && (rx_buffer[2] == Modbus.ip_addr[1]) && (rx_buffer[3] == Modbus.ip_addr[2]) && (rx_buffer[4] == Modbus.ip_addr[3]))
               {
            		 // cmd(1 byte) + changed ip(4 bytes) + new ip(4 bytes) + new subnet(4 bytes) + new getway(4)  --- old protocal
            		 // + sn(4 bytes)  -- new protocal, used to change conflict ip

            		 if(((rx_buffer[17] == Modbus.serialNum[0]) && (rx_buffer[18] == Modbus.serialNum[1]) && (rx_buffer[19] == Modbus.serialNum[2]) && (rx_buffer[20] == Modbus.serialNum[3])) \
            			 || ((rx_buffer[17] == 0) && (rx_buffer[18] == 0) && (rx_buffer[19] == 0) && (rx_buffer[20] == 0)))
            		 {
            				n = 5;
            				UdpData(1);

            			  if(flag_boardcast == 1)
						  {
							  sendto(sock, (uint8_t *)&Scan_Infor, sizeof(STR_SCAN_CMD), 0,  (struct sockaddr *)&sDestAddr, sizeof(sDestAddr));
						  }
						  else
            				sendto(sock, (uint8_t *)&Scan_Infor, sizeof(STR_SCAN_CMD), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));

            				Modbus.tcp_type = 0;

            				Modbus.ip_addr[0] = rx_buffer[n++];
            				Modbus.ip_addr[1] = rx_buffer[n++];
            				Modbus.ip_addr[2] = rx_buffer[n++];
            				Modbus.ip_addr[3] = rx_buffer[n++];

            			 	if(Modbus.com_config[2] == BACNET_SLAVE || Modbus.com_config[2] == BACNET_MASTER
            					|| Modbus.com_config[0] == BACNET_SLAVE || Modbus.com_config[0] == BACNET_MASTER)
            				{
            					Send_I_Am_Flag = 1;
            				}

            				Modbus.subnet[0] = rx_buffer[n++];
            				Modbus.subnet[1] = rx_buffer[n++];
            				Modbus.subnet[2] = rx_buffer[n++];
            				Modbus.subnet[3] = rx_buffer[n++];

            				Modbus.getway[0] = rx_buffer[n++];
            				Modbus.getway[1] = rx_buffer[n++];
            				Modbus.getway[2] = rx_buffer[n++];
            				Modbus.getway[3] = rx_buffer[n++];



            				if((Modbus.ip_addr[0] != 0)  && (Modbus.ip_addr[1] != 0)  && (Modbus.ip_addr[3] != 0) )
            				{

            					Modbus.tcp_type = 0;
            					save_uint8_to_flash( FLASH_TCP_TYPE,Modbus.tcp_type);

            					Eth_IP_Change = 1;
            					ip_change_count = 0;

            				}
            		}
               }
            }
         }

         if (sock != -1) {
           // ESP_LOGE(UDP_TASK_TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
         }
      }
      vTaskDelete(NULL);
    }
}


void Set_transaction_ID(U8_T *str, U16_T id, U16_T num)
{
	str[0] = (U8_T)(id >> 8);		//transaction id
	str[1] = (U8_T)id;

	str[2] = 0;						//protocol id, modbus protocol = 0
	str[3] = 0;

	str[4] = (U8_T)(num >> 8);
	str[5] = (U8_T)num;
}
//static uint8_t previousSock;
#define MAX_TCP_CONN  7
EXT_RAM_ATTR int sock[MAX_TCP_CONN];
EXT_RAM_ATTR int Sock_table[MAX_TCP_CONN];
//char len;
U8_T rx_buffer[MAX_TCP_CONN][512];
extern xSemaphoreHandle xSem_comport[3];

u16 modbus_send_len;
u8 modbus_send_buf[500];
int Modbus_Tcp(uint16_t len,int sock,U8_T* rx_buffer)
{
	memset(modbus_send_buf,0,500);
	modbus_send_len = 0;
	if (len == 5)
	{
		//ESP_LOGI(TCP_TASK_TAG, "Receive: %02x %02x %02x %02x %02x.", rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3], rx_buffer[4]);
	}

	//ESP_LOG_BUFFER_HEX(TCP_TASK_TAG, rx_buffer, len);

	{
		if( (rx_buffer[0] == 0xee) && (rx_buffer[1] == 0x10) &&
			(rx_buffer[2] == 0x00) && (rx_buffer[3] == 0x00) &&
			(rx_buffer[4] == 0x00) && (rx_buffer[5] == 0x00) &&
			(rx_buffer[6] == 0x00) && (rx_buffer[7] == 0x00) )
		{
			start_fw_update();
		}
	}

	if( (rx_buffer[6]== Modbus.address) || ((rx_buffer[6]==255) && (rx_buffer[7]!=0x19)))
	{
		responseModbusCmd(WIFI, (uint8_t *)rx_buffer, len ,modbus_send_buf,&modbus_send_len,0);
		if(modbus_send_len > 0)
		{
			int err = send(sock, (uint8_t *)&modbus_send_buf, modbus_send_len, 0);

			if (err < 0)
			{
				return -1;
				//ESP_LOGE(TCP_TASK_TAG, "Error occurred during sending: errno %d", errno);
				//break;
			}
			else
			flagLED_ether_tx = 1;
			return err;
		}
	}
	else
	{
		// transfer data to sub ,TCP TO RS485
		U8_T header[6];
		U8_T i;
		U16_T send_len;

		if((rx_buffer[UIP_HEAD] == 0x00) ||
		((rx_buffer[UIP_HEAD + 1] != READ_VARIABLES)
		&& (rx_buffer[UIP_HEAD + 1] != WRITE_VARIABLES)
		&& (rx_buffer[UIP_HEAD + 1] != MULTIPLE_WRITE)
		&& (rx_buffer[UIP_HEAD + 1] != CHECKONLINE)
		&& (rx_buffer[UIP_HEAD + 1] != READ_COIL)
		&& (rx_buffer[UIP_HEAD + 1] != READ_DIS_INPUT)
		&& (rx_buffer[UIP_HEAD + 1] != READ_INPUT)
		&& (rx_buffer[UIP_HEAD + 1] != WRITE_COIL)
		&& (rx_buffer[UIP_HEAD + 1] != WRITE_MULTI_COIL)
		&& (rx_buffer[UIP_HEAD + 1] != CHECKONLINE_WIHTCOM)
		&& (rx_buffer[UIP_HEAD + 1] != TEMCO_MODBUS)
		))
		{
			return 0;
		}
		if((rx_buffer[UIP_HEAD + 1] == MULTIPLE_WRITE) && ((len - UIP_HEAD) != (rx_buffer[UIP_HEAD + 6] + 7)))
		{
			return 0;
		}

		if(Modbus.com_config[0] == MODBUS_MASTER)
			Modbus.sub_port = 0;
		else if(Modbus.com_config[2] == MODBUS_MASTER)
			Modbus.sub_port = 2;
		else
		{
			return 0;
		}

		for(i = 0;i <  sub_no ;i++)
		{
			if(rx_buffer[UIP_HEAD] == uart0_sub_addr[i])
			{
				 Modbus.sub_port = 0;
				 continue;
			}
			else if(rx_buffer[UIP_HEAD] == uart2_sub_addr[i])
			{
				Modbus.sub_port = 2;
				continue;
			}
		}

		if((rx_buffer[UIP_HEAD + 1] == READ_DIS_INPUT) || (rx_buffer[UIP_HEAD + 1] == READ_COIL))
			send_len = (rx_buffer[UIP_HEAD + 5] + 7) / 8 + 3; // (buf[5] + 7) / 8 + 5;
		else if((rx_buffer[UIP_HEAD + 1] == READ_VARIABLES) || (rx_buffer[UIP_HEAD + 1] == READ_INPUT))
			send_len = rx_buffer[UIP_HEAD + 5] * 2 + 3;
		else
			send_len = 8;

		Set_transaction_ID(header, ((U16_T)rx_buffer[0] << 8) | rx_buffer[1],send_len);

		//vTaskSuspend(&main_task_handle[5]);

		flag_suspend_scan = 1;
		suspend_scan_count = 0;
		//if(xSemaphoreTake(xSem_comport,0))
		{
			//if(Test[35] == 100)
			Response_TCPIP_To_SUB(rx_buffer + UIP_HEAD,len - UIP_HEAD,Modbus.sub_port,header);
			if(modbus_send_len > 0)
			{
				int err = send(sock, (uint8_t *)&modbus_send_buf, modbus_send_len, 0);

				if (err < 0) {Test[46]++;
					//ESP_LOGE(TCP_TASK_TAG, "Error occurred during sending: errno %d", errno);
					//break;
				}
				else
					flagLED_ether_tx = 1;

				//xSemaphoreGive(xSem_comport);
				return err;
			}
			//xSemaphoreGive(xSem_comport);
		}
		//vTaskResume(&main_task_handle[5]);


		return 0;
	}
	return 0;
}


int readable__timeo(int fd, int sec)
{
	fd_set			rset;
	struct timeval	tv;

	FD_ZERO(&rset);
	FD_SET(fd, &rset);

	tv.tv_sec = sec;
	tv.tv_usec = 0;
	//debug_info("Timeout will occur after d sec");
	return(select(fd+1, &rset, NULL, NULL, &tv));
		/* > 0 if descriptor is readable */
}
/* end readable_timeo */

int Readable_timeo(int fd, int sec)
{
	int		n;

	if ( (n = readable__timeo(fd, sec)) < 0)
	{
		debug_info("readable_timeo error");
	}
	return(n);
}



void tcp_server_handle(void *args, int task_index)
{

	int ret = 0;

	int shudown_ret = 0;
	int close_ret = 0;
	struct sockinfo remoteInfo = {0};
	int nTask_Bit = 0;
	switch(task_index)
	{
	case 0:
		nTask_Bit = TASK1_BIT;
		break;
	case 1:
		nTask_Bit = TASK2_BIT;
		break;
	case 2:
		nTask_Bit = TASK3_BIT;
		break;
	case 3:
		nTask_Bit = TASK4_BIT;
		break;
	case 4:
		nTask_Bit = TASK5_BIT;
		break;
	case 5:
		nTask_Bit = TASK6_BIT;
		break;
	case 6:
		nTask_Bit = TASK7_BIT;
		break;

	default:
		return;
		break;

	}

	// 閿熸枻鎷穜emoteInfo.remoteIp 閿熸枻鎷烽敓鏂ゆ嫹涓�閿熸枻鎷烽敓渚ョ┖纭锋嫹 閿熸枻鎷锋敞閿熸枻鎷烽敓閰靛嚖鎷�
	//remoteInfo.remoteIp = (char *)heap_caps_malloc(32,MALLOC_CAP_8BIT);
	memset(remoteInfo.remoteIp,0,32);

	remoteInfo.sock			= ((struct sockinfo *)args)->sock;
	remoteInfo.remotePort 	= ((struct sockinfo *)args)->remotePort;
	memcpy(remoteInfo.remoteIp,((struct sockinfo *)args)->remoteIp,strlen(((struct sockinfo *)args)->remoteIp));

	EventBits_t res = xEventGroupClearBits(network_EventHandle,nTask_Bit);
	//if((res & nTask_Bit) != 0)
	//	debug_print("TASK _BIT cleared successfully",task_index);
	//else
	//{
	//	debug_print("TASK _BIT clear failed",task_index);
	//}

	int keepAlive = 1; // 閿熸枻鎷烽敓鏂ゆ嫹keepalive閿熸枻鎷烽敓鏂ゆ嫹
	int keepIdle = 10; // 閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓鏂ゆ嫹閿燂拷10閿熸枻鎷烽敓鏂ゆ嫹娌￠敓鏂ゆ嫹閿熻娇鐚存嫹閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓鏂ゆ嫹,閿熸枻鎷烽敓鏂ゆ嫹閿熸暀鏂ゆ嫹閿燂拷
	int keepInterval = 4; // 鎺㈤敓鏂ゆ嫹鏃堕敓鏂ゆ嫹閿熸枻鎷烽敓鏂ゆ嫹鏃堕敓鏂ゆ嫹閿熸枻鎷蜂负5 閿熸枻鎷�
	int keepCount = 1; // 鎺㈤敓瑙ｅ皾閿熺殕鐨勮揪鎷烽敓鏂ゆ嫹.閿熸枻鎷烽敓鏂ゆ嫹閿燂拷1閿熸枻鎷锋帰閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷风洀閿熸枻鎷烽敓鎺ワ讣鎷烽敓锟�,閿熸枻鎷烽敓锟�2閿熻娇鐨勮鎷烽敓鍔嚖鎷�.

	setsockopt(remoteInfo.sock,SOL_SOCKET,SO_KEEPALIVE,	(void *)&keepAlive,		sizeof(keepAlive));
	setsockopt(remoteInfo.sock,IPPROTO_TCP,TCP_KEEPIDLE,	(void *)&keepIdle,		sizeof(keepIdle));
	setsockopt(remoteInfo.sock,IPPROTO_TCP,TCP_KEEPINTVL,(void *)&keepInterval, 	sizeof(keepInterval));
	setsockopt(remoteInfo.sock,IPPROTO_TCP,TCP_KEEPCNT,	(void *)&keepCount, 	sizeof(keepCount));

    //struct timeval tv_out;
    //tv_out.tv_sec = 20;
    //tv_out.tv_usec = 0;
	//setsockopt(remoteInfo.sock, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));

	char len;
	for(;;)
	{
		if(task_index == 6)
		{
			vTaskDelay(50 / portTICK_RATE_MS);
			taskYIELD();
			debug_print("Close the last task when recv",task_index);
			break;
		}
        //if(task_index == 4)
        //{
        //	debug_print("task_index = 4 running",task_index);
        //}
		//debug_print("Readable_timeo ",task_index);

		ret = Readable_timeo(remoteInfo.sock, 60);//涓�閿熸枻鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓鏂ゆ嫹閿熸嵎灏卞叧鎲嬫嫹閿熼樁鏂ゆ嫹閿熸枻鎷� set timeout and add if
        //if(task_index == 4)
        //{
        	//char temp[20];
        	//sprintf(temp,"ret = %d",ret);
        	//debug_print(temp,task_index);
        //}
		if (ret > 0)
		{
			len = recv(remoteInfo.sock, rx_buffer[task_index], sizeof(rx_buffer) - 1, 0);

			if(len > 0)
			{flagLED_ether_rx = 1;ether_rx += len;
				ret = Modbus_Tcp(len,remoteInfo.sock,rx_buffer[task_index]);
				if(ret < 0)
				{
					debug_print("Modbus_Tcp ret < 0 error! ",task_index);
					break;
				}

			}
			else if(len == 0)
			{
				debug_print("Connection closed",task_index);
				break;
			}
			else
			{
				debug_print("Connection lost",task_index);
				break;
			}
		}
		else
		{
			debug_print("Read Timeout ",task_index);
            break;
		}

		vTaskDelay(50 / portTICK_RATE_MS);
		taskYIELD();
		//xQueueGiveMutexRecursive(sem_tcp_server);
	}
	if (remoteInfo.sock != -1)
	{
		debug_print("Shutting down socket",task_index);
		shudown_ret = shutdown(remoteInfo.sock, 0);
		if(shudown_ret!= 0)
		{
			debug_print("shutdown error!",task_index);
		}
		close_ret = close(remoteInfo.sock);
		if(close_ret!= 0)
		{
			debug_print("close error!",task_index);
		}
	}

	if(CountHandle != NULL)
	{
		if(xSemaphoreGive(CountHandle) != pdTRUE)
		{
			debug_print("Try to Give semaphore and failed!",task_index);
		}
		else
			debug_print("Give semaphore success!",task_index);
	}

	if(network_EventHandle != NULL)
	{
			EventBits_t uxBits = xEventGroupSetBits(network_EventHandle,nTask_Bit);
			if((uxBits & nTask_Bit) != 0)
				debug_print("set event bit ok",task_index);
			else
				debug_print("set event bit failed",task_index);
	}
	else
		debug_print("network_EventHandle is NULL",task_index);

	debug_print("vTaskDelete",task_index);
	Task_handle[task_index] = 0;
	vTaskDelete(NULL);

}

static void tcp_server_dealwith0(void *args)
{
	tcp_server_handle(args,0);
}

static void tcp_server_dealwith1(void *args)
{
	tcp_server_handle(args,1);
}


static void tcp_server_dealwith2(void *args)
{
	tcp_server_handle(args,2);
}

static void tcp_server_dealwith3(void *args)
{
	tcp_server_handle(args,3);
}

static void tcp_server_dealwith4(void *args)
{
	tcp_server_handle(args,4);
}

static void tcp_server_dealwith5(void *args)
{
	tcp_server_handle(args,5);
}

static void tcp_server_dealwith6(void *args)
{
	tcp_server_handle(args,6);
}





int check_sock_exist(int sock)
{
	uint8_t i;
	if(sock == -1)
		return -1;
	for(i = 0;i < MAX_TCP_CONN;i++)
	{
		if(sock == Sock_table[i]) // sock is exist
			return -1;
		if(Sock_table[i] == -1)
		{// table is not full, it is a new sock
			Sock_table[i] = sock;
			return i;
		}
	}
	if(i == MAX_TCP_CONN)
	{// table is full, replace the first sock
		Sock_table[0] = sock;
		return 0;
	}
	return -1;
}

#if 0//DDNS
#define  DOMAIN_NAME "temcocontrols.ddns.net"

void update_ddns(const char *ip_address);
void resolve_domain_to_ip(const char *domain_name, char *ip_address, size_t ip_len);
#endif
static void tcp_server_task(void *pvParameters)
{
	EventBits_t uxBits;
	UBaseType_t taskCount 	= 0;
	char taskName[50];
	struct hostent *hostP = NULL;
	int ip_protocol;
	char debug_buffer[100] =  {0};
	task_test.enable[2] = 1;
	xEventGroupSetBits(network_EventHandle,CONNECTED_BIT|TASK1_BIT|TASK2_BIT|TASK3_BIT|TASK4_BIT|TASK5_BIT|TASK6_BIT|TASK7_BIT); //Fandu : CONNECTED_BIT锟斤拷锟斤还锟斤拷要锟斤拷锟斤拷 wifi锟角凤拷锟斤拷锟接碉拷锟脚猴拷锟斤拷

#if 0//DDNS
    // 更新动态 DNS
	char resolved_ip[INET_ADDRSTRLEN]; // 用于存储解析到的 IP 地址
    const char *current_ip = "192.168.0.160"; // 替换为设备的当前 IP 地址
    update_ddns(current_ip);
    // 解析域名
    resolve_domain_to_ip(DOMAIN_NAME, resolved_ip, sizeof(resolved_ip));
#endif
	while(1)
	{
			taskCount++;
			debug_info("tcp_server_task is running\r");
		    int addr_family;
			addr_family 			 = AF_INET;
			ip_protocol 			 = IPPROTO_IP;
			// 锟斤拷锟斤拷IP锟斤拷为0 应锟矫底诧拷锟斤拷远锟斤拷锟斤拷帽锟斤拷锟絀P 锟剿口固讹拷锟斤拷7681
			struct sockaddr_in localAddr;
			localAddr.sin_addr.s_addr 	= htonl(INADDR_ANY);
			localAddr.sin_family		= AF_INET;
			localAddr.sin_port			= htons(Modbus.tcp_port);
			//锟铰斤拷一锟斤拷 socket
			int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
			if (listen_sock < 0)
			{
				debug_info("Unable to create socket\r");
				vTaskDelay(5000 / portTICK_PERIOD_MS); //5锟斤拷锟接猴拷锟斤拷锟斤拷锟斤拷执锟斤拷
				continue;
			}
			int err = bind(listen_sock, (struct sockaddr *)&localAddr, sizeof(localAddr));
			if (err < 0) {
				debug_info("Socket unable to bind: errno\r");
				vTaskDelay(5000 / portTICK_PERIOD_MS); //5锟斤拷锟接猴拷锟斤拷锟斤拷锟斤拷执锟斤拷
				continue;
			}

			//锟斤拷锟斤拷锟斤拷锟斤拷 锟斤拷锟斤拷7681锟剿匡拷
			err = listen(listen_sock,0);
			if(err != 0)
			{
				debug_info("Socket unable to connect: errno\r");
				vTaskDelay(5000 / portTICK_PERIOD_MS); //5锟斤拷锟接猴拷锟斤拷锟斤拷锟斤拷执锟斤拷
				continue;
			}
			debug_info("Socket is listening\r");
			//为accpet锟斤拷锟接达拷锟斤拷锟斤拷锟斤拷锟绞硷拷锟�
			struct sockaddr_in6 sourceAddr;
			uint addrLen = sizeof(sourceAddr);

			while (1)
			{
				debug_info("ready to accept %d\r");
				task_test.count[2]++;
				//锟斤拷取锟脚猴拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷portMAX_DELAY
				if(CountHandle != NULL)
				{
					xSemaphoreTake(CountHandle,portMAX_DELAY);
					UBaseType_t semapCount = uxSemaphoreGetCount(CountHandle);
					sprintf(debug_buffer,"Semaphore take success semapCount is:%d",semapCount);
					debug_info(debug_buffer);
				}
				else
					debug_info("SemaphoreHandle is NULL");

				//accept锟角伙拷锟斤拷锟斤拷锟斤拷锟斤拷锟�  锟斤拷锟絊emaphorTake 也一直锟斤拷锟酵诧拷知锟斤拷锟叫诧拷锟叫★拷
				int sock = accept(listen_sock, (struct sockaddr *)&sourceAddr, &addrLen);
				if (sock < 0)
				{
					ESP_ERROR_CHECK(sock);
					debug_info("Unable to accept connection\r");
					break;
				}
				debug_info("Socket accepted\r");

				//锟斤拷取锟斤拷accept锟斤拷IP sock 锟剿匡拷锟斤拷息锟斤拷锟斤拷
				struct sockinfo remoteInfo;

				remoteInfo.sock = sock;
				if(sourceAddr.sin6_family == PF_INET)
				{
					memcpy(remoteInfo.remoteIp ,inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr,addr_str,sizeof(addr_str) - 1),32);
					//remoteInfo.remoteIp = inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr,addr_str,sizeof(addr_str) - 1);
					remoteInfo.sa_familyType = PF_INET;

				}else if(sourceAddr.sin6_family == PF_INET6)
				{
					memcpy(remoteInfo.remoteIp,inet6_ntoa_r(sourceAddr.sin6_addr,addr_str,sizeof(addr_str) - 1),32);
					//remoteInfo.remoteIp = inet6_ntoa_r(sourceAddr.sin6_addr,addr_str,sizeof(addr_str) - 1);
					remoteInfo.sa_familyType = PF_INET6;
				}
				remoteInfo.remotePort = ntohs(sourceAddr.sin6_port);
				//sprintf(debug_buffer,"ip:%s,port:%d ,sock:%d connected\r",remoteInfo.remoteIp,remoteInfo.remotePort,remoteInfo.sock);
				//debug_info(debug_buffer);


				uxBits = xEventGroupWaitBits(network_EventHandle,TASK1_BIT|TASK2_BIT|TASK3_BIT|TASK4_BIT|TASK5_BIT|TASK6_BIT|TASK7_BIT,false,false,portMAX_DELAY);
				//debug_info("tcp_server_task get  xEventGroupWaitBits success\r");
				for(int i = 0; i < MAX_SOC_COUNT; i++)
				{
					if((uxBits & (1 << (i + 1))) != 0)
					{ //锟斤拷锟斤拷i + 1锟斤拷锟斤拷为 锟铰硷拷锟斤拷志锟斤拷锟斤拷锟斤拷一位锟斤拷锟斤拷CONNECT_BIT锟斤拷占锟斤拷 TASK2_BIT锟角从碉拷BIT1锟斤拷始
						sprintf(taskName,"tcp_server_dealwith%d",i);
						//锟斤拷印remoteInfo锟斤拷锟斤拷锟斤拷然锟斤拷锟劫斤拷锟斤拷锟斤拷锟斤拷
						//ESP_LOGI(TAG,"Currently socket NO:%d IP is:%s PORT is:%d",sock,remoteInfo.remoteIp,remoteInfo.remotePort);
						task_sock[i] = remoteInfo.sock;
						int res1 = xTaskCreate(taskList[i], taskName,	4096, (void *)&remoteInfo,1, &Task_handle[i]);
						//assert(res1 == pdTRUE);
						sprintf(debug_buffer,"xTaskCreate %d\r",i);
						debug_info(debug_buffer);
						break; //锟斤拷锟斤拷晒锟斤拷拇锟斤拷锟斤拷锟揭伙拷锟斤拷锟斤拷锟斤拷应锟矫斤拷锟斤拷锟斤拷锟轿诧拷锟斤拷锟斤拷
					}
				}
				vTaskDelay(200 / portTICK_PERIOD_MS);

			}
			if (listen_sock != -1)
			{
				debug_info("Shutting down listen_socket and restarting...");
				shutdown(listen_sock, 0);
				close(listen_sock);
			}

			vTaskDelay(5000 / portTICK_PERIOD_MS); //5锟斤拷锟接猴拷锟斤拷锟斤拷锟斤拷执锟斤拷
	}
	vTaskDelete(NULL);
}

typedef struct
{
	int socket;
	uint8_t ip;
	uint32_t time;
}Str_Tcp_CS;
Str_Tcp_CS tcp_client[6];
// check whether need creat a new socket
int get_current_client_socket(uint8_t ip)
{
	uint8_t i;
	for(i = 0;i < 6;i++)
	{
		if(ip == tcp_client[i].ip)
			return i;
	}
	return 0;
}


void intial_tcp_client(void)
{
	uint8_t i;
	for(i = 0;i < 6;i++)
	{
		tcp_client[i].ip = 0;
		tcp_client[i].socket = -1;
		tcp_client[i].time = 0;
	}
}

uint8_t check_time_to_live(void)
{
	uint8_t i;
	for(i = 0;i < 6;i++)
	{
		if((tcp_client[i].ip != 0) && (tcp_client[i].socket != -1))
		{
			if(system_timer - tcp_client[i].time > 10000)
			{
				shutdown(tcp_client[i].socket, 2);
				close(tcp_client[i].socket);
				tcp_client[i].ip = 0;
				tcp_client[i].socket = -1;
				//tcp_client[i].time = 0;
				return 1;
			}

		}
	}
	return 0;
}
//static const char *payload = "Message from ESP32 ";

static void tcp_client_task(void *pvParameters)
{
    char rx_buffer[128];
    U8_T network_point_index;
    //char host_ip[] = "192.168.0.55";
    int addr_family = 0;
    int ip_protocol = 0;
    uint8_t ip,sub_id,func,reg;
    uint8_t Modbus_Client_Command[20];
    uint8_t Modbus_Client_CmdLen = 0;
    static u8_t tcp_client_transaction_id = 0;
    static uint8_t index = 0;
    intial_tcp_client();
    memset(NPM_node_write,0,sizeof(STR_NPM_NODE_OPERATE) * STACK_LEN);
    task_test.enable[3] = 1;
    while (1) {
    	task_test.count[3]++;
    	if(number_of_network_points_modbus > 0)
    	{
    		// write modbus points
    		for(uint8_t i = 0;i < STACK_LEN;i++)
    		{
    			if(NPM_node_write[i].flag == 1) //	get current index, 1 -- WAIT_FOR_WRITE, 0 -- WRITE_OK
    			{
    				if(NPM_node_write[i].retry < 10)
    				{
    					NPM_node_write[i].retry++;
    				}
    				else
    				{  	// retry 10 time, give up
    					NPM_node_write[i].flag = 0;
    					NPM_node_write[i].retry = 0;
    					break;
    				}
    				ip = NPM_node_write[i].ip;
					sub_id = NPM_node_write[i].id;
					func = NPM_node_write[i].func;
					reg = NPM_node_write[i].reg;

					struct sockaddr_in dest_addr;
					int err;
					dest_addr.sin_addr.s_addr =	((uint32_t)ip << 24) + ((uint32_t)Modbus.ip_addr[2] << 16) +\
							((uint16_t)Modbus.ip_addr[1] << 8) + Modbus.ip_addr[0];
					dest_addr.sin_family = AF_INET;
					dest_addr.sin_port = htons(Modbus.tcp_port);
					addr_family = AF_INET;
					ip_protocol = IPPROTO_IP;

					int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
					index = get_current_client_socket(ip);
					check_time_to_live();

					if(tcp_client[index].socket == -1)
					{
						tcp_client[index].socket = socket(addr_family, SOCK_STREAM, ip_protocol);
						if (tcp_client[index].socket < 0) {
							continue;
						}
						err = connect(tcp_client[index].socket, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
					}
					else
						err = 0;


					if (err != 0) {
							continue;
						 }
						 else //while(1)
						 {
							if(tcp_client_transaction_id < 127)
								tcp_client_transaction_id++;
							else
								tcp_client_transaction_id = 1;


							Modbus_Client_Command[0] = NPM_node_write[i].ip;//transaction_id >> 8;
							Modbus_Client_Command[1] = tcp_client_transaction_id;
							Modbus_Client_Command[2] = 0x00;
							Modbus_Client_Command[3] = 0x00;
							Modbus_Client_Command[4] = 0x00;
							Modbus_Client_Command[5] = 0x06;  // len
							if(NPM_node_write[i].id == 0)
								NPM_node_write[i].id = 255;
							Modbus_Client_Command[6] = NPM_node_write[i].id;
							Modbus_Client_Command[7] = NPM_node_write[i].func;


							Modbus_Client_Command[8] = NPM_node_write[i].reg >> 8;
							Modbus_Client_Command[9] = NPM_node_write[i].reg;
							if(NPM_node_write[i].len == 1)
							{
								if(NPM_node_write[i].func == 0x06)
								{
									Modbus_Client_Command[10] = NPM_node_write[i].value[0] >> 8;
									Modbus_Client_Command[11] = NPM_node_write[i].value[0];
								}
								else if(NPM_node_write[i].func == 0x05) // wirte coil
								{
									if(NPM_node_write[i].value[0] == 0)
									{
										Modbus_Client_Command[10] = 0x00;
										Modbus_Client_Command[11] = 0x00;
									}
									else  // *value = 0
									{
										Modbus_Client_Command[10] = 0xFF;
										Modbus_Client_Command[11] = 0x00;
									}
								}

								Modbus_Client_CmdLen = 12;
							}
							if(NPM_node_write[i].len == 2)
							{
								Modbus_Client_Command[5] = 0x0b;  // len
								Modbus_Client_Command[10] = 0x00;
								Modbus_Client_Command[11] = 0x02;
								Modbus_Client_Command[12] = 0x04;
								Modbus_Client_Command[13] = NPM_node_write[i].value[0];
								Modbus_Client_Command[14] = NPM_node_write[i].value[0] >> 8;
								Modbus_Client_Command[15] = NPM_node_write[i].value[1];
								Modbus_Client_Command[16] = NPM_node_write[i].value[1] >> 8;
								Modbus_Client_CmdLen = 17;
							}

							err = send(tcp_client[index].socket, Modbus_Client_Command,Modbus_Client_CmdLen, 0);

							if (err < 0) {
								//ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
								//break;
								continue;
								}
							else
								flagLED_ether_tx = 1;

							err = Readable_timeo(tcp_client[index].socket, 10);
							if(err > 0)
							{
								int len = recv(tcp_client[index].socket, rx_buffer, sizeof(rx_buffer) - 1, 0);
								// Error occurred during receiving

								if (len < 0) {
									//ESP_LOGE(TAG, "recv failed: errno %d", errno);
									continue;
								}
								// Data received
								else {
									ether_rx += len;
									flagLED_ether_rx = 1;
									tcp_client[index].time = system_timer;
									U8_T tcp_clinet_buf[20];
									S32_T val_ptr = 0;
									U8_T float_type = 0;
									if(len == 12 || len == 17)	// response write
									{
										memcpy(&tcp_clinet_buf, rx_buffer,len);

										//float_type = NP_node_write[i].func;

										if(ip == tcp_clinet_buf[0])
										{
											//if(float_type == 0)
											{
												val_ptr = tcp_clinet_buf[10] * 256 + tcp_clinet_buf[11];

												add_network_point( NPM_node_write[i].ip,
														NPM_node_write[i].id,
														NPM_node_write[i].func,
														NPM_node_write[i].reg,
														val_ptr * 1000,
												0,float_type);

												//flag_receive_netp_modbus = 1;
												//network_points_list[network_point_index].lose_count = 0;
												//network_points_list[network_point_index].decomisioned = 1;
											}

										}

									}
								}


						 }
    				}
    		}

    	}
    		// 閿熸枻鎷穘etwork modbus point
    		for(network_point_index = 0;network_point_index < number_of_network_points_modbus;network_point_index++)
    		{
    			if(network_points_list[network_point_index].lose_count > 3)
				{
					network_points_list[network_point_index].lose_count = 0;
					network_points_list[network_point_index].decomisioned = 0;
				}

				//flag_receive_netp_modbus = 0;

				ip = network_points_list[network_point_index].point.panel;
				sub_id = network_points_list[network_point_index].tb.NT_modbus.id;
				func = network_points_list[network_point_index].tb.NT_modbus.func & 0x7f;
				reg = network_points_list[network_point_index].tb.NT_modbus.reg;

				struct sockaddr_in dest_addr;
				int err;
				dest_addr.sin_addr.s_addr =
				((uint32_t)ip << 24) + ((uint32_t)Modbus.ip_addr[2] << 16) +\
						((uint16_t)Modbus.ip_addr[1] << 8) + Modbus.ip_addr[0];
				dest_addr.sin_family = AF_INET;
				dest_addr.sin_port = htons(Modbus.tcp_port);
				addr_family = AF_INET;
				ip_protocol = IPPROTO_IP;
				int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
				index = get_current_client_socket(ip);
				check_time_to_live();

				if(tcp_client[index].socket == -1)
				{
					tcp_client[index].socket = socket(addr_family, SOCK_STREAM, ip_protocol);
					if (tcp_client[index].socket < 0) {
						//ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
					//debug_info("Unable to create socket: errno");
						continue;
					}
					err = connect(tcp_client[index].socket, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
				}
				else
					err = 0;




				if (err != 0) {
						//ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
				//debug_info("Socket unable to connect");
						continue;
					 }
					 else //while(1)
					 {
						 // check time to live
						 tcp_client[index].ip = ip;
				    // send packet to tcp_server
				    	if(tcp_client_transaction_id < 127)
							tcp_client_transaction_id++;
						else
							tcp_client_transaction_id = 1;

				    	Modbus_Client_Command[0] =  ip;// 0x00;//transaction_id >> 8;
						Modbus_Client_Command[1] = tcp_client_transaction_id;
						Modbus_Client_Command[2] = 0x00;
						Modbus_Client_Command[3] = 0x00;
						Modbus_Client_Command[4] = 0x00;
						Modbus_Client_Command[5] = 0x06;  // len
						if(sub_id == 0)
								sub_id = 255;
						Modbus_Client_Command[6] = sub_id;
						Modbus_Client_Command[7] = func;

						if(func == READ_VARIABLES || func == READ_COIL
							|| func == READ_DIS_INPUT || func == READ_INPUT)  // read command
						{// 01 03 02 04
							U8_T float_type;
							Modbus_Client_Command[8] = reg >> 8;
							Modbus_Client_Command[9] = reg;
							Modbus_Client_Command[10] = 0x00;
							float_type = (network_points_list[network_point_index].tb.NT_modbus.func & 0xff00) >> 8;
							// for specail customer, use READ_INPUT to replace INPUT_FLOATABCD,
							if(func == READ_INPUT) float_type = 1;
							if(float_type == 0)
							{
								Modbus_Client_Command[11] = 0x01;
								Modbus_Client_CmdLen = 12;
							}
							else
							{
								Modbus_Client_Command[11] = 0x02;
								Modbus_Client_CmdLen = 12;
							}
						}

						err = send(tcp_client[index].socket, Modbus_Client_Command,Modbus_Client_CmdLen, 0);

						if (err < 0) {
							//ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
							//break;
							continue;
							}
						else
							flagLED_ether_tx = 1;


						err = Readable_timeo(tcp_client[index].socket, 10);
						if(err > 0)
						{
							int len = recv(tcp_client[index].socket, rx_buffer, sizeof(rx_buffer) - 1, 0);
							// Error occurred during receiving

							if (len < 0) {
								//ESP_LOGE(TAG, "recv failed: errno %d", errno);
								continue;
							}
							// Data received
							else {flagLED_ether_rx = 1;ether_rx += len;
							//debug_info("revc ok");
								tcp_client[index].time = system_timer;
								U8_T tcp_clinet_buf[20];
								S32_T val_ptr = 0;
								U8_T float_type = 0;
								if(len == 11 || len == 13 || len == 10)  // response read
								{ // READ ONE is 11, read 2bytes is 13, read coil is 10
									memcpy(&tcp_clinet_buf, rx_buffer,len);

									//if(network_points_list[network_point_index].point.panel == (U8_T)(ip >> 24) )
									float_type = (network_points_list[network_point_index].tb.NT_modbus.func & 0xff00) >> 8;
									if(ip == tcp_clinet_buf[0])
									{
										if(len == 13)	// read input float 32bit
											float_type = 1;
										if(float_type == 1)
											val_ptr = (U32_T)(tcp_clinet_buf[9] << 24) + (U32_T)(tcp_clinet_buf[10] << 16) \
														+ (U16_T)(tcp_clinet_buf[11] << 8) + tcp_clinet_buf[12];
										else
										{
											if(len == 11)
												val_ptr = tcp_clinet_buf[9] * 256 + tcp_clinet_buf[10];
											else if(len == 10)
												val_ptr = tcp_clinet_buf[9];
											else
												;// error
										}

										if((tcp_clinet_buf[6] == network_points_list[network_point_index].tb.NT_modbus.id)
											&& (network_points_list[network_point_index].tb.NT_modbus.id != 0)
											&& (tcp_clinet_buf[7] == (network_points_list[network_point_index].tb.NT_modbus.func & 0x7f))
										)
										{
											add_network_point( network_points_list[network_point_index].point.panel,
											network_points_list[network_point_index].point.sub_id,
											network_points_list[network_point_index].point.point_type - 1,
											network_points_list[network_point_index].point.number + 1,
											val_ptr,
											0,float_type);

											//flag_receive_netp_modbus = 1;
											network_points_list[network_point_index].lose_count = 0;
											network_points_list[network_point_index].decomisioned = 1;
										}
									}
								}
							}
						}
						else
						{

							network_points_list[network_point_index].lose_count++;
						}

						vTaskDelay(200 / portTICK_PERIOD_MS);
					}


    		}// end 閿熸枻鎷穘etwork modbus point
    	}
    	else
    	{
    		vTaskDelay(5000 / portTICK_PERIOD_MS);
    	}
    }

    vTaskDelete(NULL);
}



EXT_RAM_ATTR uint8_t  PDUBuffer[MAX_APDU];
uint8_t Station_NUM;
uint8_t MAX_MASTER;
extern U8_T base_in;
extern U8_T base_out;
void Bacnet_Initial_Data(void);
void Trend_Log_Init(void);
void Initial_points(uint8_t point_type);
void set_default_parameters(void)
{
	save_uint8_to_flash(FLASH_MODBUS_ID,1);
	save_uint8_to_flash(FLASH_EN_SNTP,1);
	save_uint8_to_flash(FLASH_EN_TIME_SYNC_PC,1);
	save_uint8_to_flash(FLASH_LCD_TIME_OFF_DELAY,255);
	save_uint16_to_flash(FLASH_WRITE_FLASH,0);
	save_uint8_to_flash(FLASH_FIX_COM_CONFIG,1);
	Bacnet_Initial_Data();

	Initial_points(OUT);
	Initial_points(IN);
	Initial_points(VAR);
	save_point_info(0);

}

void Inital_Bacnet_Server(void)
{
	uint32 ltemp = 0;

	if(((panelname[0] == 0) && (panelname[1] == 0) && (panelname[2] == 0))  ||
			((panelname[0] == 255) && (panelname[1] == 255) && (panelname[2] == 255)) )
	{
		if(Modbus.mini_type == MINI_BIG_ARM)
			Set_Object_Name("T3-BB-ESP");
		else if(Modbus.mini_type == MINI_SMALL_ARM)
			Set_Object_Name("T3-LB-ESP");
		else if(Modbus.mini_type == MINI_TINY_ARM)
			Set_Object_Name("T3-TB-ESP");
		else if(Modbus.mini_type == MINI_NANO)
			Set_Object_Name("T3-NB-ESP");
		else if(Modbus.mini_type == PROJECT_FAN_MODULE)
			Set_Object_Name("T3-FAN-ESP");
		else if(Modbus.mini_type == PROJECT_TRANSDUCER)
			Set_Object_Name("T3-TRANS-ESP");
		else if(Modbus.mini_type == PROJECT_POWER_METER)
			Set_Object_Name("T3-POWER-ESP");
		else if(Modbus.mini_type == PROJECT_RMC1216)
			Set_Object_Name("T3-RMC1216");
		else if(Modbus.mini_type == PROJECT_NG2_NEW)
			Set_Object_Name("T3-NEWNG2-ESP");
		else if(Modbus.mini_type == PROJECT_LIGHT_PWM)
			Set_Object_Name("T3-LPWM-ESP");
		else
			Set_Object_Name("T3-XX-ESP");
	}
	else
		Set_Object_Name(panelname);

	Device_Init();

	//Modbus.mini_type = MINI_NANO;//
	Initial_Panel_Info(); // read panel name, must read flash first
	//Instance = ((uint32)Modbus.serialNum[3]<<24)+((uint32)Modbus.serialNum[2]<<16)+((uint16)Modbus.serialNum[1]<<8) + Modbus.serialNum[0];

	// tbd:
	Station_NUM = Modbus.address;
	MAX_MASTER = 254;
	//Modbus.mini_type = PROJECT_FAN_MODULE;
	//memset(panelname,"T3-XB_ESP",15);
	panel_number = Station_NUM;

	Sync_Panel_Info();
	read_point_info();

	if(Setting_Info.reg.webview_json_flash != 2)
	{
		initial_graphic_point();
	}
	Comm_Tstat_Initial_Data();
	init_scan_db();

	Device_Set_Object_Instance_Number(Instance);
	address_init();
	bip_set_broadcast_addr(0xffffffff);

	if(Modbus.mini_type == PROJECT_FAN_MODULE)
	{
		AIS = 6;
		AOS = 2;
		AVS = 0;
		BOS = 0;
	}
	else if(Modbus.mini_type == PROJECT_POWER_METER)
	{
		AIS = 7;
		AOS = 0;
		AVS = 63;
		BOS = 0;
	}
	else if(Modbus.mini_type == PROJECT_AIRLAB)
	{
		AIS = 18;
		AOS = 0;
		AVS = 5;
		BOS = 0;
	}
	else if(Modbus.mini_type == PROJECT_TRANSDUCER)
	{
		AIS = 3;
		AOS = 3;
		AVS = 3;
		BOS = 0;
	}
	else if(Modbus.mini_type == PROJECT_LSW_BTN)
	{
		AIS = 16;
		AOS = 4;
		AVS = 3;
		BOS = 4;
		TemcoVars = 3;
	}
	else if(Modbus.mini_type == PROJECT_LSW_SENSOR)
	{
		AIS = 16;
		AOS = 4;
		AVS = 3;
		BOS = 4;
		TemcoVars = 3;
	}
	else if(Modbus.mini_type == MINI_TSTAT10)
	{
		AIS = MAX_INS + 1;
		AOS = MAX_AOS + 1;
		AVS = MAX_AVS + 1;
		BOS = 0;
		TemcoVars = 9;
	}
	else
	{
		AIS = MAX_INS + 1;
		AOS = MAX_AOS + 1;
		AVS = MAX_AVS + 1;
		BOS = 0;
#if 1//BAC_TRENDLOG
		TRENDLOGS = 0;
#endif
	}
	Count_VAR_Object_Number(AVS);
	Count_IN_Object_Number();
	Count_OUT_Object_Number();

}
//EXT_RAM_ATTR FIFO_BUFFER Receive_Buffer0;
//EXT_RAM_ATTR uint8_t Receive_Buffer_Data0[512];
int Send_private_scan(U8_T index);
S8_T Get_rmp_index_by_panel(uint8_t panel,uint8_t sub_id,uint8_t * index,uint8_t protocal);
int GetRemotePoint(uint8_t object_type,uint32_t object_instance,uint8_t panel,uint8_t sub,uint8_t protocal);


uint16_t dlmstp_receive(
    BACNET_ADDRESS * src,       /* source address */
    uint8_t * pdu,      /* PDU data */
    uint16_t max_pdu,   /* amount of space available in the PDU  */
    unsigned port);
uint8_t flag_receive_rmbp;
uint8_t flag_start_scan_mstp = 0;
uint8_t start_scan_mstp_count = 0;
uint16_t Master_Scan_Mstp_Count = 0;
void set_mstp_master(void)
{
	Modbus.mstp_master = 0;
	Master_Scan_Mstp_Count = 0;
}

#if 1
void MSTP_Master_roution(uint16 count_start_task)
{
	int invoke = 0;
	if(count_start_task % 12000 == 0)	// 1 min
	{
		//if(((Modbus.mini_type >= MINI_BIG_ARM) && (Modbus.mini_type <= MINI_NANO)) || (Modbus.mini_type == MINI_TINY_11I))
		{
			//if(Modbus.com_config[2] == BACNET_MASTER || Modbus.com_config[0] == BACNET_MASTER)
			if(Modbus.mstp_master == 1 || flag_start_scan_mstp == 1)
			{
				//if(upate_mstp_flag == 0)
				{
						Send_Whois_Flag = 1;
				}
				if((flag_start_scan_mstp++ > 2))
				{
					start_scan_mstp_count = 0;
					flag_start_scan_mstp = 0;
				}
			}
		}
	}
	else
	{ 	 // whether exist remote mstp point
		//if(((Modbus.mini_type >= MINI_BIG_ARM) && (Modbus.mini_type <= MINI_NANO)) || (Modbus.mini_type == MINI_TINY_11I))
		{
			if(Modbus.mstp_master == 1 || flag_start_scan_mstp == 1)//if(Modbus.com_config[2] == BACNET_MASTER || Modbus.com_config[0] == BACNET_MASTER)
			{
				if((count_start_task % 200 == 0) /*&& (upate_mstp_flag == 0)*/) // 1.5s
				{
					// check whether the device is online or offline
					if(flag_receive_rmbp == 1)
					{
						U8_T remote_panel_index;
						if(Get_rmp_index_by_panel(remote_points_list[remote_bacnet_index].point.panel,
						remote_points_list[remote_bacnet_index].point.sub_id,
						&remote_panel_index,
						BAC_MSTP) != -1)
						{
							remote_panel_db[remote_panel_index].time_to_live = RMP_TIME_TO_LIVE;
						}
						remote_points_list[remote_bacnet_index].lose_count = 0;
						remote_points_list[remote_bacnet_index].decomisioned = 1;
					}
					else
					{
						remote_points_list[remote_bacnet_index].lose_count++;
						if(remote_points_list[remote_bacnet_index].lose_count > 10)
						{
							remote_points_list[remote_bacnet_index].lose_count = 0;
							remote_points_list[remote_bacnet_index].decomisioned = 0;
						}
					}

				// read remote mstp points
					if(remote_bacnet_index < number_of_remote_points_bacnet)
					{
						remote_bacnet_index++;
					}
					else
					{
						remote_bacnet_index = 0;
					}

					if(remote_bacnet_index == number_of_remote_points_bacnet)
					{  // read private modbus from Temco product

						static uint8_t j = 0;
						uint8_t count = 0;
						if(j < remote_panel_num)//for(j = 0;j < remote_panel_num;j++)
						{
							if(remote_panel_db[j].protocal == BAC_MSTP
								&& remote_panel_db[j].sn == 0)
							{
								remote_panel_db[j].retry_reading_panel++;
								flag_receive_rmbp = 0;
								invoke = Send_private_scan(j);
								remote_mstp_panel_index = j;
								while((flag_receive_rmbp == 0) && count++ < 20)
									delay_ms(200);
							}
							if(remote_panel_db[j].retry_reading_panel > 5)
							{
								remote_panel_db[j].sn = remote_panel_db[j].device_id;
								remote_panel_db[j].retry_reading_panel = 0;
								remote_panel_db[j].product_model = 0;
							}
						}
						j++;

						if(j > remote_panel_num)
							j = 0;


					}
					else
					{
						if(number_of_remote_points_bacnet > 0)
						{
							// read remote bacnet point
							//if(Modbus.com_config[2] == BACNET_MASTER || Modbus.com_config[0] == BACNET_MASTER)
							if(Modbus.mstp_master == 1 || flag_start_scan_mstp == 1)
							{
								flag_receive_rmbp = 0;
								invoke = GetRemotePoint(remote_points_list[remote_bacnet_index].tb.RP_bacnet.object,
											remote_points_list[remote_bacnet_index].tb.RP_bacnet.instance,
											panel_number,/*Modbus.network_ID[2],*/
											remote_points_list[remote_bacnet_index].tb.RP_bacnet.panel ,
											BAC_MSTP);
								// check whether the device is online or offline

								if(invoke >= 0)
								{
									remote_points_list[remote_bacnet_index].invoked_id	= invoke;
								}
								else
								{
									remote_points_list[remote_bacnet_index].lose_count++;
								}
							}
						}
					}
				}
			}
		}
	}

}
#endif
void Master0_Node_task(void)
{

	uint16_t pdu_len = 0;

	BACNET_ADDRESS  src;
	static uint16_t count_start_task = 0;

	dlmstp_init(NULL);

	if(Modbus.com_config[0] == BACNET_SLAVE || Modbus.com_config[0] == BACNET_MASTER)
	{
		Recievebuf_Initialize(0);
		Send_I_Am_Flag = 1;
		Send_Whois_Flag = 1;
	}
	remote_bacnet_index = 0;
	number_of_remote_points_bacnet = 0;
	task_test.enable[8] = 1;

	Modbus.mstp_master = 1;
	Master_Scan_Mstp_Count = 0;

//	flag_mstp_err[0] = flag_mstp_err[2] = 1;
//	count_mstp_err[0] = count_mstp_err[2] = 0;
	//uart_serial_restart(0);

	for (;;)
	{
		task_test.count[8]++;

		if(Master_Scan_Mstp_Count++ >= 12000)
		{  // did not find master
			// current panel is master
			Modbus.mstp_master = 1;
			Master_Scan_Mstp_Count = 0;

		}


		if(Modbus.com_config[0] == BACNET_MASTER || Modbus.com_config[0] == BACNET_SLAVE)
		{

//			count_send_whois++;
			vTaskDelay(5 / portTICK_RATE_MS);

			MSTP_Master_roution(count_start_task);
			if(count_start_task < 12000) // 1min
				count_start_task++;
			else
				count_start_task = 0;

			//if(flag_suspend_mstp == 0)
			{
				pdu_len = datalink_receive(&src, &PDUBuffer[0], sizeof(PDUBuffer), 0,BAC_MSTP);
				{
					if(pdu_len)
					{
						npdu_handler(&src, &PDUBuffer[0], pdu_len,BAC_MSTP);
					}
				}
			}
		}
		else
		{
				//if(upate_mstp_flag == 0)
			delay_ms(5000);
		}

	}
}

EXT_RAM_ATTR FIFO_BUFFER Receive_Buffer2;
EXT_RAM_ATTR uint8_t Receive_Buffer_Data2[512];
EXT_RAM_ATTR uint8_t  PDUBuffer2[MAX_APDU];

void Master2_Node_task(void)
{

	uint16_t pdu_len = 0;
	BACNET_ADDRESS  src;
	static uint16_t count_start_task = 0;

	if(Modbus.com_config[2] == BACNET_SLAVE || Modbus.com_config[2] == BACNET_MASTER)
	{
		Recievebuf_Initialize(2);
		Send_I_Am_Flag = 1;
		Send_Whois_Flag = 1;
	}

	task_test.enable[11] = 1;
	for (;;)
	{
		task_test.count[11]++;

		if(Modbus.com_config[2] == BACNET_MASTER || Modbus.com_config[2] == BACNET_SLAVE)
		{
			//
			if((Modbus.com_config[0] != BACNET_MASTER) && (Modbus.com_config[0] != BACNET_SLAVE))
			{
				MSTP_Master_roution(count_start_task);
				if(count_start_task < 12000) // 1min
					count_start_task++;
				else
					count_start_task = 0;
			}


			uint8_t count;
			int invoke;
			pdu_len = dlmstp_receive(&src, &PDUBuffer2[0], sizeof(PDUBuffer), 2);
			if(pdu_len)
			{
				npdu_handler(&src, &PDUBuffer2[0], pdu_len, BAC_MSTP);
			}
			vTaskDelay(5 / portTICK_RATE_MS);
		}
		else
		{
			vTaskDelay(5000 / portTICK_RATE_MS);
		}

	}

}

uint8_t get_protocal(uint8 port)
{
	return Modbus.com_config[port];
}



uint32_t net_health[4];
//uint32_t wifi_rx;
uint32_t ether_rx;
void check_net_health(uint8_t interval)
{
	static uint8_t count = 0;
	static uint32_t backup_rx[4] = {0,0,0,0};

	if(interval <= 0)
		return ;

	if(count < interval)
		count++;
	else
	{
		count = 0;
		net_health[0] = (com_rx[2] - backup_rx[0]);
		net_health[1] = (com_rx[0] - backup_rx[1]);
		net_health[2] = (ether_rx - backup_rx[2]);
		net_health[3] = net_health[2];//(wifi_rx - backup_rx[3]);


		backup_rx[0] = com_rx[2];
		backup_rx[1] = com_rx[0];
		backup_rx[2] = ether_rx;
		//backup_rx[3] = wifi_rx;
	}
}


void check_task(void)// check task
{
	uint8_t loop;

	for(loop = 0;loop < 20;loop++)
	{
		if(task_test.enable[loop] == 1)
		{
		  if(task_test.count[loop] != task_test.old_count[loop])
			{
				task_test.old_count[loop] = task_test.count[loop];
				task_test.inactive_count[loop] = 0;
			}
			else
				task_test.inactive_count[loop]++;
		}
		/*if(task_test.inactive_count[0] > 20)
		{
			task_test.inactive_count[0] = 0;
			E2prom_Write_Byte(EEP_TEST1,200);
			delay_ms(10);
			flag_reboot = 1;
		}*/
	}
}

uint32_t system_timer = 0;
uint32_t run_time = 0;


uint8 led_sub_tx;
uint8 led_sub_rx;
uint8 led_main_tx;
uint8 led_main_rx;

uint8 flagLED_ether_tx = 0;
uint8 flagLED_ether_rx = 0;
uint8 flagLED_sub_rx = 0;
uint8 flagLED_sub_tx = 0;
uint8 flagLED_main_rx = 0;
uint8 flagLED_main_tx = 0;
//uint8 flagLED_uart2_rx = 0;
//uint8 flagLED_uart2_tx = 0;

extern uint16_t Mstp_NotForUs;
extern uint16_t Mstp_ForUs;
extern uint16_t current_page;
esp_err_t save_point_info(uint8_t point_type);
#define TIMER_INTERVAL 10
#define TIMER_LED_INTERVAL 100
void wifi_Test(void);
void Check_change_uart(void);
void check_modbus_slave(void);

#if  COV
#include "bacnet.h"
<<<<<<< .mine
//#include "cov.h"
=======

>>>>>>> .theirs

BACNET_COV_DATA cov_data;
BACNET_PROPERTY_VALUE value_list;
BACNET_SUBSCRIBE_COV_DATA subscribe_cov;

extern int Send_UCOV_Notify(
    uint8_t * buffer,
    BACNET_COV_DATA * cov_data,
	uint8_t protocal);

extern uint8_t Send_COV_Subscribe(
    uint32_t device_id,
    BACNET_SUBSCRIBE_COV_DATA * cov_data,
		uint8_t protocal);
S16_T put_net_point_value( Point_Net *point, S32_T *val_ptr, S16_T aux, S16_T prog_op , U8_T mode );

void handler_cov_task(uint8_t protocal);
void handler_cov_timer_seconds( uint32_t elapsed_seconds);
uint8_t apdu[480];
extern uint8_t flag_start_scan_network;
void check_cov_data(BACNET_COV_DATA* cov,uint16_t instance, int32_t value)
{
		Point_Net point;
		BACNET_PROPERTY_VALUE *list;
			if (cov == NULL) {
		}

		// check whether it is in network point table

		point.panel = Get_panel_by_deviceid(cov->initiatingDeviceIdentifier);
		point.sub_id = 0; // if network bacnet point
		point.number = instance;
		point.point_type = cov->monitoredObjectIdentifier.type;

		if(cov->monitoredObjectIdentifier.type == OBJECT_ANALOG_VALUE)  // AV
		{
			point.point_type = BAC_AV + 1;
		}
		if(cov->monitoredObjectIdentifier.type == OBJECT_ANALOG_INPUT)  // AI
		{
			point.point_type = BAC_AI + 1;
		}
		if(cov->monitoredObjectIdentifier.type == OBJECT_ANALOG_OUTPUT)  // AO
		{
			point.point_type = BAC_AO + 1;
		}
		if(cov->monitoredObjectIdentifier.type == OBJECT_BINARY_VALUE)  // BV
		{
			point.point_type = BAC_BV + 1;
		}
		if(cov->monitoredObjectIdentifier.type == OBJECT_BINARY_INPUT)  // BI
		{
			point.point_type = BAC_BI + 1;
		}
		if(cov->monitoredObjectIdentifier.type == OBJECT_BINARY_OUTPUT)  // BO
		{
			point.point_type = BAC_BO + 1;
		}

		point.network_number = 0;

		// if current panel is network master
		if(flag_start_scan_network == 1)
		{
			put_net_point_value(&point,&value,0,1,cov->timeRemaining);
		}

}

// update the value subscribed object
// send out UCOV_Notify
void Update_Value_List(uint8_t type, uint32_t instance)
{
	char text[10];
	cov_data_value_list_link(&cov_data, &value_list, 1);
	value_list.propertyIdentifier = PROP_PRESENT_VALUE;
	value_list.propertyArrayIndex = BACNET_ARRAY_ALL;
	if(instance > 0)
		instance = instance - 1;
	switch(type)
	{
		case OBJECT_ANALOG_INPUT:
			if(inputs[instance].range == 0)
				break;
			if(inputs[instance].digital_analog == 1)
			{
				sprintf(text, "%f",(float)inputs[instance].value / 1000);
				bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL, text,
					&value_list.value);
			}
		break;
		case OBJECT_ANALOG_OUTPUT:
			if(outputs[instance].range == 0)
				break;
			if(outputs[instance].digital_analog == 1)
			{
				sprintf(text, "%f",(float)outputs[instance].value / 1000);
				bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL, text,
					&value_list.value);
			}
		break;
		case OBJECT_ANALOG_VALUE:
			if(vars[instance].range == 0)
				break;
			if(vars[instance].digital_analog == 1)
			{
				sprintf(text, "%f",(float)vars[instance].value / 1000);
				bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL, text,
					&value_list.value);
			}
		break;
		case OBJECT_BINARY_INPUT:
			if(inputs[instance].range == 0)
				break;
			if(inputs[instance].digital_analog == 0)
			{
				if(inputs[instance].control == 1)
					bacapp_parse_application_data(BACNET_APPLICATION_TAG_BOOLEAN, "1",	&value_list.value);
				else
					bacapp_parse_application_data(BACNET_APPLICATION_TAG_BOOLEAN, "0",	&value_list.value);
			}
		break;
		case OBJECT_BINARY_OUTPUT:
			if(outputs[instance].range == 0)
				break;
			if(outputs[instance].digital_analog == 0)
			{
				if(outputs[instance].control == 1)
					bacapp_parse_application_data(BACNET_APPLICATION_TAG_BOOLEAN, "1",	&value_list.value);
				else
					bacapp_parse_application_data(BACNET_APPLICATION_TAG_BOOLEAN, "0",	&value_list.value);
			}
		break;
		case OBJECT_BINARY_VALUE:
			if(vars[instance].range == 0)
				break;
			if(vars[instance].digital_analog == 0)
			{
				if(vars[instance].control == 1)
					bacapp_parse_application_data(BACNET_APPLICATION_TAG_BOOLEAN, "1",	&value_list.value);
				else
					bacapp_parse_application_data(BACNET_APPLICATION_TAG_BOOLEAN, "0",	&value_list.value);
			}
		break;
		default:
			bacapp_parse_application_data(BACNET_APPLICATION_TAG_NULL, NULL ,	&value_list.value);
			break;

	}

}

int send_cov_demo(void) {
//	  uint32_t device_id = 12345;

	// Lifetime of the subscription in seconds
    uint32_t lifetime = 600; // 10 minutes
		BACNET_ADDRESS dest;



    // Property ID of the target property
//    BACNET_PROPERTY_ID property_id = PROP_PRESENT_VALUE;


    // Whether the notifications should be confirmed
    bool confirmed = true;
    // Object ID of the target object
//    BACNET_OBJECT_ID object_id;
//    object_id.type = OBJECT_ANALOG_INPUT;
//    object_id.instance = 1;
//
    // Initialize the destination address (example values)

    // Device ID of the target device



	// covdata
	 if(Test[0] == 1001 || Test[0] == 2001 || Test[0] == 3001 || Test[0] == 4001)
	 {
		cov_data.timeRemaining = 60;
		cov_data.subscriberProcessIdentifier = 1;
		cov_data.initiatingDeviceIdentifier = Instance;
		cov_data.monitoredObjectIdentifier.type = OBJECT_ANALOG_VALUE;
		cov_data.monitoredObjectIdentifier.instance = 4;

		cov_data_value_list_link(&cov_data, &value_list, 1);
		value_list.propertyIdentifier = PROP_PRESENT_VALUE;
		value_list.propertyArrayIndex = BACNET_ARRAY_ALL;
		bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL, "25.0",
				&value_list.value);



			//debug_cov("Send_UCOV_Notify");

		Send_UCOV_Notify(apdu,&cov_data,BAC_IP_CLIENT);
		udp_client_send(5);


	}

	if(Test[0] == 1000 || Test[0] == 2000 || Test[0] == 3000 || Test[0] == 4000)
	{
		subscribe_cov.subscriberProcessIdentifier = 1;
    subscribe_cov.monitoredObjectIdentifier.type = Test[11];
		subscribe_cov.monitoredObjectIdentifier.instance = Test[10];
    subscribe_cov.cancellationRequest = false;
    subscribe_cov.issueConfirmedNotifications = 0;  // 0-> ucov  , 1-> cov
    subscribe_cov.lifetime = 60;

    // Send the COV subscription request
		if(Test[0] == 1000)

			Send_COV_Subscribe(212375, &subscribe_cov,BAC_IP_CLIENT);//15770
		if(Test[0] == 5000)
			Send_COV_Subscribe(212375, &subscribe_cov,BAC_IP_CLIENT);//15770

		if(Test[0] == 2000)
			Send_COV_Subscribe(212366, &subscribe_cov,BAC_IP_CLIENT);//15758 10

		if(Test[0] == 3000)
			Send_COV_Subscribe(47085, &subscribe_cov,BAC_IP_CLIENT);//47085

		if(Test[0] == 4000)
			Send_COV_Subscribe(131072, &subscribe_cov,BAC_IP_CLIENT);//0
		Test[44]++;
		udp_client_send(5);
	}
	Test[0] = 0;
  return 0;
}
#endif


void Light_PWM_Init(void);
void Light_PWM_AO_Update(void);
void Light_SW_Init(void);
#if LSW_ON_OFF
void LSW_Control_Sensor(uint16_t on_time, uint16_t off_time);
extern uint16_t LSW_on_time;
extern uint16_t LSW_off_time;

void enable_modem_sleep(void)
{
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM); // 设置为最小功耗模式
}

#include "esp_sleep.h"

void enable_light_sleep(void)
{
    esp_sleep_enable_timer_wakeup(LSW_off_time * 1000000); // 设置 1 秒后唤醒
    esp_light_sleep_start(); // 进入轻睡眠模式
}


void enable_deep_sleep(void)
{
    esp_sleep_enable_timer_wakeup(10000000); // 设置 10 秒后唤醒
    esp_deep_sleep_start(); // 进入深度睡眠模式
}
#endif

void Timer_task(void)
{
	//uint16_t count;
	portTickType xLastWakeTime = xTaskGetTickCount();
	uint16_t count = 0;
	uint16_t count_1s = 0;
	timezone = 800;
	Daylight_Saving_Time = 0;
	if((Modbus.mini_type != PROJECT_FAN_MODULE)&&(Modbus.mini_type != PROJECT_TRANSDUCER)&&(Modbus.mini_type != PROJECT_POWER_METER)
			&&(Modbus.mini_type != PROJECT_MULTIMETER) && (Modbus.mini_type != PROJECT_LSW_BTN) && (Modbus.mini_type != PROJECT_LSW_SENSOR) )
	{

		PCF_hctosys();
		PCF_systohc();
	}
	Eth_IP_Change = 0;

	update_timers();
	system_timer = 0;
	Mstp_ForUs = 0;
	Mstp_NotForUs = 0;
	task_test.enable[13] = 1;
	monitor_init();
	//FOR TEST
	//Rtc_Set(22,4,26,9,40,10,0); // to be deleted
	if(Modbus.mini_type == PROJECT_LIGHT_PWM)
		Light_PWM_Init();

	for (;;)
	{// 10ms
		task_test.count[13]++;

		/*if(Test[20] == 100)
		{
			enable_modem_sleep();
			Test[20] = 10;
		}
		if(Test[20] == 200)
		{	enable_light_sleep();Test[20] = 20;}
		if(Test[20] == 300)
		{	enable_deep_sleep();Test[20] = 30;}

		// for LIGTH_PWM
		if(Modbus.mini_type == PROJECT_LIGHT_PWM)
		{
			Light_PWM_AO_Update();
		}*/




#if COV
		handler_cov_task(BAC_IP_CLIENT);
#endif
		if(Eth_IP_Change == 1)
		{
			if(ip_change_count++ > 5)
			{
			Save_Ethernet_Info();
			Eth_IP_Change = 0;
			esp_retboot();
			}
		}

		if((Mstp_ForUs > 100) && (Mstp_NotForUs > 10))
		{// MSTP error, reboot
			//
		}

		SilenceTime = SilenceTime + TIMER_INTERVAL;
		if(SilenceTime > 10000) // 10s
		{
			SilenceTime = 0;
		}

		// tbd:
		miliseclast = miliseclast + TIMER_INTERVAL;
		system_timer = system_timer + TIMER_INTERVAL;
		//Check_Pulse_Counter();
		if(system_timer % 1000  == 0) // 1000ms,  only for test
		{
			run_time = run_time + 1;
#if LSW_ON_OFF
			if(Modbus.mini_type == PROJECT_LSW_SENSOR)
			{
				if(run_time > 60)
				{
					if((LSW_off_time > 0) && (LSW_on_time > 0))
					{
						if((run_time % LSW_on_time) == 0)
						//LSW_Control_Sensor(LSW_on_time, LSW_off_time);
						{
							//enable_light_sleep();
						}
					}
				}
			}
#endif
			Test[0] = flag_ethernet_initial + 10;
			//if ((Modbus.mini_type != PROJECT_AIRLAB) && (Modbus.mini_type != MINI_TSTAT10))
			if(Modbus.mini_type == MINI_BIG_ARM || Modbus.mini_type == PROJECT_LSW_SENSOR)
			{
				if(flag_ethernet_initial != 0 && run_time > 20)
				{// reboot
					esp_retboot();
				}
			}

#if COV
			handler_cov_timer_seconds(1);
#endif
			if(Modbus.ethernet_status == 4 || SSID_Info.IP_Wifi_Status == 2/*WIFI_NORMAL*/) // got ip
			{
				if(Modbus.com_config[0] == BACNET_MASTER || Modbus.com_config[0] == BACNET_SLAVE || Modbus.com_config[2] == BACNET_MASTER || Modbus.com_config[2] == BACNET_SLAVE)
					flag_start_scan_mstp = 1;
				check_modbus_slave();
			}
			else
				flag_start_scan_mstp = 0;

			if((Modbus.mini_type != PROJECT_FAN_MODULE)&&(Modbus.mini_type != PROJECT_TRANSDUCER)&&(Modbus.mini_type != PROJECT_POWER_METER)
					 && (Modbus.mini_type != PROJECT_LSW_BTN) && (Modbus.mini_type != PROJECT_LSW_SENSOR))
			{
				PCF_GetDateTime(&rtc_date);
				Test[40]++;
				// syc time per hour
				if(rtc_date.minute == 0 && rtc_date.second == 0)
				{Test[41]++;
					PCF_systohc();
				}
			}

			if(count_hold_on_bip_to_mstp > 0)
				count_hold_on_bip_to_mstp--;
			count = 0;

			check_net_health(60);
			Check_change_uart();

		}


		if((system_timer > 20000 / TIMER_INTERVAL) && (flag_clear_count_reboot == 0))
		{ // 20s clear reboot count
			flag_clear_count_reboot = 1;
			save_uint8_to_flash(FLASH_COUNT_REBOOT,0); // clear reboot count
		}

		if(ChangeFlash != 0)
		{
			uint32_t write_delay;
			if(ChangeFlash == 1)
			{
				write_delay = 5;
			}
			else //  ChangeFlash == 2
			{
				write_delay = Modbus.write_flash * 60;
			}

			if(count_write_Flash++ > write_delay * 1000 / TIMER_INTERVAL) // 5s
			{
				save_point_info(0);
				Store_Pulse_Counter(1);
				if(Modbus.write_flash == 0)
					ChangeFlash = 0;
				else
					ChangeFlash = 2;

				count_write_Flash = 0;
			}


		}

		check_task();

		//vTaskDelay(TIMER_INTERVAL / portTICK_RATE_MS);
		vTaskDelayUntil( &xLastWakeTime,TIMER_INTERVAL); // 10ms

	}


}


#define GPIO_STM_RST    	32
#define GPIO_STM_RST_SEL  	(1ULL<<GPIO_STM_RST)

void STM_RST_Init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19

    io_conf.pin_bit_mask = GPIO_STM_RST_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}


#if 1//I2C_TASK
uint8_t led_buf[4];
// check led indicated communication
void Updata_Comm_Led(void)
{
	U8_T temp1 = 0;
	U8_T pre_status1 = led_buf[0];


	temp1 = 0;
	pre_status1 = led_buf[0];

	if(flagLED_main_rx)	{ temp1 |= 0x02;	 	flagLED_main_rx = 0; }
	if(flagLED_main_tx)	{ temp1 |= 0x01;		flagLED_main_tx = 0; }

	if(Modbus.mini_type == MINI_SMALL_ARM || Modbus.mini_type == PROJECT_RMC1216 || Modbus.mini_type == PROJECT_NG2_NEW)
	{
		if(flagLED_ether_rx)	{	temp1 |= 0x08;		flagLED_ether_rx = 0; 	}
		if(flagLED_ether_tx)	{	temp1 |= 0x04;		flagLED_ether_tx = 0;	}

		if(flagLED_sub_rx)	{  temp1 |= 0x20;		flagLED_sub_rx = 0;	}
		if(flagLED_sub_tx)	{  temp1 |= 0x10;		flagLED_sub_tx = 0;}
	}
	else if(Modbus.mini_type == MINI_BIG_ARM)
	{
		if(flagLED_ether_rx)	{	temp1 |= 0x20;		flagLED_ether_rx = 0; 	}
		if(flagLED_ether_tx)	{	temp1 |= 0x10;		flagLED_ether_tx = 0;	}

		if(flagLED_sub_rx)	{  temp1 |= 0x08;		flagLED_sub_rx = 0;	}
		if(flagLED_sub_tx)	{  temp1 |= 0x04;		flagLED_sub_tx = 0;}
	}
	else if(Modbus.mini_type == PROJECT_CO2)
	{
		if(flagLED_sub_rx)	{  temp1 |= 0x20;		flagLED_sub_rx = 0;	}
		if(flagLED_sub_tx)	{  temp1 |= 0x10;		flagLED_sub_tx = 0;}

	}

	led_buf[0] = temp1;
	//if(pre_status1 != CommLed[0])
	//	flag_led_comm_changed = 1;



}

uint8_t InputLed[32];  // high 4 bits - input type, low 4 bits - brightness
uint8_t input_type[32];
uint8_t input_type1[32];
uint8_t OutputLed[24];
uint8_t CommLed[2];
uint8_t flag_read_switch;

#define TEMPER_0_C   191*16
#define TEMPER_10_C   167*16
#define TEMPER_20_C   141*16
#define TEMPER_30_C   115*16
#define TEMPER_40_C   92*16
// update led and input_type
void Update_Led(void)
{
	U8_T loop;
	Str_points_ptr ptr;
//	U8_T index,shift;
//	U32_T tempvalue;
	static U16_T pre_in[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	U8_T error_in = 0; // error of input raw value
	U8_T pre_status = 0;
	U8_T max_in = 0;
	U8_T max_out = 0;
	U8_T max_digout = 0;
	/*    check input led status */

	if(Modbus.mini_type == MINI_BIG_ARM)
	{
		max_in = 32;
		max_out = 24;
		max_digout = 12;
	}
	else if(Modbus.mini_type == MINI_SMALL_ARM)
	{
		max_in = 16;
		max_out = 10;
		max_digout = 6;
	}
	else if(Modbus.mini_type == PROJECT_NG2_NEW)
	{
		max_in = 24;
		max_out = 12;
		max_digout = 8;
	}
	else if(Modbus.mini_type == PROJECT_RMC1216)
	{
		max_in = 16;
		max_out = 7;
		max_digout = 7;
	}
	else if(Modbus.mini_type == MINI_TSTAT10)
	{
		max_in = 8;
		max_out = 7;
		max_digout = 5;
	}


	for(loop = 0;loop < max_in;loop++)
	{
		pre_status = InputLed[loop];
		if(input_raw[loop]  > pre_in[loop])
			error_in = input_raw[loop]  - pre_in[loop];
		else
			error_in = pre_in[loop] - input_raw[loop];
		
		ptr = put_io_buf(IN,loop);

		if(ptr.pin->range == not_used_input)
			InputLed[loop] = 0;
		else
		{
			if(ptr.pin->auto_manual == 0) // auto
			{
				if(ptr.pin->digital_analog == 1) // analog
				{
					if(ptr.pin->range <= 9/*PT1000_200_570DegF*/)	  // temperature
					{	//  10k termistor GREYSTONE
						if(input_raw[loop]  > TEMPER_0_C) 			InputLed[loop] = 0;	   // 0 degree
						else  if(input_raw[loop]  > TEMPER_10_C) 	InputLed[loop] = 1;	// 10 degree
						else  if(input_raw[loop]  > TEMPER_20_C) 	InputLed[loop] = 2;	// 20 degree
						else  if(input_raw[loop]  > TEMPER_30_C) 	InputLed[loop] = 3;	// 30 degree
						else  if(input_raw[loop]  > TEMPER_40_C) 	InputLed[loop] = 4;	// 40 degree
						else
							InputLed[loop] = 5;	   // > 50 degree

					}
					else 	  // voltage or current
					{
						if(input_raw[loop]  < 200) 			InputLed[loop] = 0;
						else  if(input_raw[loop] < 800) 	InputLed[loop] = 1;
						else  if(input_raw[loop] < 1600) 	InputLed[loop] = 2;
						else  if(input_raw[loop] < 2400) 	InputLed[loop] = 3;
						else  if(input_raw[loop] < 3200) 	InputLed[loop] = 4;
						else
							InputLed[loop] = 5;

					}

				}
				else if(ptr.pin->digital_analog == 0) // digtial
				{
					if(( ptr.pin->range >= ON_OFF  && ptr.pin->range <= HIGH_LOW )  // control 0=OFF 1=ON
					||(ptr.pin->range >= custom_digital1 // customer digital unit
					&& ptr.pin->range <= custom_digital8
					&& digi_units[ptr.pin->range - custom_digital1].direct == 1))
					{// inverse
						if(ptr.pin->control == 1) InputLed[loop] = 0;
						else
							InputLed[loop] = 5;
					}
					else
					{
						if(ptr.pin->control == 1) InputLed[loop] = 5;
						else
							InputLed[loop] = 0;
					}
				}
			}
			else // manual
			{
				if(ptr.pin->digital_analog == 0) // digtial
				{
					if(( ptr.pin->range >= ON_OFF  && ptr.pin->range <= HIGH_LOW )  // control 0=OFF 1=ON
					||(ptr.pin->range >= custom_digital1 // customer digital unit
					&& ptr.pin->range <= custom_digital8
					&& digi_units[ptr.pin->range - custom_digital1].direct == 1))
					{ // inverse
						if(ptr.pin->control == 1) InputLed[loop] = 0;
						else
							InputLed[loop] = 5;
					}
					else
					{
						if(ptr.pin->control == 1) InputLed[loop] = 5;
						else
							InputLed[loop] = 0;
					}

				}
				else   // analog
				{
					U32_T tempvalue;
					tempvalue = (ptr.pin->value) / 1000;
					if(ptr.pin->range <= PT1000_200_570DegF)	  // temperature
					{	//  10k termistor GREYSTONE
						if(tempvalue <= 0) 	InputLed[loop] = 0;	   // 0 degree
						else  if(tempvalue < 10) 	InputLed[loop] = 1;	// 10 degree
						else  if(tempvalue < 20) 	InputLed[loop] = 2;	// 20 degree
						else  if(tempvalue < 30) 	InputLed[loop] = 3;	// 30 degree
						else  if(tempvalue < 40) 	InputLed[loop] = 4;	// 40 degree
						else
							InputLed[loop] = 5;	   // > 25 degree
					}
					else 	  // voltage or current
					{
						//InputLed high 4 bits - input type,
						if(ptr.pin->range == V0_5 || ptr.pin->range == P0_100_0_5V)
						{
							if(tempvalue <= 0) 	InputLed[loop] = 0;	   // 0 degree
							else  if(tempvalue <= 1) 	InputLed[loop] = 1;	// 10 degree
							else  if(tempvalue <= 2) 	InputLed[loop] = 2;	// 20 degree
							else  if(tempvalue <= 3) 	InputLed[loop] = 3;	// 30 degree
							else  if(tempvalue <= 4) 	InputLed[loop] = 4;	// 40 degree
							else
								InputLed[loop] = 5;	   // > 50 degree
						}
						if(ptr.pin->range == I0_20ma)
						{
							if(tempvalue <= 4) 	InputLed[loop] = 0;	   // 0 degree
							else  if(tempvalue <= 7) 	InputLed[loop] = 1;	// 10 degree
							else  if(tempvalue <= 10) 	InputLed[loop] = 2;	// 20 degree
							else  if(tempvalue <= 14) 	InputLed[loop] = 3;	// 30 degree
							else  if(tempvalue <= 18) 	InputLed[loop] = 4;	// 40 degree
							else
								InputLed[loop] = 5;	   // > 50 degree
						}
						if(ptr.pin->range == V0_10_IN || ptr.pin->range == P0_100_0_10V)
						{
							if(tempvalue <= 0) 	InputLed[loop] = 0;	   // 0 degree
							else  if(tempvalue <= 2) 	InputLed[loop] = 1;	// 10 degree
							else  if(tempvalue <= 4) 	InputLed[loop] = 2;	// 20 degree
							else  if(tempvalue <= 6) 	InputLed[loop] = 3;	// 30 degree
							else  if(tempvalue <= 8) 	InputLed[loop] = 4;	// 40 degree
							else
								InputLed[loop] = 5;	   // > 50 degree
						}

					}
				}
			}
		}

		/*if(pre_status != InputLed[loop] && error_in > 200)
		{  //  error is larger than 20, led of input is changed
			flag_led_in_changed = 1;
			re_send_led_in = 0;
		}*/
		pre_in[loop] = input_raw[loop];

		//if(Setting_Info.reg.pro_info.firmware_rev >= 14)
		{
			InputLed[loop] &= 0x0f;
			if(input_type[loop] >= 1)
				InputLed[loop] |= ((input_type[loop] - 1) << 4);
			else
				InputLed[loop] |= (input_type[loop] << 4);
		}
	}

	if(flag_read_switch == 1)
	{
		/*    check output led status */
		for(loop = 0;loop < max_out;loop++)
		{
			ptr = put_io_buf(OUT,loop);
	//		OutputLed[loop] = loop / 4;

			pre_status = OutputLed[loop];
			if(ptr.pout->switch_status == SW_AUTO)
			{
				if(ptr.pout->range == 0)
				{
					OutputLed[loop] = 0;
				}
				else
				{
					if(loop < max_digout)	  // digital
					{
						if(ptr.pout->value == 0) OutputLed[loop] = 0;
						else
							OutputLed[loop] = 5;
					}
					else
					{// hardwar AO
						if(ptr.pout->digital_analog == 1)
						{
							if(output_raw[loop] >= 0 && output_raw[loop] < 50 )
								OutputLed[loop] = 0;
							else if(output_raw[loop] >= 50 && output_raw[loop] < 200 )
								OutputLed[loop] = 1;
							else if(output_raw[loop] >= 200 && output_raw[loop] < 400 )
								OutputLed[loop] = 2;
							else if(output_raw[loop] >= 400 && output_raw[loop] < 600 )
								OutputLed[loop] = 3;
							else if(output_raw[loop] >= 600 && output_raw[loop] < 800 )
								OutputLed[loop] = 4;
							else if(output_raw[loop] >= 800 && output_raw[loop] < 1023 )
								OutputLed[loop] = 5;
						}
						else
						{  // AO is used for DO
							if(( ptr.pout->range >= ON_OFF  && ptr.pout->range <= HIGH_LOW )  // control 0=OFF 1=ON
							||(ptr.pout->range >= custom_digital1 // customer digital unit
							&& ptr.pout->range <= custom_digital8
							&& digi_units[ptr.pout->range - custom_digital1].direct == 1))
							{ // inverse
								if(ptr.pout->control == 1) OutputLed[loop] = 0;
								else
									OutputLed[loop] = 5;
							}
							else
							{
								if(ptr.pout->control == 1) OutputLed[loop] = 5;
								else
									OutputLed[loop] = 0;
							}
						}
					}
				}
			}
			else if(ptr.pout->switch_status == SW_OFF)		OutputLed[loop] = 0;
			else if(ptr.pout->switch_status == SW_HAND)
			{
				if(ptr.pout->range != 0)
					OutputLed[loop] = 5;
			}

			/*if(pre_status != OutputLed[loop])
			{
				flag_led_out_changed = 1;
				re_send_led_out = 0;
			}*/
		}
	}
/*	else
	{ // turn off all outputs before switch status are ready
		for(loop = 0;loop < max_out;loop++)
			OutputLed[loop] = 0;
	}*/

	/*    check communication led status */

	Updata_Comm_Led();
}

uint16_t adjust_output(uint16_t output)
{
	if(output <= 100)
		return output * 3 / 5;
	else if(output <= 200)
		return output * 5 / 7;
	else if(output <= 300)
		return output * 58 / 75;
	else if(output <= 400)
		return output * 81 / 100;
	else
		return output * 26 / 29;
}

uint8 flag_internal_temperature = 1;
extern QueueHandle_t qKey;
uint8_t i2c_send_buf[100];
uint8_t i2c_rcv_buf[200];
uint8_t lastSequenceNumber = 0xFF; // Initialize to an invalid value
extern uint16_t count_lcd_time_off_delay;
void lcd_back_set(uint8_t status);

void reboot_sub_chip(void)
{
	gpio_set_level(GPIO_NUM_32, 0);
	usleep(100000); // 500ms
	gpio_set_level(GPIO_NUM_32, 1);
	Test[11]++;
}
void i2c_master_task(void)
{
	Str_points_ptr ptr;
	uint8 index = 0;

	uint8 led_sub_tx_backup = 0;
	uint8 led_sub_rx_backup = 0;
	uint8 led_main_tx_backup = 0;
	uint8 led_main_rx_backup = 0;
	uint8_t top_hardware = 0;
	uint8_t top_firmware = 0;
	uint32_t multiMeterChannelvalue;
#if 1
	// RESET LED chip IO32
	//if(Modbus.mini_type == MINI_SMALL_ARM || Modbus.mini_type == MINI_BIG_ARM)
	{
		i2c_master_init();
		STM_RST_Init();
		gpio_set_level(GPIO_NUM_32, 0);
		usleep(100000); // 500ms
		gpio_set_level(GPIO_NUM_32, 1);
	}
#endif
	if(Modbus.mini_type == PROJECT_CO2)
	{
		qSendCo2 = xQueueCreate(2, 2);

	}
	/*if(Modbus.mini_type == MINI_TSTAT10)
	{
		ptr = put_io_buf(IN,8);  // temperature
		if(ptr.pin->range == 0)
			ptr.pin->range = R10K_40_120DegC;
		memcpy(ptr.pin->label,"TEMP",strlen("TEMP"));

		ptr = put_io_buf(IN,9);  // VOC
		//if(ptr.pin->range == 0)
		//	ptr.pin->range = R10K_40_120DegC;
		memcpy(ptr.pin->label,"TVOC",strlen("TVOC"));

		ptr = put_io_buf(IN,10);
		if(ptr.pin->range == 0)
			ptr.pin->range = Humidty;
		memcpy(ptr.pin->label,"HUM",strlen("HUM "));

		ptr = put_io_buf(IN,11);
		ptr.pin->digital_analog = 0;
		if(ptr.pin->range == 0)
			ptr.pin->range = UNOCCUPIED_OCCUPIED;
		memcpy(ptr.pin->label,"OCC_UOCC",strlen("OCC_UOCC"));

		ptr = put_io_buf(IN,12);
		//if(ptr.pin->range == 0)
		//	ptr.pin->range = R10K_40_120DegC;
		memcpy(ptr.pin->label,"CO2",strlen("CO2"));

		ptr = put_io_buf(IN,13);
		if(ptr.pin->range == 0)
			ptr.pin->range = LUX;
		memcpy(ptr.pin->label,"LUX",strlen("LUX"));
	}*/
	if(Modbus.mini_type == PROJECT_RMC1216)
	{
		ptr = put_io_buf(IN,16);
		if(ptr.pin->range == 0)
			ptr.pin->range = R10K_40_120DegC;
		memcpy(ptr.pin->label,"TEMP1",strlen("TEMP1"));
		ptr = put_io_buf(IN,17);
		if(ptr.pin->range == 0)
			ptr.pin->range = Humidty;
		memcpy(ptr.pin->label,"HUM1",strlen("HUM1"));
		ptr = put_io_buf(IN,18);
		if(ptr.pin->range == 0)
			ptr.pin->range = R10K_40_120DegC;
		memcpy(ptr.pin->label,"TEMP2",strlen("TEMP2"));
		ptr = put_io_buf(IN,19);
		if(ptr.pin->range == 0)
			ptr.pin->range = Humidty;
		memcpy(ptr.pin->label,"HUM2",strlen("HUM2"));
		ptr = put_io_buf(IN,20);
		if(ptr.pin->range == 0)
			ptr.pin->range = R10K_40_120DegC;
		memcpy(ptr.pin->label,"TEMP3",strlen("TEMP3"));
		ptr = put_io_buf(IN,21);
		if(ptr.pin->range == 0)
			ptr.pin->range = Humidty;
		memcpy(ptr.pin->label,"HUM3",strlen("HUM3"));
	}

	if(Modbus.mini_type == PROJECT_NG2_NEW)
	{
		ptr = put_io_buf(IN,24);
		if(ptr.pin->range == 0)
		{
			ptr.pin->range = R10K_40_120DegC;
			memcpy(ptr.pin->label,"TEMP1",strlen("TEMP1"));
		}
		ptr = put_io_buf(IN,25);
		if(ptr.pin->range == 0)
		{
			ptr.pin->range = Humidty;
			memcpy(ptr.pin->label,"HUM1",strlen("HUM1"));
		}
		ptr = put_io_buf(IN,26);
		if(ptr.pin->range == 0)
		{
			ptr.pin->range = R10K_40_120DegC;
			memcpy(ptr.pin->label,"TEMP2",strlen("TEMP2"));
		}
		ptr = put_io_buf(IN,27);
		if(ptr.pin->range == 0)
		{
			ptr.pin->range = Humidty;
			memcpy(ptr.pin->label,"HUM2",strlen("HUM2"));
		}
		ptr = put_io_buf(IN,28);
		if(ptr.pin->range == 0)
		{
			ptr.pin->range = Frequence;
			memcpy(ptr.pin->label,"FEQ1",strlen("FEQ1"));
		}
		ptr = put_io_buf(IN,29);
		if(ptr.pin->range == 0)
		{
			ptr.pin->range = Frequence;
			memcpy(ptr.pin->label,"FEQ2",strlen("FEQ2"));
		}
	}
	task_test.enable[0] = 1;
	i2c_send_buf[0] = i2c_send_buf[1] = i2c_send_buf[2] = i2c_send_buf[3] = 0;

	for (;;)
	{
		//if(Test[42] != 0)
		//	ethernet_init();
		task_test.count[0]++;


		if(Modbus.mini_type == PROJECT_TSTAT9)
		{
			i2c_send_buf[0] = 0x55;
			i2c_send_buf[1]++;
			i2c_send_buf[2] = 24;
			i2c_send_buf[3] = 35;
			LED_i2c_write(0x74,i2c_send_buf,4);
			vTaskDelay(500 / portTICK_RATE_MS);
		}
		else if(Modbus.mini_type == MINI_NANO)
		{
			led_buf[0] = 0x55;
			// send out led status
			if(led_main_tx != led_main_tx_backup)
			{
				led_buf[1] |= 0x01;
				led_main_tx_backup = led_main_tx;
			}
			else
				led_buf[1] &= 0xfe;

			if(led_main_rx != led_main_rx_backup)
			{
				led_buf[1] |= 0x02;
				led_main_rx_backup = led_main_rx;
			}
			else
				led_buf[1] &= 0xfd;

			if(led_sub_tx != led_sub_tx_backup)
			{
				led_buf[1] |= 0x04;
				led_sub_tx_backup = led_sub_tx;
			}
			else
				led_buf[1] &= 0xfb;

			if(led_sub_rx != led_sub_rx_backup)
			{
				led_buf[1] |= 0x08;
				led_sub_rx_backup = led_sub_rx;
			}
			else
				led_buf[1] &= 0xf7;


			LED_i2c_write(0x74,led_buf,4);
			vTaskDelay(500 / portTICK_RATE_MS);
		}
		else if(Modbus.mini_type == MINI_SMALL_ARM || Modbus.mini_type == MINI_BIG_ARM || Modbus.mini_type == PROJECT_RMC1216
				|| Modbus.mini_type == MINI_TSTAT10 || Modbus.mini_type == PROJECT_NG2_NEW || Modbus.mini_type == PROJECT_CO2)
		{
			// send
			// led
			if(index++ % 2 == 0)
			{
				{
					Update_Led();
					i2c_send_buf[0] = led_buf[0];
					i2c_send_buf[1] = Modbus.mini_type;

					memcpy(&i2c_send_buf[2],OutputLed,24);
					memcpy(&i2c_send_buf[26],InputLed,32);
					if(Modbus.mini_type == PROJECT_CO2 )
					{
						uint8_t kk;
						// 100 以前的寄存器，about基础信息的修改
						for(kk = 0;kk < 4;kk++)
						{
							i2c_send_buf[2 + kk] = Modbus.ip_addr[kk];
							i2c_send_buf[6 + kk] = Modbus.subnet[kk];
							i2c_send_buf[10 + kk] = Modbus.getway[kk];
							i2c_send_buf[14 + kk] = SSID_Info.ip_addr[kk];
							i2c_send_buf[18 + kk] = SSID_Info.net_mask[kk];
							i2c_send_buf[22 + kk] = SSID_Info.getway[kk];
						}

						kk = 26;
						i2c_send_buf[kk++] = Modbus.address;
						i2c_send_buf[kk++] = Modbus.protocal;
						i2c_send_buf[kk++] = Modbus.baudrate[0];
						i2c_send_buf[kk++] = SSID_Info.IP_Wifi_Status;
						i2c_send_buf[kk++] = SSID_Info.rssi;

						i2c_send_buf[kk++] = vars[0].value >> 8;
						i2c_send_buf[kk++] = vars[0].value;
						i2c_send_buf[kk++] = vars[1].value >> 8;
						i2c_send_buf[kk++] = vars[1].value;
						i2c_send_buf[kk++] = vars[2].value >> 8;
						i2c_send_buf[kk++] = vars[2].value;
						if(vars[0].range == 0)
							vars[0].range = R10K_40_120DegC;
						i2c_send_buf[kk++] = vars[0].range;
						if(vars[1].range == 0)
							vars[1].range = Humidty;
						i2c_send_buf[kk++] = vars[1].range;
						if(vars[1].range == 0)
							vars[2].range = CO2_PPM;
						i2c_send_buf[kk++] = vars[2].range;

						//写 100之后 的寄存器处理，有关 sensor的操作
						kk = 50;
						i2c_send_buf[kk++] = flag_write_i2c;
						i2c_send_buf[kk++] = CO2_modbus_Addr >> 8;
						i2c_send_buf[kk++] = CO2_modbus_Addr;
						i2c_send_buf[kk++] = CO2_modbus_value >> 8;
						i2c_send_buf[kk++] = CO2_modbus_value;

						if(xQueueReceive(qSendCo2, &flag_write_i2c, 5) == pdTRUE)
						{
							if(flag_write_i2c == 1)
							{// check Whetehr write ok
								Test[20]++;
								Test[21] = CO2_modbus_Addr;
								Test[22] = CO2_modbus_value;
								if(check_write_co2(CO2_modbus_Addr,CO2_modbus_value))
								{Test[23]++;
									flag_write_i2c = 0;
								}
							}
						}


					}
					else if(Modbus.mini_type == MINI_SMALL_ARM )
					{
						for(uint8_t kk = 0;kk < 6;kk++)
						{
							i2c_send_buf[64 + kk] = output_raw[kk] > 512 ? 1 :0;
						}
						for(uint8_t kk = 0;kk < 4;kk++)
						{
							i2c_send_buf[70 + kk] = (output_raw[kk + 6]) / 4;
						}
					}
					else if(Modbus.mini_type == PROJECT_NG2_NEW)
					{
						for(uint8_t kk = 0;kk < 8;kk++)
						{
							i2c_send_buf[64 + kk] = output_raw[kk] > 512 ? 1 :0;
						}
						for(uint8_t kk = 0;kk < 4;kk++)
						{
							i2c_send_buf[72 + kk * 2] = output_raw[kk + 8] >> 8;
							i2c_send_buf[72 + kk * 2 + 1] = output_raw[kk + 8];
						}
					}
					else if(Modbus.mini_type == PROJECT_RMC1216)
					{
						for(uint8_t kk = 0;kk < 7;kk++)
						{
							i2c_send_buf[64 + kk] = output_raw[kk] > 512 ? 1 :0;
						}
					}
					else if(Modbus.mini_type == MINI_TSTAT10)
					{
					//ESP --> ARM  79bytes
					//1 + 1 + 24 + 32 + 6 + 10 + 6
					// 1 - communication led
					// 1 - mini_type
					// 24 - led of outputs status
					// 32 - inputs status, types and led-> first 4 bit for inputs type, last 4 bits for input led status,added in new hardware with INPUT moudle
					// 6 -> reserved
					// 10  -> output value
					// 5 -> reserved
						for(uint8_t kk = 0;kk < 5;kk++)
						{
							i2c_send_buf[64 + kk] = output_raw[kk] > 512 ? 1 :0;
						}
						for(uint8_t kk = 5;kk < 7;kk++)
						{
							i2c_send_buf[64 + kk] = output_raw[kk] / 4;
						}
					}
					else if(Modbus.mini_type == MINI_BIG_ARM)
					{
						for(uint8_t kk = 0;kk < 12;kk++)
						{
							i2c_send_buf[64 + kk] = output_raw[kk] > 512 ? 1 :0;
						}
						for(uint8_t kk = 0;kk < 12;kk++)
						{
							i2c_send_buf[76 + kk] = output_raw[kk + 12] / 4;
						}
					}


					if(gIdentify)
					{
						if(count_gIdentify++ % 2 == 0)
							memset(&i2c_send_buf[2],0,56);
						else
							memset(&i2c_send_buf[2],5,56);
						if(count_gIdentify > 10)
						{
							gIdentify = 0;
							count_gIdentify = 0;
						}
					}

					if(Modbus.mini_type == MINI_BIG_ARM)
					{
						Test[28]++;
						Test[29] = stm_i2c_write(S_ALL_NEW,i2c_send_buf,89);
					}
					else
					{
						// new NG2 have more IO, send length is bigger
						if(Modbus.mini_type == PROJECT_CO2)
						{
							//if(Test[11] == 100)  // ONLY FOR CO2 TEST
							{
								uint8_t ret = stm_i2c_write(S_ALL_NEW,i2c_send_buf,79);
								Test[6]++;
								if(ret != 0)
									Test[7]++;
							}
						}
						else
						{
							if(top_firmware < 8)
							{
								uint8_t ret = stm_i2c_write(S_ALL_NEW,i2c_send_buf,79);
								Test[6]++;
								if(ret != 0)
									Test[7]++;

							}
							else
							{
								uint8_t ret = stm_i2c_write(S_ALL_NEW,i2c_send_buf,81);
								Test[6]++;
								if(ret != 0)
									Test[7]++;
							}
						}
					}
				}

			}
			else if(index > 3)
			{
			// rcv
				{
					uint8_t i = 0;
					uint32_t temp = 0;
					static uint8_t err = 0;
					int ret = 0;

					{
						if(Modbus.mini_type == PROJECT_RMC1216 || Modbus.mini_type == PROJECT_NG2_NEW)
						{
							u16 crc_check;

							ret = stm_i2c_read(G_ALL_NEW,&i2c_rcv_buf,114);
							if(ret == 0)
								err = 0;
							else
							{
								if(err++ > 20)
								{
									err = 0;
									reboot_sub_chip();
								}
							}
							crc_check = crc16(i2c_rcv_buf, 114 - 2);

							if((HIGH_BYTE(crc_check) == i2c_rcv_buf[112]) && (LOW_BYTE(crc_check) == i2c_rcv_buf[113]))
							{// for TCU_NG2_TOP
								// check whether i2c error
								Test[12]++;
								for(i = 0;i < 12;i++)
								{
									ptr = put_io_buf(OUT,i);
									ptr.pout->switch_status = 1;//i2c_rcv_buf[i];
									//check_output_priority_HOA(i);??????????????
									flag_read_switch = 1;
								}

								if((i2c_rcv_buf[0] == 0) && (i2c_rcv_buf[1] == 0) && (i2c_rcv_buf[3] == 0) && (i2c_rcv_buf[3] == 0))
								{// no used
									Test[13]++;
									ptr = put_io_buf(IN,16);
									ptr.pin->value = -40000;
									ptr = put_io_buf(IN,17);
									ptr.pin->value = 0;
								}
								else if(i2c_rcv_buf[0] == 0x55 && i2c_rcv_buf[1] == 0xaa)
								{  // hardware >= 6
									Test[14]++;
									top_hardware = i2c_rcv_buf[2];
									top_firmware = i2c_rcv_buf[3];
								}
								else
								{// no used
									Test[15]++;
									ptr = put_io_buf(IN,16);
									ptr.pin->value = (i2c_rcv_buf[0] * 256 + i2c_rcv_buf[1]) * 100;
									ptr = put_io_buf(IN,17);
									ptr.pin->value = (i2c_rcv_buf[2] * 256 + i2c_rcv_buf[3]) * 100;
								}

								for(i = 0;i < 48 / 2;i++)	  // 88 == 24+64
								{
									//temp = Filter(i,(U16_T)(i2c_rcv_buf[i * 2 + 1 + 24] + i2c_rcv_buf[i * 2 + 24] * 256));
									//uint16 temp1 = i2c_rcv_buf[i * 2 + 1 + 24] + i2c_rcv_buf[i * 2 + 24] * 256;
									temp = i2c_rcv_buf[i * 2 + 1 + 24] + (U16_T)i2c_rcv_buf[i * 2 + 24] * 256;
									if((temp > 0) && (temp < 4200))
									//if(temp != 0xffff)
									{// rev42 of top is 12U8_T, older rev is 10U8_T
										//temp = Filter(i,temp);
										//input_raw[i] = temp;//* input_cal[i] / 4095;
										//
										if(input_cal[0] != 0)
											temp = temp * 4095 / input_cal[0];
										temp = Filter(i,temp);
										input_raw[i] = temp;
									}
									else
										Test[29]++;
								}

								if(Modbus.mini_type == PROJECT_RMC1216)
								{Test[16]++;
									for(i = 0;i < 6;i++)	  //6  high spd counter
									{
										if(top_hardware >= 6)
										{
											if((i2c_rcv_buf[88] == 0) && (i2c_rcv_buf[89] == 0) && (i2c_rcv_buf[90] == 0) && (i2c_rcv_buf[91] == 0))
											{
												ptr = put_io_buf(IN,16);
												ptr.pin->value = -40000;
												ptr = put_io_buf(IN,17);
												ptr.pin->value = 0;
											}
											else
											{
												ptr = put_io_buf(IN,16);
												ptr.pin->value = (i2c_rcv_buf[88] * 256 + i2c_rcv_buf[89]) * 100;
												ptr = put_io_buf(IN,17);
												ptr.pin->value = (i2c_rcv_buf[90] * 256 + i2c_rcv_buf[91]) * 100;
											}
											if((i2c_rcv_buf[92] == 0) && (i2c_rcv_buf[93] == 0) && (i2c_rcv_buf[94] == 0) && (i2c_rcv_buf[95] == 0))
											{
												ptr = put_io_buf(IN,18);
												ptr.pin->value = -40000;
												ptr = put_io_buf(IN,19);
												ptr.pin->value = 0;
											}
											else
											{
												ptr = put_io_buf(IN,18);
												ptr.pin->value = (i2c_rcv_buf[92] * 256 + i2c_rcv_buf[93]) * 100;
												ptr = put_io_buf(IN,19);
												ptr.pin->value = (i2c_rcv_buf[94] * 256 + i2c_rcv_buf[95]) * 100;
											}
											if((i2c_rcv_buf[96] == 0) && (i2c_rcv_buf[97] == 0) && (i2c_rcv_buf[98] == 0) && (i2c_rcv_buf[99] == 0))
											{
												ptr = put_io_buf(IN,20);
												ptr.pin->value = -40000;
												ptr = put_io_buf(IN,21);
												ptr.pin->value = 0;
											}
											else
											{
												ptr = put_io_buf(IN,20);
												ptr.pin->value = (i2c_rcv_buf[96] * 256 + i2c_rcv_buf[97]) * 100;
												ptr = put_io_buf(IN,21);
												ptr.pin->value = (i2c_rcv_buf[98] * 256 + i2c_rcv_buf[99]) * 100;
											}

										}
										/*temp = i2c_rcv_buf[i * 4 + 88] + ((U16_T)i2c_rcv_buf[i * 4 + 89] * 256) \
										 + ((U32_T)i2c_rcv_buf[i * 4 + 90] << 16) + ((U32_T)i2c_rcv_buf[i * 4 + 91] << 24);*/
									}
								}
								if(Modbus.mini_type == PROJECT_NG2_NEW)
								{
									for(i = 0;i < 4;i++)	  //6  high spd counter
									{
										if(top_hardware >= 6)
										{
											if((i2c_rcv_buf[88] == 0) && (i2c_rcv_buf[89] == 0) && (i2c_rcv_buf[90] == 0) && (i2c_rcv_buf[91] == 0))
											{
												ptr = put_io_buf(IN,24);
												ptr.pin->range = R10K_40_120DegC;
												ptr.pin->value = -40000;
												ptr = put_io_buf(IN,25);
												ptr.pin->range = Humidty;
												ptr.pin->value = 0;
											}
											else
											{
												ptr = put_io_buf(IN,24);
												ptr.pin->range = R10K_40_120DegC;
												ptr.pin->value = (i2c_rcv_buf[88] * 256 + i2c_rcv_buf[89]) * 100;
												ptr = put_io_buf(IN,25);
												ptr.pin->range = Humidty;
												ptr.pin->value = (i2c_rcv_buf[90] * 256 + i2c_rcv_buf[91]) * 100;
											}
											if((i2c_rcv_buf[92] == 0) && (i2c_rcv_buf[93] == 0) && (i2c_rcv_buf[94] == 0) && (i2c_rcv_buf[95] == 0))
											{
												ptr = put_io_buf(IN,26);
												ptr.pin->range = R10K_40_120DegC;
												ptr.pin->value = -40000;
												ptr = put_io_buf(IN,27);
												ptr.pin->range = Humidty;
												ptr.pin->value = 0;
											}
											else
											{
												ptr = put_io_buf(IN,26);
												ptr.pin->range = R10K_40_120DegC;
												ptr.pin->value = (i2c_rcv_buf[92] * 256 + i2c_rcv_buf[93]) * 100;
												ptr = put_io_buf(IN,27);
												ptr.pin->range = Humidty;
												ptr.pin->value = (i2c_rcv_buf[94] * 256 + i2c_rcv_buf[95]) * 100;
											}
											// HSP COUNTER
											ptr = put_io_buf(IN,28);
											ptr.pin->range = Frequence;
											ptr.pin->value = (((U32_T)i2c_rcv_buf[100] << 24) + ((U32_T)i2c_rcv_buf[101] << 16) + ((U16_T)i2c_rcv_buf[102] << 8) + i2c_rcv_buf[103]) * 1000;
											ptr = put_io_buf(IN,29);
											ptr.pin->range = Frequence;
											ptr.pin->value = (((U32_T)i2c_rcv_buf[104] << 24) + ((U32_T)i2c_rcv_buf[105] << 16) + ((U16_T)i2c_rcv_buf[106] << 8) + i2c_rcv_buf[107]) * 1000;

										}
										/*temp = i2c_rcv_buf[i * 4 + 88] + ((U16_T)i2c_rcv_buf[i * 4 + 89] * 256) \
										 + ((U32_T)i2c_rcv_buf[i * 4 + 90] << 16) + ((U32_T)i2c_rcv_buf[i * 4 + 91] << 24);*/
									}
								}
							}

						}
						else if(Modbus.mini_type == MINI_TSTAT10)
						{
							u16 crc_check;
							u16 temp_key = 0;
							static uint32_t key_refresh_timer_last = 0;
							uint32_t key_refresh_timer = 0;

							uint32_t sample = 0;
							uint16_t co2_raw = 0;
							//	ARM -> ESP 114bytes
							//4 + 2 + 18 + 32 + 56 + 2
							// 4 -  0x55 0xaa TOP_HARDWARE    TOP_FIRMWARE
							// 2 - KEY
							// 18 - reserved
							//  32 - input AD_value
							// 56- reserved
							// 2 - crc
							ret = stm_i2c_read(G_ALL_NEW,&i2c_rcv_buf,114);

							if(ret == 0)
								err = 0;
							else
							{
								if(err++ > 20)
								{
									err = 0;
									reboot_sub_chip();
								}
							}
							crc_check = crc16(i2c_rcv_buf, 114 - 2);

							if((HIGH_BYTE(crc_check) == i2c_rcv_buf[112]) && (LOW_BYTE(crc_check) == i2c_rcv_buf[113]))
							{
								if(i2c_rcv_buf[0] == 0x55 && i2c_rcv_buf[1] == 0xaa)
								{   // hardware >= 6
									uint8_t flag_SHT4X = 0;
									uint8_t flag_SCD40 = 0;
									uint16_t top_runtime = 0;
									uint8_t flag_top_ready = 0;

									top_hardware = i2c_rcv_buf[2];
									top_firmware = i2c_rcv_buf[3];
									chip_info[1] = i2c_rcv_buf[2]; // top hardware
									chip_info[2] = i2c_rcv_buf[3]; // top firmware

									flag_top_ready = 1;
									if(top_firmware >= 7)
									{
										flag_SHT4X = i2c_rcv_buf[6] - 10;
										flag_SCD40 = i2c_rcv_buf[7] - 10;
										top_runtime =  (i2c_rcv_buf[8] << 8) + i2c_rcv_buf[9];
										if(top_runtime == 0)
										{Test[4]++;
											flag_top_ready = 0;
										}
									}
									Test[2] = top_firmware;
									Test[0] = i2c_rcv_buf[8];
									Test[1] = i2c_rcv_buf[9];
									// key
									if(flag_SHT4X > 0 || flag_SCD40 > 0)
									{Test[3]++;
										if(i2c_rcv_buf[44] == 0 && i2c_rcv_buf[45] == 0)
										{Test[5]++;
											flag_top_ready = 0;
										}
									}

									if(flag_top_ready == 1)
									{
									temp_key = (i2c_rcv_buf[4] << 8) + i2c_rcv_buf[5];
									if(temp_key != 0)
									{
										key_refresh_timer = xTaskGetTickCount();
										if(key_refresh_timer - key_refresh_timer_last > 1000)
										{
											key_refresh_timer_last = xTaskGetTickCount();
											xQueueSend(qKey, &temp_key, 0);
											lcd_back_set(1);Test[9]++;
											count_lcd_time_off_delay = 0;
										}
										else
										{
											//temp_key = 0;
											//xQueueSend(qKey, &temp_key, 0);
											//Test[30]++;
											//Test[32] = (key_refresh_timer - key_refresh_timer_last);
										}
									}

									for(i = 0;i < 8;i++)	  // 88 == 24+64
									{

										temp = i2c_rcv_buf[i * 2 + 1 + 24] + (U16_T)i2c_rcv_buf[i * 2 + 24] * 256;

										if((temp > 0) && (temp < 4200))
										{// rev42 of top is 12U8_T, older rev is 10U8_T

											if(input_cal[i] != 0)
												temp = temp * 4095 / input_cal[i];
											temp = Filter(i,temp);
											input_raw[i] = temp;
										}
										else
											Test[29]++;
										Check_Pulse_Counter();

									}

								// in1-in8 common UI
								// input

									ptr = put_io_buf(IN,9); // voc
									ptr.pin->value = (i2c_rcv_buf[42] * 256 + i2c_rcv_buf[43]) * 1000;


									ptr = put_io_buf(IN,10);// humidity
									//ptr.pin->value = (i2c_rcv_buf[44] * 256 + i2c_rcv_buf[45]);
									sample/*ptr.pin->value*/  = (i2c_rcv_buf[44] * 256 + i2c_rcv_buf[45]);
									if( !ptr.pin->calibration_sign )
										sample += 100L * (ptr.pin->calibration_hi * 256 + ptr.pin->calibration_lo);
									else
										sample += -100L * (ptr.pin->calibration_hi * 256 + ptr.pin->calibration_lo);
									ptr.pin->value = sample;

									//flag_internal_temperature = 1;

									if((i2c_rcv_buf[40] * 256 + i2c_rcv_buf[41]) != 0)
									{
										if((i2c_rcv_buf[44] * 256 + i2c_rcv_buf[45]) != 0)
										{
											ptr = put_io_buf(IN,8);

											sample/*ptr.pin->value*/  = (i2c_rcv_buf[40] * 256 + i2c_rcv_buf[41]);
											if( !ptr.pin->calibration_sign )
												sample += 100L * (ptr.pin->calibration_hi * 256 + ptr.pin->calibration_lo);
											else
												sample += -100L * (ptr.pin->calibration_hi * 256 + ptr.pin->calibration_lo);
											ptr.pin->value = sample;

											input_raw[8] = 0;
											flag_internal_temperature = 0;
										}
										else
										{
											flag_internal_temperature = 1;
											temp = (i2c_rcv_buf[40] * 256 + i2c_rcv_buf[41]);
											temp = Filter(i,temp);
											if(input_cal[8] != 0)
												input_raw[8] = temp * 4095 / input_cal[8];
											else
												input_raw[8] = temp;
										}
									}
									else
									{
										temp = (i2c_rcv_buf[40] * 256 + i2c_rcv_buf[41]);

										input_raw[8] = temp;
										flag_internal_temperature = 0;
									}
									co2_raw = i2c_rcv_buf[48] * 256 + i2c_rcv_buf[49];

									ptr = put_io_buf(IN,11); // occ

									 if(abs(i2c_rcv_buf[46] * 256 + i2c_rcv_buf[47] - 15) >= 5) //occupied
									{
										ptr.pin->value = 1000;
										ptr.pin->control = 1;
									}
									else
									{
										ptr.pin->value = 0;
										ptr.pin->control = 0;
									}

									ptr = put_io_buf(IN,12); // co2
									ptr.pin->value = ((uint32_t)co2_raw * 1000);
									ptr = put_io_buf(IN,13); // light
									ptr.pin->value = (i2c_rcv_buf[50] * 256 + i2c_rcv_buf[51]);
									ptr = put_io_buf(IN,14); // voice
									ptr.pin->value = (i2c_rcv_buf[52] * 256 + i2c_rcv_buf[53]);
								}

								}
							}

						}
						else if(Modbus.mini_type == MINI_BIG_ARM || Modbus.mini_type == MINI_SMALL_ARM)
						{
							ret = stm_i2c_read(G_ALL_NEW,&i2c_rcv_buf,100);
							if(ret == 0)
								err = 0;
							else
							{
								if(err++ > 10)
								{
									err = 0;
									reboot_sub_chip();
								}
							}

							if((i2c_rcv_buf[0] == 0x55) && (i2c_rcv_buf[1] == 0xaa))
							{
								if(Modbus.mini_type == MINI_BIG_ARM)
								{
									for(i = 0;i < 24;i++)
									{
										ptr = put_io_buf(OUT,i);
										ptr.pout->switch_status = i2c_rcv_buf[2 + i];
										check_output_priority_HOA(i);
										flag_read_switch = 1;
									}
									for(i = 0;i < 32;i++)	  // 88 == 24+64
									{
										//temp = Filter(i,(U16_T)(i2c_rcv_buf[i * 2 + 1 + 24] + i2c_rcv_buf[i * 2 + 24] * 256));
										temp = i2c_rcv_buf[i * 2 + 1 + 26] + (U16_T)i2c_rcv_buf[i * 2 + 26] * 256;
										//if(temp != 0xffff)
										{// rev42 of top is 12U8_T, older rev is 10U8_T

											input_raw[i] = temp;


										}
									}
								}

								if(Modbus.mini_type == MINI_SMALL_ARM)
								{
									for(i = 0;i < 10;i++)
									{
										ptr = put_io_buf(OUT,i);
										ptr.pout->switch_status = i2c_rcv_buf[i];
										check_output_priority_HOA(i);
										flag_read_switch = 1;
									}
									for(i = 0;i < 32 / 2;i++)	  // 88 == 24+64
									{
										temp = i2c_rcv_buf[i * 2 + 1 + 24] + (U16_T)i2c_rcv_buf[i * 2 + 24] * 256;

										{// rev42 of top is 12U8_T, older rev is 10U8_T

											input_raw[i] = temp;

										}
									}
								}

								Check_Pulse_Counter();
							}

						}
						else if(Modbus.mini_type == PROJECT_CO2)
						{
							u16 crc_check;
							u16 temp_key = 0;
							static uint32_t key_refresh_timer_last = 0;
							uint32_t key_refresh_timer = 0;
							uint32 sample;
							uint16_t co2_raw = 0;

							memset(i2c_rcv_buf,0,114);
							ret = stm_i2c_read(G_ALL_NEW,&i2c_rcv_buf,114);

							if(ret == 0)
								err = 0;
							else
							{
								if(err++ > 10)
								{
									err = 0;
									//reboot_sub_chip();
								}
							}
							crc_check = crc16(i2c_rcv_buf, 114 - 2);

							if((HIGH_BYTE(crc_check) == i2c_rcv_buf[112]) && (LOW_BYTE(crc_check) == i2c_rcv_buf[113]))
							{
								if(i2c_rcv_buf[0] == 0x55 && i2c_rcv_buf[1] == 0xaa)
								{
									uint8 i = 0;
									uint8_t j;
									char str[9];
									// input
									Test[8]++;
									memcpy(&co2_data,&i2c_rcv_buf[2],sizeof(STR_CO2_Reg));
									for(i = 0; i < 3;i++)
									{
										if(co2_data.i2c_sensor_type[i] == E_I2C_SHT4X || co2_data.i2c_sensor_type[i] == E_I2C_SCD4X )
										{Test[9]++;
											ptr = put_io_buf(IN,i * 3);
											//检查 ptr.pin->label 是否为 NULL
											if (ptr.pin->label == NULL) {
												sprintf(str, "TEM%d", i); // 初始化为 "TEMP<j>"
												memcpy(ptr.pin->label, str, 8);
											}
											if(co2_data.deg_c_or_f == 0)
												ptr.pin->range = R10K_40_120DegC;
											else
												ptr.pin->range = R10K_40_250DegF;
											ptr.pin->digital_analog = 1;
											ptr.pin->value = (co2_data.I2C_Sensor[i].tem_org + co2_data.I2C_Sensor[i].tem_offset) * 100;
											if(co2_data.I2C_Sensor[i].tem_offset < 0)
											{
												ptr.pin->calibration_sign = 1; // negtive
												ptr.pin->calibration_hi = (65536 - co2_data.I2C_Sensor[i].tem_offset) >> 8;
												ptr.pin->calibration_lo = (65536 - co2_data.I2C_Sensor[i].tem_offset) ;
											}
											else
											{
												ptr.pin->calibration_sign = 0; // postive
												ptr.pin->calibration_hi = (co2_data.I2C_Sensor[i].tem_offset) >> 8;
												ptr.pin->calibration_lo = (co2_data.I2C_Sensor[i].tem_offset) ;
											}


											ptr = put_io_buf(IN, i * 3 + 1);
											// 检查 ptr.pin->label 是否为 NULL
											if (ptr.pin->label == NULL) {
												sprintf(str, "HUM%d", i); // 初始化为 "HUMI<j>"
												memcpy(ptr.pin->label, str, 8);
											}
											ptr.pin->range = Humidty;
											ptr.pin->digital_analog = 1;
											ptr.pin->value = (co2_data.I2C_Sensor[i].hum_org + co2_data.I2C_Sensor[i].hum_offset) * 100;
											if(co2_data.I2C_Sensor[i].hum_offset < 0)
											{
												ptr.pin->calibration_sign = 1;
												ptr.pin->calibration_hi = (65535 - co2_data.I2C_Sensor[i].hum_offset) >> 8;
												ptr.pin->calibration_lo = (65535 - co2_data.I2C_Sensor[i].hum_offset) ;
											}
											else
											{
												ptr.pin->calibration_sign = 0;
												ptr.pin->calibration_hi = (co2_data.I2C_Sensor[i].hum_offset) >> 8;
												ptr.pin->calibration_lo = (co2_data.I2C_Sensor[i].hum_offset) ;
											}

										}
										if( co2_data.i2c_sensor_type[i] == E_I2C_SCD4X )
										{
											ptr = put_io_buf(IN, i * 3 + 2);
											// 检查 ptr.pin->label 是否为 NULL
											if (ptr.pin->label == NULL) {
												sprintf(str, "CO2%d", i); // 初始化为 "HUMI<j>"
												memcpy(ptr.pin->label, str, 8);
											}

											ptr.pin->range = CO2_PPM;
											ptr.pin->digital_analog = 1;
											ptr.pin->value = (co2_data.I2C_Sensor[i].co2_org) * 1000;
										}
									}


									// output

									for(j = 0;j < 3;j++)
									{
									ptr = put_io_buf(OUT,j);
									if(co2_data.output_mode == E_4_20MA)
										ptr.pout->range = I_0_20ma;
									else if(co2_data.output_mode == E_0_5V)
										ptr.pout->range = V0_10;
									else if(co2_data.output_mode == E_0_10V)
										ptr.pout->range = V0_10;
									else
										ptr.pout->range = 0;
									ptr.pout->digital_analog = 1;
									ptr.pout->value = co2_data.analog_output[j] * 10;
									}


								}

							}

						}
					}

				}
				//vTaskDelay(100 / portTICK_RATE_MS);
			}
			if(Modbus.mini_type == MINI_TSTAT10)
				vTaskDelay(100 / portTICK_RATE_MS);
			else	if(Modbus.mini_type == PROJECT_CO2)
				vTaskDelay(500 / portTICK_RATE_MS);
			else
				vTaskDelay(100 / portTICK_RATE_MS);
		}

	}

}
#endif
// for control task
S16_T exec_program(S16_T current_prg, U8_T *prog_code);
void Check_All_WR(void);
void pid_controller( S16_T p_number );
U8_T check_whehter_running_code(void);
void Check_Net_Point_Table(void);
void TRENLOG_TEST(void);
void check_monitor_sample_points(U8_T i);

#define PID_SAMPLE_COUNT 20
#define PID_SAMPLE_TIME 10

#if 1//EMAIL
extern U8_T flag_sendemail;
extern char send_message[200];
extern U8_T panel_alarm;
void smtp_client_task_nossl(char *);
#endif

void update_sntp(void);
void Bacnet_Control(void)
{
	U16_T i,j;
	U8_T decom;
	portTickType xLastWakeTime = xTaskGetTickCount();
	static U8_T count_wait_sample = 0;
	static U8_T count_PID;
	static U16_T count_schedule;
	if(Setting_Info.reg.webview_json_flash != 2)
	{
		check_graphic_element();
	}
	if(Modbus.mini_type == MINI_BIG_ARM)
	{	//max_dos = BIG_MAX_DOS; max_aos = BIG_MAX_AOS;
		max_dos = 12; max_aos = 12;
	}
	else if(Modbus.mini_type == MINI_SMALL_ARM)
	{	//max_dos = SMALL_MAX_DOS; max_aos = SMALL_MAX_AOS;
		max_dos = 6; max_aos = 4;
	}
	else if(Modbus.mini_type == PROJECT_RMC1216) // RMC1216
	{	//max_dos = SMALL_MAX_DOS; max_aos = SMALL_MAX_AOS;
		max_dos = 7; max_aos = 0;
	}
	else if(Modbus.mini_type == MINI_TSTAT10)
	{	//max_dos = SMALL_MAX_DOS; max_aos = SMALL_MAX_AOS;
		max_dos = 5; max_aos = 2;
	}
	else if(Modbus.mini_type == PROJECT_NG2_NEW)
	{	//max_dos = SMALL_MAX_DOS; max_aos = SMALL_MAX_AOS;
		max_dos = 8; max_aos = 4;
	}
	else if(Modbus.mini_type == MINI_NANO)
	{
		max_dos = 0; max_aos = 0;
	}
	else if(Modbus.mini_type == PROJECT_LIGHT_PWM)
	{
		max_dos = 0; max_aos = 4;
	}

	for(i = 0;i < MAX_OUTS;i++)
	{
		check_output_priority_array(i,0);
#if OUTPUT_DEATMASTER
		clear_dead_master();
#endif
	}
	Check_All_WR();
	task_test.enable[14] = 1;

#if 1//DNS
	flag_Update_Sntp = 0;
	Update_Sntp_Retry = 0;
	count_sntp = 0;
#endif

#if 1//EMAIL
	/*Email_Setting.reg.smtp_ip[0] = 192;
	Email_Setting.reg.smtp_ip[1] = 168;
	Email_Setting.reg.smtp_ip[2] = 0;
	Email_Setting.reg.smtp_ip[3] = 7;
	Email_Setting.reg.smtp_port = 25;
	memcpy(Email_Setting.reg.email_address,"chelsea@temcocontrols.com",26);
	memcpy(Email_Setting.reg.To1Addr,"chelsea@temcocontrols.com",26);
	memcpy(Email_Setting.reg.password,"u6flh?lO",9);*/
	Email_Setting.reg.secure_connection_type = 0; // no ssl
//	Email_Setting.reg.smtp_type = 1;
	flag_sendemail = 0;
#endif
	for(;;)
	{
		task_test.count[14]++;
#if 1//EMAIL
		{
			if(flag_sendemail == 1)
			{
				flag_sendemail = 0;
				smtp_client_task_nossl(send_message);
			}

		}
#endif

#if 1//DNS
		dns_tmr();
		update_sntp();
#endif
		if(((Modbus.mini_type >= MINI_BIG_ARM) && (Modbus.mini_type <=MINI_TINY_11I))
				|| (Modbus.mini_type == PROJECT_RMC1216) || (Modbus.mini_type == PROJECT_NG2_NEW))
		{
			control_input();
		}

		//if(check_whehter_running_code() == 1)
		for( i = 0; i < MAX_PRGS; i++/*, ptr++*/ )
		{
			if( programs[i].bytes )
			{
				if(programs[i].on_off)
				{
					uint32_t t1 = 0;
					uint32_t t2 = 0;
					// add checking running time of program
					t1 = system_timer;
					exec_program( i, prg_code[i]);
					t2 = system_timer;
					programs[i].costtime = (t2 - t1) + 1;
				}
				else
					programs[i].costtime = 0;
			}
			else
					programs[i].costtime = 0;

		}

		if(((Modbus.mini_type >= MINI_BIG_ARM) && (Modbus.mini_type <=MINI_TINY_11I))
				|| (Modbus.mini_type == PROJECT_RMC1216) || (Modbus.mini_type == PROJECT_NG2_NEW))
			control_output();

// check whether external IO are on line
		count_PID++;  // 1s
		if(count_PID >= PID_SAMPLE_COUNT) // 500MS * PID_SAMPLE_COUNT == PID_SAMPLE_TIME 10S
		{
	   // dealwith controller roution per 1 sec
			for(i = 0;i < MAX_CONS; i++)
			{
				pid_controller( i );
			}
			count_PID = 0;
			Store_Pulse_Counter(0);
//			calculate_RPM();

		}

		// dealwith check_weekly_routines per 1 min

		if(count_schedule < 5) 	count_schedule++;
		else
		{
			count_schedule = 0;
			check_weekly_routines();
			check_annual_routines();
		}

		// count monitor task
		if(just_load)
			just_load = 0;

		//if( count_1s++  >= 1)
		{
			miliseclast_cur = miliseclast;
			miliseclast = 0;
		}
//		else
//			count_1s = 0;
#if BAC_TRENDLOG
		check_trendlog_1s(2);
#endif

#if 1//BAC_TRENDLOG
		//trend_log_timer(0); // for standard trend log
#endif
		Check_Net_Point_Table();

		vTaskDelay(1000 / portTICK_RATE_MS);
		//vTaskDelayUntil( &xLastWakeTime,500 );
	}

}

#if 0
// check whehtehr ethenet initial ok, if not , reboot and try 3 timer
// continue if failed after try 3 timers
void Ethernet_Initial(void)
{
#if 1
	esp_err_t ret = 0;
	uint8_t eth_init_count = 0;
	Test[20]++;
	do
	{	Test[21]++;
		ret = ethernet_init();
		ets_delay_us(500000);
	}while((ret != ESP_OK) && (eth_init_count++ < 3));

#if 1
	if(Modbus.mini_type == MINI_SMALL_ARM || Modbus.mini_type == MINI_BIG_ARM || Modbus.mini_type == PROJECT_CO2 || Modbus.mini_type == PROJECT_LSW_SENSOR)
	{
		sprintf(debug_array,"ethernet initial, ret = %u, eth_init_count %u, count_reboot = %u",ret,eth_init_count,count_reboot);
		uart_write_bytes(UART_NUM_0, (const char *)debug_array, strlen(debug_array));
		//Modbus.mini_type = MINI_TSTAT10;

		if(eth_init_count >= 3 && count_reboot < 10)
		{
			esp_retboot();
		}
	}
	else
	{
		if(eth_init_count >= 3 && count_reboot < 3)
		{
			esp_retboot();
		}

	}
#endif

#endif
	//flag_ethernet_initial = ethernet_init();
}
#endif

#define FAN 0
void TEST_FLASH(void);
void vStartScanTask(unsigned char uxPriority);
void i2c_sensor_task(void *arg);
void MenuTask(void *pvParameters);

void LS_led_task(void);

extern void ethernet_check_task( void *pvParameters);
void start_dns_server(void);
#if 0//DDNS
void ddns_task(void *pvParameters);
#endif


void app_main()
{

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */


	SW_REV = SOFTREV;
	count_reboot = 0;

	Bacnet_Initial_Data();
	read_default_from_flash();
	initial_HSP();
	Inital_Bacnet_Server();
	Get_Tst_DB_From_Flash();   // read sub device information from flash memeory

	uart_init(0);

#if 1
    sprintf(debug_array,"app %u, mini_type %u, count_reboot = %u",SOFTREV,Modbus.mini_type,count_reboot);
    uart_write_bytes(UART_NUM_0, (const char *)debug_array, strlen(debug_array));
    //Modbus.mini_type = MINI_TSTAT10;
#endif

    if (Modbus.mini_type != MINI_BIG_ARM)
    	uart_init(2);

   /* if ((Modbus.mini_type != PROJECT_AIRLAB) && (Modbus.mini_type != MINI_TSTAT10))
    {
    	Ethernet_Initial();
    }
    else*/
    {
    	flag_ethernet_initial = ethernet_init();
    }

    xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 1, &main_task_handle[1]);

    network_EventHandle = xEventGroupCreate();
    xTaskCreate(tcp_server_task, "tcp_server", 6000, NULL, 5, &main_task_handle[2]); // tcp server
    // dealing with network modbus point
    xTaskCreate(tcp_client_task, "tcp_client", 6000, NULL, 1, &main_task_handle[3]); // tcp client
    xTaskCreate(udp_scan_task, "udp_scan", 4096, NULL, 1, &main_task_handle[4]); // udp server 1234
    xTaskCreate(bip_task, "bacnet ip", 6000, NULL, 1, &main_task_handle[0]); // udp server 47808
    xTaskCreate(Scan_network_bacnet_Task,"Scan_network_bacnet_Task", 4096, NULL, tskIDLE_PRIORITY + 1, &main_task_handle[16]); // udp client 47808
#if 0//DDNS
    xTaskCreate(ddns_task, "ddns_task", 4096, NULL, 5, NULL);
#endif
    if(Modbus.mini_type == PROJECT_MPPT)
    	mppt_task_init();
    if(Modbus.mini_type == PROJECT_MULTIMETER_NEW)
    	hy3131_spi_init();

    // Check the Modbus mini_type and call uart_init(2) if necessary
    // for T3-BB-ESP, uart(2)初始化放在wifi初始化之后，否则初始失败
	if (Modbus.mini_type == MINI_BIG_ARM)
		  uart_init(2);
	// ok

	if(Modbus.mini_type == PROJECT_LSW_BTN)
	{
		lightswitch_adc_init();
		xTaskCreate(LS_led_task, "led_task", 2048, NULL, 14, NULL);
		xTaskCreate(light_sleep_task, "LightSleepTask", 2048, NULL, 14, NULL);
	}

    if(Modbus.mini_type == MINI_NANO || Modbus.mini_type == PROJECT_TSTAT9 ||  Modbus.mini_type == MINI_SMALL_ARM || Modbus.mini_type == PROJECT_RMC1216
    		|| Modbus.mini_type == MINI_BIG_ARM ||  Modbus.mini_type == MINI_TSTAT10 || Modbus.mini_type == PROJECT_NG2_NEW || Modbus.mini_type == PROJECT_CO2)
    {
    	xTaskCreate(i2c_master_task,"i2c_master_task", 4096, NULL, 10, &main_task_handle[10]);
    }

    // Check if modbus.mini_type is PROJECT_MULTIMETER
    if (Modbus.mini_type == PROJECT_MULTIMETER) {
        // Create multiMeterTask with low priority
        xTaskCreate(multiMeterTask, "MultiMeterTask", 2048*2, NULL, tskIDLE_PRIORITY+5, NULL);
    }
    //if(Modbus.mini_type == PROJECT_TSTAT9 || Modbus.mini_type == PROJECT_AIRLAB)
    //    xTaskCreate(Lcd_task,"lcd_task",2048, NULL, tskIDLE_PRIORITY + 4,&main_task_handle[6]);

    if((Modbus.mini_type == PROJECT_FAN_MODULE) || (Modbus.mini_type == PROJECT_TRANSDUCER) || (Modbus.mini_type == PROJECT_POWER_METER)
    		|| (Modbus.mini_type == PROJECT_AIRLAB) || (Modbus.mini_type == PROJECT_LSW_SENSOR))
       xTaskCreate(i2c_sensor_task,"i2c_task", 2048*2, NULL, 5, NULL);


    if(Modbus.mini_type == PROJECT_AIRLAB)
    {
    	Airlab_init(); // 初始化数据和任务
    	//xTaskCreate(vInputTask,"InputTask",2048, NULL, tskIDLE_PRIORITY + 4,&main_task_handle[7]);
    }

    // AIRLAB and LSW are not controller
    if(Modbus.mini_type == PROJECT_AIRLAB || Modbus.mini_type == PROJECT_LSW_SENSOR)
    {
    	xTaskCreate(vPM25Task,"PM25Task",2048, NULL, tskIDLE_PRIORITY + 1,&main_task_handle[15]);
    }
    else
    	vStartScanTask(5);

    xTaskCreate(Master0_Node_task,"mstp0_task",4096, NULL, 4, &main_task_handle[8]);
    xTaskCreate(uart0_rx_task,"uart0_rx_task",6000, NULL, 11, &main_task_handle[9]);

    if(((Modbus.mini_type >= MINI_BIG_ARM) && (Modbus.mini_type <= MINI_NANO))
    	|| (Modbus.mini_type == PROJECT_RMC1216) || (Modbus.mini_type == PROJECT_NG2_NEW)
		)
    {
	   xTaskCreate(Master2_Node_task,"mstp2_task",4096, NULL, 4, &main_task_handle[11]);
	   xTaskCreate(uart2_rx_task,"uart2_rx_task",4096, NULL, 8, &main_task_handle[12]);
    }// ok

    if(Modbus.mini_type == MINI_TSTAT10 || Modbus.mini_type == PROJECT_AIRLAB)
	{
		Test_Array();
		xTaskCreate(MenuTask,  "MenuTask", 4096, NULL, tskIDLE_PRIORITY + 1,  &main_task_handle[17]);
	}


 #if 1
	xTaskCreate(Bacnet_Control,"BAC_Control_task",6000, NULL, 3, &main_task_handle[14]);
#endif

 #if 1
 	xTaskCreate(Timer_task,"timer_task",6000, NULL, 13, &main_task_handle[13]);

#endif


//	xTaskCreate(smtp_client_task, "smtp_client_task", 2048, NULL, 5, NULL);

}

// for bacnet lib
void uart_send_string(U8_T *p, U16_T length,U8_T port)
{
	/*if((Modbus.com_config[port] == BACNET_SLAVE || Modbus.com_config[port] == BACNET_MASTER) && \
			(flag_mstp_err[port] == 1))
	{
	// mstp error, dont send out data
	return;
	}*/
	if(Modbus.mini_type == PROJECT_FAN_MODULE)
		holding_reg_params.led_rx485_tx = 2;

	if(port == 0)
	{
		uart_write_bytes(UART_NUM_0, (const char *)p, length);
	}
	else if(port == 2)
	{
		uart_write_bytes(UART_NUM_2, (const char *)p, length);
	}

	if(port == 0)	{led_sub_tx++; flagLED_sub_tx = 1;}
	else if(port == 2)	{led_main_tx++; flagLED_main_tx = 1;}

	com_tx[port]++;
}

/*char get_current_mstp_port(void)
{

		return -1;
}*/

U8_T RS485_Get_Baudrate(void)
{
	return 9;//uart2_baudrate;
}


void delay_ms(unsigned int t)
{
	usleep(t * 1000);
}

U8_T Get_Mini_Type(void)
{
	return Modbus.mini_type;
}


void I2C_sensor_Init(void)
{
	i2c_master_init();
	if(Modbus.mini_type == PROJECT_FAN_MODULE)
	{
		holding_reg_params.fan_module_pwm2 = 0;
		led_pwm_init();
		led_init();
		my_pcnt_init();
		adc_init();
	}
	if(Modbus.mini_type == PROJECT_TRANSDUCER)
	{
		led_pwm_init();
		transducer_switch_init();
	}
}


#if 1
int udp_send_sock = -1;
void udp_client_send(uint16 time)
{
    uint8_t rx_buffer[128];
    uint8_t count = 0;
    uint8_t i;
    //char host_ip[] = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;
	struct sockaddr_in dest_addr;
	memcpy(&dest_addr.sin_addr.s_addr,&Send_bip_address,4);
	memcpy(&dest_addr.sin_port,&Send_bip_address[4],2);

	dest_addr.sin_family = AF_INET;
	addr_family = AF_INET;
	ip_protocol = IPPROTO_IP;

	if (udp_send_sock != -1) {
			   // ESP_LOGE(TAG, "Shutting down socket and restarting...");
				shutdown(udp_send_sock, 0);
				close(udp_send_sock);
			}
	udp_send_sock = socket(addr_family, SOCK_DGRAM, ip_protocol);

	if(udp_send_sock > 0)
	{
		int err =
			sendto(udp_send_sock, bip_client_send_buf, bip_client_send_len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	}

	bip_client_send_len = 0;

	if(time == 255)
	{
		count = 10;
		time = 5;
	}
	else
		count = 1;


	for(i = 0;i < count;i++)
	{
		int err = Readable_timeo(udp_send_sock, time);

		if(err > 0)
		{
			struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
			socklen_t socklen = sizeof(source_addr);

			int len = recvfrom(udp_send_sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
							// Error occurred during receiving
			if (len < 0) {
				//ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
				//break;

			}	// Data received
			else {
				BACNET_ADDRESS src; /* source address */
				uint8_t pdu_len;
				uint8_t temp_addr[8];
				memcpy(&temp_addr,&source_addr,8);
				memcpy(&BIP_src_addr[0],&temp_addr[4],4);
				memcpy(&BIP_src_addr[4],&temp_addr[2],2);
				ether_rx += len;
				pdu_len = datalink_receive(&src, &rx_buffer[0], sizeof(rx_buffer) - 1, 0,BAC_IP_CLIENT);
				{
					if(pdu_len)
					{
						npdu_handler(&src, &rx_buffer[0], pdu_len, BAC_IP_CLIENT);
					}
				}

			}
		}
		else
			break;//if(Test[0] == 500)	Test[46]++;
	}

}

U8_T Send_bip_address[6];
void Set_broadcast_bip_address(uint32_t net_address);

uint8_t Master_Scan_Network_Count = 0;
void Send_Time_Sync_To_Network(void);
uint8_t flag_start_scan_network;
uint8_t start_scan_network_count;
U16_T scan_network_bacnet_count = 0;
U8_T flag_send_get_panel_number;
void bip_set_port( uint16_t port);

#if 1
uint8_t NPB_node_write_count = 0;

// put NPB_node to write_buf
void write_NP_Bacnet_to_nodes(uint8_t object_type,uint32_t number,uint8_t panel,uint8_t sub_id,float value)
{

	uint8_t i;
	Test[26]++;
	 // Check if the same data already exists
    for (i = 0; i < NPB_node_write_count; i++)
    {
        if ((NPB_node_write[i].object_type == object_type) &&
					(NPB_node_write[i].num == number) &&
					(NPB_node_write[i].panel == panel) &&
					(NPB_node_write[i].sub_id == sub_id) &&
					(NPB_node_write[i].value == value))
        {
            // Data already exists, return directly to avoid duplication
            return;
        }
    }

    // If the array is full, print an error message and return
    if (NPB_node_write_count >= MAX_NPB_NODE_WRITE)
    {
        //printf("Error: NPB_node_write array is full!\n");
        return;
    }

    // Store the new data in the array
    NPB_node_write[NPB_node_write_count].object_type = object_type;
    NPB_node_write[NPB_node_write_count].num = number;
	NPB_node_write[NPB_node_write_count].panel = panel;
	NPB_node_write[NPB_node_write_count].sub_id = sub_id;
    NPB_node_write[NPB_node_write_count].value = value;
	NPB_node_write[NPB_node_write_count].time_to_live = 50;
	NPB_node_write[NPB_node_write_count].flag = 1;
    // Increment the total entry count
    NPB_node_write_count++;
//	Test[27]++;
//	Test[28] = NPB_node_write_count;

}


void Check_NPB_node_write_TTL(void)
{
		uint8_t i,j;
		for ( i = 0; i < NPB_node_write_count; i++)
    {
        if (NPB_node_write[i].time_to_live > 0)
        {
            NPB_node_write[i].time_to_live--; // Decrement time_to_live

            // If time_to_live reaches 0, handle the expired record
            if (NPB_node_write[i].time_to_live == 0)
            {
//                printf("Record expired: Node ID = %d, Property = %d\n",
//                       NPB_node_write[i].node_id, NPB_node_write[i].property);

                // Option 1: Remove the expired record (shift remaining records)
                for (j = i; j < NPB_node_write_count - 1; j++)
                {
                    NPB_node_write[j] = NPB_node_write[j + 1];
                }
                NPB_node_write_count--; // Decrement the total count
                i--; // Adjust index after removal

                // Option 2: Mark the record as invalid (if removal is not needed)
                // NPB_node_write[i].node_id = 0; // Mark as invalid
            }
        }
    }
}

extern int WriteRemotePoint(uint8_t object_type,uint32_t object_instance,uint8_t panel,uint8_t sub,float value,uint8_t protocal);

void check_NP_Bacnet_to_nodes(void)
{

	uint8_t i;
	for (i = 0;i < NPB_node_write_count;i++)
	{
			if(NPB_node_write[i].flag == 1)
			{
				WriteRemotePoint(NPB_node_write[i].object_type,
					NPB_node_write[i].num,
					NPB_node_write[i].panel,
					NPB_node_write[i].sub_id,
					NPB_node_write[i].value,
					BAC_IP_CLIENT);
				vTaskDelay( 500 / portTICK_RATE_MS);
				NPB_node_write[i].flag = 0;
			}
	}
}
#endif


void Scan_network_bacnet_Task(void)
{
//	portTickType xDelayPeriod = (portTickType)1000 / portTICK_RATE_MS;
	U8_T port = 0;
	U8_T wait_count;
	U8_T network_point_index = 0;
	U8_T send_whois_interval;
	int invoke;
	bip_set_port(Modbus.Bip_port/*UDP_BACNET_LPORT*/);

//	count_send_bip = 0;
	Modbus.network_master = 0;  // ONLY FOR NETWORK TEST
#if 1
	NPB_node_write_count = 0;
	memset(NPB_node_write,0,sizeof(STR_NPB_NODE_OPERATE) * MAX_NPB_NODE_WRITE);
#endif
//	Set_bip_client_reading_flag();
	task_test.enable[16] = 1;

//	Send_Time_Sync_To_Network();
	send_whois_interval = 1;
	scan_network_bacnet_count = 0;
	flag_start_scan_network = 1;
	//bip_set_socket(47808);
	while(1)
	{
		vTaskDelay( 500 / portTICK_RATE_MS);
		task_test.count[16]++;
		Master_Scan_Network_Count++;
		if(Master_Scan_Network_Count >= 180)
		{  	// did not find master
			// current panel is master
			Modbus.network_master = 1;
			Master_Scan_Network_Count = 0;
		}

#if 1
		// 鑴﹂檵鑴曡劊鑴犺矾鍗ゆ嫝铏忕绂勮癌褰曠洸璧備箞鑴よ姦鎴剻
		if(scan_network_bacnet_count > 128)
			scan_network_bacnet_count = 128;

		if(scan_network_bacnet_count % send_whois_interval == 0)  // 1min+
		{
			if(send_whois_interval < 100)
				send_whois_interval *= 2;

			if(flag_start_scan_network == 1)
			{// if no network points, do not scan scan command
				if((start_scan_network_count++ < 2))
				{
					static char tempindex = 0;
					if(tempindex++ % 2 == 0) // broadcast
					{
						Set_broadcast_bip_address(multicast_addr);
						bip_set_broadcast_addr(multicast_addr);
					}
					else
					{
						Set_broadcast_bip_address(0xffffffff);
						bip_set_broadcast_addr(0xffffffff);
					}
					Send_WhoIs(-1,-1,BAC_IP_CLIENT);
					udp_client_send(255);
				}
				else
				{
					start_scan_network_count = 0;
					flag_start_scan_network = 0;
				}
			}

			scan_network_bacnet_count = 0;
		}
		else  // check whether reading remote point
		{
			// check send time sync
			if(flag_send_udp_timesync)
			{
				flag_send_udp_timesync = 0;
				Send_TimeSync_Broadcast(BAC_IP_CLIENT);
				udp_client_send(255);
				Test[38]++;
			}
			else
			{
#if 1
				uint8_t i;
				for(i = 0;i < remote_panel_num;i++)
				{// get panel number from proprietary

					if(((remote_panel_db[i].protocal == BAC_IP) || (remote_panel_db[i].protocal == BAC_IP_CLIENT))
						&& remote_panel_db[i].product_model == 1)
					{
						flag_receive_netp_temcovar = 0;
						Get_address_by_instacne(remote_panel_db[i].device_id,Send_bip_address);
						temcovar_panel_invoke	= Send_Read_Property_Request(remote_panel_db[i].device_id,
						PROPRIETARY_BACNET_OBJECT_TYPE,1,/* panel number is proprietary 1*/
						PROP_PRESENT_VALUE,0,BAC_IP_CLIENT);
						temcovar_panel = 0;

						if(temcovar_panel_invoke >= 0)
						{
							udp_client_send(5);
							remote_panel_db[i].retry_reading_panel++;
							if(flag_receive_netp_temcovar == 1)
							{
								uint8_t j;
								remote_panel_db[i].product_model = 2;
	// if update panel and sub_id, should update network points table also
								for(j = 0;j < number_of_network_points_bacnet;j++)
								{
									if(network_points_list[j].instance == remote_panel_db[i].device_id)
									{// instance is same, update panel
										if(temcovar_panel != 0)
										{
											network_points_list[j].point.panel = temcovar_panel;
											network_points_list[j].point.sub_id = temcovar_panel;
										}
									}
								}
								if(temcovar_panel != 0)
								{
									remote_panel_db[i].panel = temcovar_panel;
									remote_panel_db[i].sub_id = temcovar_panel;
								}
								else
								{
									;//Test[10]++;
								}
							}

							// if retry many times, means it is old panel or other product
							if(remote_panel_db[i].retry_reading_panel > 10)
							{
								remote_panel_db[i].product_model = 2;
								remote_panel_db[i].retry_reading_panel = 0;
							}
							vTaskDelay( 500 / portTICK_RATE_MS);
						}
						//scan_network_bacnet_count++;
					}

				}
#endif

				if(number_of_network_points_bacnet > 0)
				{
					for(network_point_index = 0;network_point_index < number_of_network_points_bacnet;network_point_index++)
					{
						if(network_points_list[network_point_index].lose_count > 5)
						{
							network_points_list[network_point_index].lose_count = 0;
							network_points_list[network_point_index].decomisioned = 0;
						}
						flag_receive_netp = 0;
						network_points_list[network_point_index].invoked_id = 0;
						//if(flag_bip_client_reading == 0)  // if reading program code, dont read network points

						invoke =
								GetRemotePoint(network_points_list[network_point_index].point.point_type & 0x1f,
										network_points_list[network_point_index].point.number + (U16_T)((network_points_list[network_point_index].point.point_type & 0xe0) << 3),
										network_points_list[network_point_index].point.panel ,
										network_points_list[network_point_index].point.sub_id,
										BAC_IP_CLIENT);


						if(invoke >= 0)
						{
							network_points_list[network_point_index].invoked_id = invoke;
							udp_client_send(5);

					// check whether the device is online or offline

							if(flag_receive_netp == 1)
							{
								U8_T remote_panel_index;

								network_points_list[network_point_index].lose_count = 0;
								network_points_list[network_point_index].decomisioned = 1;
								if(Get_rmp_index_by_panel(network_points_list[network_point_index].point.panel,
								network_points_list[network_point_index].point.sub_id,
								&remote_panel_index,BAC_IP_CLIENT) != -1)
								{
									remote_panel_db[remote_panel_index].time_to_live = RMP_TIME_TO_LIVE;
								}
							}
							else
							{
								network_points_list[network_point_index].lose_count++;
								//network_points_list[network_point_index].decomisioned = 0;
							}


						}
						else  // if invoked id is 0, this point is off line
						{//network_points_list[network_point_index].decomisioned = 0;
							network_points_list[network_point_index].lose_count++;
						}
						//vTaskDelay( 500 / portTICK_RATE_MS);
						//scan_network_bacnet_count++;
					}
				}
			}
		}
		scan_network_bacnet_count++;
#endif

	}
}


void Update_LCD_IconArray(void)
{
	u8 r;
//arrays_data[0] = 1;// DAY_NIGHT
	r = arrays_data[0] / 1000;
	Modbus.icon_config &= 0xfc;
	if(r <= 3)
		Modbus.icon_config |= r;

	//arrays_data[1] = 2; // OCC_UNOCC
	r = arrays_data[1] / 1000;
	Modbus.icon_config &= 0xf3;
	if(r <= 3)
		Modbus.icon_config |= (r << 2);

	//arrays_data[2] = 1; // HEAT_COOL
	r = arrays_data[2] / 1000;
	Modbus.icon_config &= 0xcf;
	if(r <= 3)
		Modbus.icon_config |= (r << 4);

	//arrays_data[3] = 3; // FAN
	r = arrays_data[3] / 1000;
	Modbus.icon_config &= 0x3f;
	if(r <= 3)
		Modbus.icon_config |= (r << 6);

	//E2prom_Write_Byte(EEP_T10_ICON_CONFIG,Modbus.icon_config);
}
#endif


