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

//#include "point.h"
#include "define.h"
#include "rs485.h"
#include "fifo.h"
#include "freertos/event_groups.h"


//#include "types.h"
#define PORT CONFIG_EXAMPLE_PORT

#define S_ALL_NEW  0x15
#define G_ALL_NEW  0x25

xTaskHandle main_task_handle[20];

char debug_array[100];
//static const char *TAG = "Example";
static const char *TCP_TASK_TAG = "TCP_TASK";
static const char *UDP_TASK_TAG = "UDP_TASK";

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

STR_Task_Test task_test;

extern uint8_t gIdentify;
extern uint8_t count_gIdentify;

uint8_t flag_clear_count_reboot;

void modbus0_task(void *arg);
void modbus2_task(void *arg);
void Bacnet_Control(void) ;
void smtp_client_task_nossl(void);
//void Lcd_task(void *arg);
//计数信号量相关 信号量句柄 最大计数值，初始化计数值 （计数信号量管理是否有资源可用。）
//MAX_COUNT 最大计数量，最多有几个资源
xSemaphoreHandle CountHandle;

uint8_t flag_suspend_scan = 0;
uint8_t	suspend_scan_count = 0;

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
//WIFI事件标志组 也兼用于任务运行标志组（任务被创建就set 被删除就clean 相当于资源清单哪些资源是可用的）
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

void start_fw_update(void)
{
   const esp_partition_t *factory = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
   esp_ota_set_boot_partition(factory);
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
   Scan_Infor.modbus_port = 502;//SSID_Info.modbus_port;  //tbd :????????????????

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
      memcpy(Scan_Infor.panelname,panelname,12);
   //else
   //   memcpy(Scan_Infor.panelname,(char*)"AirLab-esp32",12);

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
	// Send_SUB_I_Am 或者把MSTP包转回到网络
	if(len > 0)
	{
		sendto(bip_sock, (uint8_t *)buf, len, 0, (struct sockaddr *)&bip_source_addr, sizeof(bip_source_addr));
		len = 0;
	}

}

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
            ESP_LOGE(UDP_TASK_TAG, "Unable to create socket: errno %d", errno);
            break;
         }
         ESP_LOGI(UDP_TASK_TAG, "Socket created");

         int err = bind(bip_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
         if (err < 0) {
            ESP_LOGE(UDP_TASK_TAG, "Socket unable to bind: errno %d", errno);
         }
         ESP_LOGI(UDP_TASK_TAG, "Socket bound, port %d", PORT);

         while (1) {

            ESP_LOGI(UDP_TASK_TAG, "Waiting for data");
            //struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(bip_source_addr);
            int len = recvfrom(bip_sock, PDUBuffer_BIP, sizeof(PDUBuffer_BIP) - 1, 0, (struct sockaddr *)&bip_source_addr, &socklen);
                       bip_len = len;
            flagLED_ether_rx = 1;
            bip_Data = PDUBuffer_BIP;
            // Error occurred during receiving
            if (len < 0) {
               ESP_LOGE(UDP_TASK_TAG, "recvfrom failed: errno %d", errno);
               break;
            }
            // Data received
            else
            {
               // Get the sender's ip address as string
               if (bip_source_addr.sin6_family == PF_INET) {
                  inet_ntoa_r(((struct sockaddr_in *)&bip_source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                  ESP_LOGI(UDP_TASK_TAG, "IPV4 receive data");
               } else if (bip_source_addr.sin6_family == PF_INET6) {
                  inet6_ntoa_r(bip_source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                  ESP_LOGI(UDP_TASK_TAG, "IPV6 receive data");
               }

               //rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
               ESP_LOGI(UDP_TASK_TAG, "Received %d bytes from %s:", len, addr_str);
               ESP_LOG_BUFFER_HEX(UDP_TASK_TAG, PDUBuffer_BIP, len);



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
            ESP_LOGE(UDP_TASK_TAG, "Shutting down socket and restarting...");
            shutdown(bip_sock, 0);
            close(bip_sock);
         }
      }
      vTaskDelete(NULL);
    }
}

static void udp_scan_task(void *pvParameters)
{
    char rx_buffer[600];
    char addr_str[128];
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
            ESP_LOGE(UDP_TASK_TAG, "Unable to create socket: errno %d", errno);
            break;
         }
         ESP_LOGI(UDP_TASK_TAG, "Socket created");

         int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
         if (err < 0) {
            ESP_LOGE(UDP_TASK_TAG, "Socket unable to bind: errno %d", errno);
         }
         ESP_LOGI(UDP_TASK_TAG, "Socket bound, port %d", PORT);

         while (1) {

            ESP_LOGI(UDP_TASK_TAG, "Waiting for data");
            struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
            flagLED_ether_rx = 1;
            // Error occurred during receiving
            if (len < 0) {//debug_info("udp1234 recv error\r\n");
               ESP_LOGE(UDP_TASK_TAG, "recvfrom failed: errno %d", errno);
               break;
            }
            // Data received
            else 
            {//debug_info("udp1234 recv ok\r\n");
               // Get the sender's ip address as string
               if (source_addr.sin6_family == PF_INET) {
                  inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                  ESP_LOGI(UDP_TASK_TAG, "IPV4 receive data");
               } else if (source_addr.sin6_family == PF_INET6) {
                  inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                  ESP_LOGI(UDP_TASK_TAG, "IPV6 receive data");
               }

               //rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
               ESP_LOGI(UDP_TASK_TAG, "Received %d bytes from %s:", len, addr_str);
               ESP_LOG_BUFFER_HEX(UDP_TASK_TAG, rx_buffer, len);

               if(rx_buffer[0] == 0x64)
               {
                  UdpData(0);
                  ESP_LOGI(UDP_TASK_TAG, "receive data buffer[0] = 0x64");
                  int err = sendto(sock, (uint8_t *)&Scan_Infor, sizeof(STR_SCAN_CMD), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
                  if (err < 0) {
                     ESP_LOGE(UDP_TASK_TAG, "Error occurred during sending: errno %d", errno);

                     //debug_info("udp1234 send error\r\n");
                     break;
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
									ESP_LOGE(UDP_TASK_TAG, "Error occurred during sending: errno %d", errno);
									break;
								}
							}
						}
					}
               }
            }
         }

         if (sock != -1) {
            ESP_LOGE(UDP_TASK_TAG, "Shutting down socket and restarting...");
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
xSemaphoreHandle sem_tcp_server;

uint16_t modbus_send_len;
u8 modbus_send_buf[500];
int Modbus_Tcp(uint16_t len,int sock,U8_T* rx_buffer)
{
	memset(modbus_send_buf,0,500);
	modbus_send_len = 0;
	if (len == 5)
	{
		ESP_LOGI(TCP_TASK_TAG, "Receive: %02x %02x %02x %02x %02x.", rx_buffer[0], rx_buffer[1], rx_buffer[2], rx_buffer[3], rx_buffer[4]);
	}

	ESP_LOG_BUFFER_HEX(TCP_TASK_TAG, rx_buffer, len);

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

		Set_transaction_ID(header, ((U16_T)rx_buffer[0] << 8) | rx_buffer[1], 2 * rx_buffer[UIP_HEAD + 5] + 3);

		//vTaskSuspend(Handle_Scan);
		flag_suspend_scan = 1;
		suspend_scan_count = 0;
		Response_TCPIP_To_SUB(rx_buffer + UIP_HEAD,len - UIP_HEAD,Modbus.sub_port,header);

		if(modbus_send_len > 0)
		{
			int err = send(sock, (uint8_t *)&modbus_send_buf, modbus_send_len, 0);

			if (err < 0) {
				//ESP_LOGE(TCP_TASK_TAG, "Error occurred during sending: errno %d", errno);
				//break;
			}
			else
				flagLED_ether_tx = 1;
			return err;
		}
		//vTaskResume(Handle_Scan);
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

	// 给remoteInfo.remoteIp 开辟一定的空间 且注意释放
	//remoteInfo.remoteIp = (char *)heap_caps_malloc(32,MALLOC_CAP_8BIT);
	memset(remoteInfo.remoteIp,0,32);

	remoteInfo.sock			= ((struct sockinfo *)args)->sock;
	remoteInfo.remotePort 	= ((struct sockinfo *)args)->remotePort;
	memcpy(remoteInfo.remoteIp,((struct sockinfo *)args)->remoteIp,strlen(((struct sockinfo *)args)->remoteIp));

	EventBits_t res = xEventGroupClearBits(network_EventHandle,nTask_Bit);
	if((res & nTask_Bit) != 0)
		debug_print("TASK _BIT cleared successfully",task_index);
	else
	{
		debug_print("TASK _BIT clear failed",task_index);
	}

	int keepAlive = 1; // 开启keepalive属性
	int keepIdle = 10; // 如该连接在10秒内没有任何数据往来,则进行探测
	int keepInterval = 4; // 探测时发包的时间间隔为5 秒
	int keepCount = 1; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.

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
		debug_print("Readable_timeo ",task_index);

		ret = Readable_timeo(remoteInfo.sock, 60);//一分钟无数据就关闭套接字 set timeout and add if
        //if(task_index == 4)
        //{
        	char temp[20];
        	sprintf(temp,"ret = %d",ret);
        	debug_print(temp,task_index);
        //}
		if (ret > 0)
		{
			len = recv(remoteInfo.sock, rx_buffer[task_index], sizeof(rx_buffer) - 1, 0);

			if(len > 0)
			{flagLED_ether_rx = 1;
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


static void tcp_server_task(void *pvParameters)
{
	EventBits_t uxBits;
	UBaseType_t taskCount 	= 0;
	char taskName[50];
	struct hostent *hostP = NULL;
	int ip_protocol;
	char debug_buffer[100] =  {0};
	task_test.enable[2] = 0;
	xEventGroupSetBits(network_EventHandle,CONNECTED_BIT|TASK1_BIT|TASK2_BIT|TASK3_BIT|TASK4_BIT|TASK5_BIT|TASK6_BIT|TASK7_BIT); //Fandu : CONNECTED_BIT这里还需要处理 wifi是否连接的信号量
	while(1)
	{//task_test.count[2]++;
			taskCount++;
			debug_info("tcp_server_task is running\r");
		    int addr_family;
			addr_family 			 = AF_INET;
			ip_protocol 			 = IPPROTO_IP;
			// 本地IP设为0 应该底层会自动设置本地IP 端口固定到7681
			struct sockaddr_in localAddr;
			localAddr.sin_addr.s_addr 	= htonl(INADDR_ANY);
			localAddr.sin_family		= AF_INET;
			localAddr.sin_port			= htons(502);
			//新建一个 socket
			int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
			if (listen_sock < 0)
			{
				debug_info("Unable to create socket\r");
				vTaskDelay(5000 / portTICK_PERIOD_MS); //5秒钟后再重新执行
				continue;
			}
			int err = bind(listen_sock, (struct sockaddr *)&localAddr, sizeof(localAddr));
			if (err < 0) {
				debug_info("Socket unable to bind: errno\r");
				vTaskDelay(5000 / portTICK_PERIOD_MS); //5秒钟后再重新执行
				continue;
			}




			//开启监听 监听7681端口
			err = listen(listen_sock,0);
			if(err != 0)
			{
				debug_info("Socket unable to connect: errno\r");
				vTaskDelay(5000 / portTICK_PERIOD_MS); //5秒钟后再重新执行
				continue;
			}
			debug_info("Socket is listening\r");
			//为accpet连接传入参数初始化
			struct sockaddr_in6 sourceAddr;
			uint addrLen = sizeof(sourceAddr);

			while (1)
			{
				debug_info("ready to accept %d\r");

				//获取信号量，这里先阻滞portMAX_DELAY
				if(CountHandle != NULL)
				{
					xSemaphoreTake(CountHandle,portMAX_DELAY);
					UBaseType_t semapCount = uxSemaphoreGetCount(CountHandle);
					sprintf(debug_buffer,"Semaphore take success semapCount is:%d",semapCount);
					debug_info(debug_buffer);
				}
				else
					debug_info("SemaphoreHandle is NULL");

				//accept是会阻滞任务的  如果SemaphorTake 也一直阻滞不知道行不行。
				int sock = accept(listen_sock, (struct sockaddr *)&sourceAddr, &addrLen);
				if (sock < 0)
				{
					ESP_ERROR_CHECK(sock);
					debug_info("Unable to accept connection\r");
					break;
				}
				debug_info("Socket accepted\r");

				//获取到accept的IP sock 端口信息保存
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
				sprintf(debug_buffer,"ip:%s,port:%d ,sock:%d connected\r",remoteInfo.remoteIp,remoteInfo.remotePort,remoteInfo.sock);
				debug_info(debug_buffer);


				uxBits = xEventGroupWaitBits(network_EventHandle,TASK1_BIT|TASK2_BIT|TASK3_BIT|TASK4_BIT|TASK5_BIT|TASK6_BIT|TASK7_BIT,false,false,portMAX_DELAY);
				debug_info("tcp_server_task get  xEventGroupWaitBits success\r");
				for(int i = 0; i < MAX_SOC_COUNT; i++)
				{
					if((uxBits & (1 << (i + 1))) != 0)
					{ //这里i + 1是因为 事件标志组的最后一位是由CONNECT_BIT所占用 TASK2_BIT是从第BIT1开始
						sprintf(taskName,"tcp_server_dealwith%d",i);
						//打印remoteInfo的内容然后再建立任务
						//ESP_LOGI(TAG,"Currently socket NO:%d IP is:%s PORT is:%d",sock,remoteInfo.remoteIp,remoteInfo.remotePort);
						task_sock[i] = remoteInfo.sock;
						int res1 = xTaskCreate(taskList[i], taskName,	4096, (void *)&remoteInfo,7, &Task_handle[i]);
						//assert(res1 == pdTRUE);
						sprintf(debug_buffer,"xTaskCreate %d\r",i);
						debug_info(debug_buffer);
						break; //如果成功的创建了一个任务就应该结束本次查找了
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

			vTaskDelay(5000 / portTICK_PERIOD_MS); //5秒钟后再重新执行
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
    memset(NP_node_write,0,sizeof(STR_NP_NODE_OPERATE) * STACK_LEN);
    task_test.enable[3] = 1;
    while (1) {
    	task_test.count[3]++;
    	if(number_of_network_points_modbus > 0)
    	{
    		// write modbus points
    		for(uint8_t i = 0;i < STACK_LEN;i++)
    		{
    			if(NP_node_write[i].flag == 1) //	get current index, 1 -- WAIT_FOR_WRITE, 0 -- WRITE_OK
    			{
    				if(NP_node_write[i].retry < 10)
    				{
    					NP_node_write[i].retry++;
    				}
    				else
    				{  	// retry 10 time, give up
    					NP_node_write[i].flag = 0;
    					NP_node_write[i].retry = 0;
    					break;
    				}
    				ip = NP_node_write[i].ip;
					sub_id = NP_node_write[i].id;
					func = NP_node_write[i].func;
					reg = NP_node_write[i].reg;

					struct sockaddr_in dest_addr;
					int err;
					dest_addr.sin_addr.s_addr =	((uint32_t)ip << 24) + ((uint32_t)Modbus.ip_addr[2] << 16) +\
							((uint16_t)Modbus.ip_addr[1] << 8) + Modbus.ip_addr[0];
					dest_addr.sin_family = AF_INET;
					dest_addr.sin_port = htons(502);
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


							Modbus_Client_Command[0] = NP_node_write[i].ip;//transaction_id >> 8;
							Modbus_Client_Command[1] = tcp_client_transaction_id;
							Modbus_Client_Command[2] = 0x00;
							Modbus_Client_Command[3] = 0x00;
							Modbus_Client_Command[4] = 0x00;
							Modbus_Client_Command[5] = 0x06;  // len
							if(NP_node_write[i].id == 0)
								NP_node_write[i].id = 255;
							Modbus_Client_Command[6] = NP_node_write[i].id;
							Modbus_Client_Command[7] = NP_node_write[i].func;


							Modbus_Client_Command[8] = NP_node_write[i].reg >> 8;
							Modbus_Client_Command[9] = NP_node_write[i].reg;
							if(NP_node_write[i].len == 1)
							{
								if(NP_node_write[i].func == 0x06)
								{
									Modbus_Client_Command[10] = NP_node_write[i].value[0] >> 8;
									Modbus_Client_Command[11] = NP_node_write[i].value[0];
								}
								else if(NP_node_write[i].func == 0x05) // wirte coil
								{
									if(NP_node_write[i].value[0] == 0)
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
							if(NP_node_write[i].len == 2)
							{
								Modbus_Client_Command[5] = 0x0b;  // len
								Modbus_Client_Command[10] = 0x00;
								Modbus_Client_Command[11] = 0x02;
								Modbus_Client_Command[12] = 0x04;
								Modbus_Client_Command[13] = NP_node_write[i].value[0];
								Modbus_Client_Command[14] = NP_node_write[i].value[0] >> 8;
								Modbus_Client_Command[15] = NP_node_write[i].value[1];
								Modbus_Client_Command[16] = NP_node_write[i].value[1] >> 8;
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

												add_network_point( NP_node_write[i].ip,
														NP_node_write[i].id,
														NP_node_write[i].func,
														NP_node_write[i].reg,
														val_ptr * 1000,
												0,float_type);

												//flag_receive_netp_modbus = 1;
												//network_points_list_modbus[network_point_index].lose_count = 0;
												//network_points_list_modbus[network_point_index].decomisioned = 1;
											}

										}

									}
								}


						 }
    				}
    		}

    	}
    		// 读network modbus point
    		for(network_point_index = 0;network_point_index < number_of_network_points_modbus;network_point_index++)
    		{
    			if(network_points_list_modbus[network_point_index].lose_count > 3)
				{
					network_points_list_modbus[network_point_index].lose_count = 0;
					network_points_list_modbus[network_point_index].decomisioned = 0;
				}

				//flag_receive_netp_modbus = 0;

				ip = network_points_list_modbus[network_point_index].point.panel;
				sub_id = network_points_list_modbus[network_point_index].tb.NT_modbus.id;
				func = network_points_list_modbus[network_point_index].tb.NT_modbus.func & 0x7f;
				reg = network_points_list_modbus[network_point_index].tb.NT_modbus.reg;

				struct sockaddr_in dest_addr;
				int err;
				dest_addr.sin_addr.s_addr =
				((uint32_t)ip << 24) + ((uint32_t)Modbus.ip_addr[2] << 16) +\
						((uint16_t)Modbus.ip_addr[1] << 8) + Modbus.ip_addr[0];
				dest_addr.sin_family = AF_INET;
				dest_addr.sin_port = htons(502);
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
							float_type = (network_points_list_modbus[network_point_index].tb.NT_modbus.func & 0xff00) >> 8;
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

						if (err < 0) {Test[3]++;
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
							else {flagLED_ether_rx = 1;
							//debug_info("revc ok");
								tcp_client[index].time = system_timer;
								U8_T tcp_clinet_buf[20];
								S32_T val_ptr = 0;
								U8_T float_type = 0;
								if(len == 11 || len == 13 || len == 10)  // response read
								{ // READ ONE is 11, read 2bytes is 13, read coil is 10
									memcpy(&tcp_clinet_buf, rx_buffer,len);

									//if(network_points_list_modbus[network_point_index].point.panel == (U8_T)(ip >> 24) )
									float_type = (network_points_list_modbus[network_point_index].tb.NT_modbus.func & 0xff00) >> 8;
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

										if((tcp_clinet_buf[6] == network_points_list_modbus[network_point_index].tb.NT_modbus.id)
											&& (network_points_list_modbus[network_point_index].tb.NT_modbus.id != 0)
											&& (tcp_clinet_buf[7] == (network_points_list_modbus[network_point_index].tb.NT_modbus.func & 0x7f))
										)
										{
											add_network_point( network_points_list_modbus[network_point_index].point.panel,
											network_points_list_modbus[network_point_index].point.sub_id,
											network_points_list_modbus[network_point_index].point.point_type - 1,
											network_points_list_modbus[network_point_index].point.number + 1,
											val_ptr,
											0,float_type);

											//flag_receive_netp_modbus = 1;
											network_points_list_modbus[network_point_index].lose_count = 0;
											network_points_list_modbus[network_point_index].decomisioned = 1;
										}
									}
								}
							}
						}
						else
						{

							network_points_list_modbus[network_point_index].lose_count++;
						}

						vTaskDelay(200 / portTICK_PERIOD_MS);
					}


    		}// end 读network modbus point
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
void set_default_parameters(void)
{
	Bacnet_Initial_Data();
	save_point_info(0);

}

void Inital_Bacnet_Server(void)
{
	uint32 ltemp = 0;

	if(((panelname[0] == 0) && (panelname[1] == 0) && (panelname[2] == 0))  ||
			((panelname[0] == 255) && (panelname[1] == 255) && (panelname[2] == 255)) )
	{

			Set_Object_Name("T3-XB-ESP");
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
	Bacnet_Initial_Data();
	Sync_Panel_Info();

	read_point_info();
	initial_graphic_point();
	monitor_init();
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
	if(Modbus.mini_type == PROJECT_POWER_METER)
	{
		AIS = 7;
		AOS = 0;
		AVS = 63;
		BOS = 0;
	}
	if(Modbus.mini_type == PROJECT_TRANSDUCER)
	{
		AIS = 3;
		AOS = 3;
		AVS = 3;
		BOS = 0;
	}
	else
	{
		AIS = MAX_INS + 1;
		AOS = MAX_AOS + 1;
		AVS = MAX_AVS + 1;
		BOS = 0;

	}
#if BAC_TRENDLOG
	TRENDLOGS = 0;
	//Trend_Log_Init();
#endif

	Count_VAR_Object_Number();
	Count_IN_Object_Number();
	Count_OUT_Object_Number();
	//Test[29] = TRENDLOGS;
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
void Master0_Node_task(void)
{

	uint16_t pdu_len = 0;

	BACNET_ADDRESS  src;
	static uint16_t count_start_task = 0;
	int invoke = 0;

//	static u16 protocal_timer = SWITCH_TIMER;
	//modbus.protocal_timer_enable = 0;
//	bacnet_inital();
	Inital_Bacnet_Server();
	dlmstp_init(NULL);
	//Recievebuf_Initialize(0);
	//FIFO_Init(&Receive_Buffer0, &Receive_Buffer_Data0[0],
	//		 sizeof(Receive_Buffer_Data0));


	if(Modbus.com_config[0] == BACNET_SLAVE || Modbus.com_config[0] == BACNET_MASTER)
	{
		Recievebuf_Initialize(0);
		Send_I_Am_Flag = 1;
	}

	if( Modbus.com_config[0] == BACNET_MASTER)
	{
		Send_Whois_Flag = 1;Test[15]++;
	}
	remote_bacnet_index = 0;
	number_of_remote_points_bacnet = 0;
	task_test.enable[8] = 1;
	for (;;)
	{
		task_test.count[8]++;
		if(count_start_task % 12000 == 0)	// 1 min
		{
			if((Modbus.mini_type >= MINI_BIG_ARM) && (Modbus.mini_type <= MINI_NANO))
			{
				if(Modbus.com_config[2] == BACNET_MASTER || Modbus.com_config[0] == BACNET_MASTER)
				{	Send_Whois_Flag = 1;Test[16]++;}
				count_start_task = 0;
			}
		}
		else
		{
			if(Modbus.com_config[0] == BACNET_SLAVE || Modbus.com_config[0] == BACNET_MASTER)
			{
#if 0
				if((Modbus.mini_type >= MINI_BIG_ARM) && (Modbus.mini_type <= MINI_NANO))
				{
					if(count_start_task % 200 == 0) // 1.5s
					{
						// check whether the device is online or offline
						if(flag_receive_rmbp == 1)
						{
							U8_T remote_panel_index;
							if(Get_rmp_index_by_panel(remote_points_list_bacnet[remote_bacnet_index].point.panel,
							remote_points_list_bacnet[remote_bacnet_index].point.sub_id,
							&remote_panel_index,
							BAC_MSTP) != -1)
							{
								remote_panel_db[remote_panel_index].time_to_live = RMP_TIME_TO_LIVE;
							}
							remote_points_list_bacnet[remote_bacnet_index].lose_count = 0;
							remote_points_list_bacnet[remote_bacnet_index].decomisioned = 1;
						}
						else
						{
							remote_points_list_bacnet[remote_bacnet_index].lose_count++;
							if(remote_points_list_bacnet[remote_bacnet_index].lose_count > 10)
							{
								remote_points_list_bacnet[remote_bacnet_index].lose_count = 0;
								remote_points_list_bacnet[remote_bacnet_index].decomisioned = 0;
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

							static uint8 j = 0;
							uint8 count = 0;

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
										vTaskDelay(200 / portTICK_RATE_MS);

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
								if(Modbus.com_config[2] == BACNET_MASTER || Modbus.com_config[0] == BACNET_MASTER)
								{

									flag_receive_rmbp = 0;
									invoke	= GetRemotePoint(remote_points_list_bacnet[remote_bacnet_index].tb.RP_bacnet.object,
												remote_points_list_bacnet[remote_bacnet_index].tb.RP_bacnet.instance,
												panel_number,
												remote_points_list_bacnet[remote_bacnet_index].tb.RP_bacnet.panel ,
												BAC_MSTP);
									// check whether the device is online or offline

									if(invoke >= 0)
									{
										remote_points_list_bacnet[remote_bacnet_index].invoked_id = invoke;
									}
									else
									{
										remote_points_list_bacnet[remote_bacnet_index].lose_count++;
									}
								}
							}
						}

					}
				}

#endif
				pdu_len = dlmstp_receive(&src, &PDUBuffer[0], sizeof(PDUBuffer), 0);
				if(pdu_len)
				{
					npdu_handler(&src, &PDUBuffer[0], pdu_len,BAC_MSTP);
				}

			}

		}
		count_start_task++;
		vTaskDelay(5 / portTICK_RATE_MS);
	}


}

EXT_RAM_ATTR FIFO_BUFFER Receive_Buffer2;
EXT_RAM_ATTR uint8_t Receive_Buffer_Data2[512];
EXT_RAM_ATTR uint8_t  PDUBuffer2[MAX_APDU];

void Master2_Node_task(void)
{

	uint16_t pdu_len = 0;
	BACNET_ADDRESS  src;
	uint16_t count_start_task = 0;

//	FIFO_Init(&Receive_Buffer2, &Receive_Buffer_Data2[0],
//			 sizeof(Receive_Buffer_Data2));
	if(Modbus.com_config[2] == BACNET_SLAVE || Modbus.com_config[2] == BACNET_MASTER)
	{
		Recievebuf_Initialize(2);
		Send_I_Am_Flag = 1;
	}

	if( Modbus.com_config[2] == BACNET_MASTER){Test[17]++;
		Send_Whois_Flag = 1;}
	task_test.enable[11] = 1;
	for (;;)
	{
		task_test.count[11]++;
		if(count_start_task++ % 12000 == 0)	// 1 min
		{
			if((Modbus.mini_type >= MINI_BIG_ARM) && (Modbus.mini_type <= MINI_NANO))
			{
				if(Modbus.com_config[2] == BACNET_MASTER )
				{Test[18]++;
					Send_Whois_Flag = 1;
				}
			}
		}

		if(Modbus.com_config[2] == BACNET_MASTER || Modbus.com_config[2] == BACNET_SLAVE)
		{
			uint8_t count;
			int invoke;
			if(count_start_task % 100 == 0) // 1.5s
			{
				// check whether the device is online or offline
				if(flag_receive_rmbp == 1)
				{
					U8_T remote_panel_index;
					if(Get_rmp_index_by_panel(remote_points_list_bacnet[remote_bacnet_index].point.panel,
					remote_points_list_bacnet[remote_bacnet_index].point.sub_id,
					&remote_panel_index,
					BAC_MSTP) != -1)
					{
						remote_panel_db[remote_panel_index].time_to_live = RMP_TIME_TO_LIVE;
					}
					remote_points_list_bacnet[remote_bacnet_index].lose_count = 0;
					remote_points_list_bacnet[remote_bacnet_index].decomisioned = 1;
				}
				else
				{
					remote_points_list_bacnet[remote_bacnet_index].lose_count++;
					if(remote_points_list_bacnet[remote_bacnet_index].lose_count > 10)
					{
						remote_points_list_bacnet[remote_bacnet_index].lose_count = 0;
						remote_points_list_bacnet[remote_bacnet_index].decomisioned = 0;
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
				{
					static uint8 j = 0;Test[34]++;
					if(j < remote_panel_num)
					{
						if(remote_panel_db[j].protocal == BAC_MSTP
							&& remote_panel_db[j].sn == 0)
						{
							remote_panel_db[j].retry_reading_panel++;
							flag_receive_rmbp = 0;
							invoke = Send_private_scan(j);
							remote_mstp_panel_index = j;
							count = 0;
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
						if(Modbus.com_config[2] == BACNET_MASTER || Modbus.com_config[0] == BACNET_MASTER)
						{
							flag_receive_rmbp = 0;
							invoke	= GetRemotePoint(remote_points_list_bacnet[remote_bacnet_index].tb.RP_bacnet.object,
										remote_points_list_bacnet[remote_bacnet_index].tb.RP_bacnet.instance,
										panel_number,/*Modbus.network_ID[2],*/
										remote_points_list_bacnet[remote_bacnet_index].tb.RP_bacnet.panel ,
										BAC_MSTP);
							// check whether the device is online or offline

							if(invoke >= 0)
							{
								remote_points_list_bacnet[remote_bacnet_index].invoked_id	= invoke;
							}
							else
							{
								remote_points_list_bacnet[remote_bacnet_index].lose_count++;
							}
						}
					}
				}
			}
			//Test[20]++;
			Test[15]++;
			pdu_len = dlmstp_receive(&src, &PDUBuffer2[0], sizeof(PDUBuffer), 2);
			if(pdu_len)
			{
				led_main_rx++;
				com_rx[2]++;
				flagLED_main_rx = 1;
				npdu_handler(&src, &PDUBuffer2[0], pdu_len, BAC_MSTP);
			}
		}

		vTaskDelay(5 / portTICK_RATE_MS);
	}

}

uint8_t get_protocal(uint8 port)
{
	return Modbus.com_config[port];
}

void uart0_rx_task(void)
{
	uint8_t i;
	task_test.enable[9] = 1;
	for (;;)
	{
		task_test.count[9]++;
		if(Modbus.com_config[0] == BACNET_MASTER)
		{
			uint8_t *uart_rsv = (uint8_t*)malloc(512);
			int len = uart_read_bytes(UART_NUM_0, uart_rsv, 512, 10 / portTICK_RATE_MS);

			if(len > 0)
			{
				led_sub_rx++;
				com_rx[0]++;
				flagLED_sub_rx = 1;
				Timer_Silence_Reset();
			}
			free(uart_rsv);
		}
		else
			vTaskDelay(5000 / portTICK_RATE_MS);
	}

}

void uart2_rx_task(void)
{
	uint8_t i;
	task_test.enable[12] = 1;
	for (;;)
	{
		task_test.count[11]++;
		if(Modbus.com_config[2] == BACNET_MASTER)
		{
			uint8_t *uart_rsv = (uint8_t*)malloc(512);
			int len = uart_read_bytes(UART_NUM_2, uart_rsv, 512, 10 / portTICK_RATE_MS);

			if(len > 0)
			{
				led_main_rx++;
				com_rx[2]++;
				flagLED_main_rx = 1;
				Timer_Silence_Reset();
			}
			free(uart_rsv);
		}
		else
			vTaskDelay(5000 / portTICK_RATE_MS);
	}

}

uint32_t system_timer = 0;


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
esp_err_t save_point_info(uint8_t point_type);
#define TIMER_INTERVAL 1
#define TIMER_LED_INTERVAL 100
void Timer_task(void)
{
	//uint16_t count;
	uint16_t count = 0;
	timezone = 800;
	Daylight_Saving_Time = 0;
	if((Modbus.mini_type != PROJECT_FAN_MODULE)&&(Modbus.mini_type != PROJECT_TRANSDUCER)&&(Modbus.mini_type != PROJECT_POWER_METER))
		PCF_hctosys();
	update_timers();
	system_timer = 0;
	Mstp_ForUs = 0;
	Mstp_NotForUs = 0;
	task_test.enable[13] = 1;
	//FOR TEST
	//Rtc_Set(22,4,26,9,40,10,0); // to be deleted
	for (;;)
	{// 10ms
		task_test.count[13]++;
		//Test[0]++;
		//Test[10] = Mstp_ForUs;
		//Test[11] = Mstp_NotForUs;
		if((Mstp_ForUs > 100) && (Mstp_NotForUs > 10))
		{// MSTP error, reboot
			//esp_restart();
		}
		Test[19] = SilenceTime;

		SilenceTime = SilenceTime + TIMER_INTERVAL;
		if(SilenceTime > 10000 / TIMER_INTERVAL)
		{
			SilenceTime = 0;
		}
		// tbd:
		//if(0) // only for test
		{	
			if((Modbus.mini_type != PROJECT_FAN_MODULE)&&(Modbus.mini_type != PROJECT_TRANSDUCER)&&(Modbus.mini_type != PROJECT_POWER_METER))
				PCF_GetDateTime(&rtc_date);
			update_timers();
			if(count_hold_on_bip_to_mstp > 0)
				count_hold_on_bip_to_mstp--;
			count = 0;
		}
		system_timer = system_timer + TIMER_INTERVAL;

		if((system_timer > 10000) && (flag_clear_count_reboot == 0))
		{
			flag_clear_count_reboot = 1;
			save_uint8_to_flash(FLASH_COUNT_REBOOT,0); // clear reboot count
		}

		if(ChangeFlash != 0)
		{
			if(count_write_Flash++ > 5000 / TIMER_INTERVAL) // 5s
			{
			save_point_info(0);
			ChangeFlash = 0;
			count_write_Flash = 0;
			}
		}

		vTaskDelay(TIMER_INTERVAL / portTICK_RATE_MS);


	}

}


#define GPIO_STM_RST    32
#define GPIO_STM_RST_SEL  (1ULL<<GPIO_STM_RST)

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

	if(Modbus.mini_type == MINI_SMALL_ARM)
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

#define TEMPER_0_C   191*4
#define TEMPER_10_C   167*4
#define TEMPER_20_C   141*4
#define TEMPER_30_C   115*4
#define TEMPER_40_C   92*4
// update led and input_type
void Update_Led(void)
{
	U8_T loop;

//	U8_T index,shift;
//	U32_T tempvalue;
	static U16_T pre_in[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	U8_T error_in; // error of input raw value
	U8_T pre_status;
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
/*	else  if(Modbus.mini_type == MINI_TINY_ARM)
	{
		max_in = 8;
		max_out = 14;
		max_digout = 6;
		if(outputs[6].digital_analog == 0)
			max_digout++;
		if(outputs[7].digital_analog == 0)
			max_digout++;
	}
	else  if(Modbus.mini_type == MINI_TINY_11I)
	{
		max_in = 11;
		max_out = 11;
		max_digout = 6;

	}*/
	for(loop = 0;loop < max_in;loop++)
	{
		pre_status = InputLed[loop];
		if(input_raw[loop]  > pre_in[loop])
			error_in = input_raw[loop]  - pre_in[loop];
		else
			error_in = pre_in[loop] - input_raw[loop];


		if(inputs[loop].range == not_used_input)
			InputLed[loop] = 0;
		else
		{
			if(inputs[loop].auto_manual == 0) // auto
			{
				if(inputs[loop].digital_analog == 1) // analog
				{
					if(inputs[loop].range <= PT1000_200_570DegF)	  // temperature
					{	//  10k termistor GREYSTONE
						if(input_raw[loop]  > TEMPER_0_C) 	InputLed[loop] = 0;	   // 0 degree
						else  if(input_raw[loop]  > TEMPER_10_C) 	InputLed[loop] = 1;	// 10 degree
						else  if(input_raw[loop]  > TEMPER_20_C) 	InputLed[loop] = 2;	// 20 degree
						else  if(input_raw[loop]  > TEMPER_30_C) 	InputLed[loop] = 3;	// 30 degree
						else  if(input_raw[loop]  > TEMPER_40_C) 	InputLed[loop] = 4;	// 40 degree
						else
							InputLed[loop] = 5;	   // > 50 degree
					}
					else 	  // voltage or current
					{
						if(input_raw[loop]  < 50) 	InputLed[loop] = 0;
						else  if(input_raw[loop] < 200) 	InputLed[loop] = 1;
						else  if(input_raw[loop] < 400) 	InputLed[loop] = 2;
						else  if(input_raw[loop] < 600) 	InputLed[loop] = 3;
						else  if(input_raw[loop] < 800) 	InputLed[loop] = 4;
						else
							InputLed[loop] = 5;

					}

				}
				else if(inputs[loop].digital_analog == 0) // digtial
				{
					if(( inputs[loop].range >= ON_OFF  && inputs[loop].range <= HIGH_LOW )  // control 0=OFF 1=ON
					||(inputs[loop].range >= custom_digital1 // customer digital unit
					&& inputs[loop].range <= custom_digital8
					&& digi_units[inputs[loop].range - custom_digital1].direct == 1))
					{// inverse
						if(inputs[loop].control == 1) InputLed[loop] = 0;
						else
							InputLed[loop] = 5;
					}
					else
					{
						if(inputs[loop].control == 1) InputLed[loop] = 5;
						else
							InputLed[loop] = 0;
					}
				}
			}
			else // manual
			{
				if(inputs[loop].digital_analog == 0) // digtial
				{
					if(( inputs[loop].range >= ON_OFF  && inputs[loop].range <= HIGH_LOW )  // control 0=OFF 1=ON
					||(inputs[loop].range >= custom_digital1 // customer digital unit
					&& inputs[loop].range <= custom_digital8
					&& digi_units[inputs[loop].range - custom_digital1].direct == 1))
					{ // inverse
						if(inputs[loop].control == 1) InputLed[loop] = 0;
						else
							InputLed[loop] = 5;
					}
					else
					{
						if(inputs[loop].control == 1) InputLed[loop] = 5;
						else
							InputLed[loop] = 0;
					}

				}
				else   // analog
				{
					U32_T tempvalue;
					tempvalue = (inputs[loop].value) / 1000;
					if(inputs[loop].range <= PT1000_200_570DegF)	  // temperature
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
						if(inputs[loop].range == V0_5 || inputs[loop].range == P0_100_0_5V)
						{
							if(tempvalue <= 0) 	InputLed[loop] = 0;	   // 0 degree
							else  if(tempvalue <= 1) 	InputLed[loop] = 1;	// 10 degree
							else  if(tempvalue <= 2) 	InputLed[loop] = 2;	// 20 degree
							else  if(tempvalue <= 3) 	InputLed[loop] = 3;	// 30 degree
							else  if(tempvalue <= 4) 	InputLed[loop] = 4;	// 40 degree
							else
								InputLed[loop] = 5;	   // > 50 degree
						}
						if(inputs[loop].range == I0_20ma)
						{
							if(tempvalue <= 4) 	InputLed[loop] = 0;	   // 0 degree
							else  if(tempvalue <= 7) 	InputLed[loop] = 1;	// 10 degree
							else  if(tempvalue <= 10) 	InputLed[loop] = 2;	// 20 degree
							else  if(tempvalue <= 14) 	InputLed[loop] = 3;	// 30 degree
							else  if(tempvalue <= 18) 	InputLed[loop] = 4;	// 40 degree
							else
								InputLed[loop] = 5;	   // > 50 degree
						}
						if(inputs[loop].range == V0_10_IN || inputs[loop].range == P0_100_0_10V)
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
	//		OutputLed[loop] = loop / 4;
			//Test[30 + loop] = output_raw[loop];
			pre_status = OutputLed[loop];
			if(outputs[loop].switch_status == SW_AUTO)
			{
				if(outputs[loop].range == 0)
				{
					OutputLed[loop] = 0;
				}
				else
				{
					if(loop < max_digout)	  // digital
					{
						if(( outputs[loop].range >= ON_OFF  && outputs[loop].range <= HIGH_LOW )  // control 0=OFF 1=ON
						||(outputs[loop].range >= custom_digital1 // customer digital unit
						&& outputs[loop].range <= custom_digital8
						&& digi_units[outputs[loop].range - custom_digital1].direct == 1))
						{ // inverse
							if(outputs[loop].control == 1) OutputLed[loop] = 0;
							else
								OutputLed[loop] = 5;
						}
						else
						{
							if(outputs[loop].control == 1) OutputLed[loop] = 5;
							else
								OutputLed[loop] = 0;
						}
					}
					else
					{// hardwar AO
						if(outputs[loop].digital_analog == 1)
						{
							if(output_raw[loop] >= 512 )
								OutputLed[loop] = 0;
							else
								OutputLed[loop] = 5;
						}
						else
						{  // AO is used for DO
							if(( outputs[loop].range >= ON_OFF  && outputs[loop].range <= HIGH_LOW )  // control 0=OFF 1=ON
							||(outputs[loop].range >= custom_digital1 // customer digital unit
							&& outputs[loop].range <= custom_digital8
							&& digi_units[outputs[loop].range - custom_digital1].direct == 1))
							{ // inverse
								if(outputs[loop].control == 1) OutputLed[loop] = 0;
								else
									OutputLed[loop] = 5;
							}
							else
							{
								if(outputs[loop].control == 1) OutputLed[loop] = 5;
								else
									OutputLed[loop] = 0;
							}
						}
					}
				}
			}
			else if(outputs[loop].switch_status == SW_OFF)		OutputLed[loop] = 0;
			else if(outputs[loop].switch_status == SW_HAND)
			{
				if(outputs[loop].range != 0)
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

void i2c_master_task(void)
{

	uint8 index = 0;

	uint8 led_sub_tx_backup = 0;
	uint8 led_sub_rx_backup = 0;
	uint8 led_main_tx_backup = 0;
	uint8 led_main_rx_backup = 0;

	uint8_t i2c_send_buf[100];
	uint8_t i2c_rcv_buf[100];
	// RESET LED chip IO32
	//if(Modbus.mini_type == MINI_SMALL_ARM || Modbus.mini_type == MINI_BIG_ARM)
	{
		i2c_master_init();
		STM_RST_Init();
		gpio_set_level(GPIO_NUM_32, 0);
		usleep(100000); // 500ms
		gpio_set_level(GPIO_NUM_32, 1);
	}
	task_test.enable[0] = 1;
	i2c_send_buf[0] = i2c_send_buf[1] = i2c_send_buf[2] = i2c_send_buf[3] = 0;
	for (;;)
	{task_test.count[1]++;
		i2c_send_buf[0] = 0x55;
		if(Modbus.mini_type == PROJECT_TSTAT9)
		{
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
#if 1
		else if(Modbus.mini_type == MINI_SMALL_ARM || Modbus.mini_type == MINI_BIG_ARM)
		{
			// send
			// led
			if(index++ % 2 == 0)
			{
				Update_Led();
				i2c_send_buf[0] = led_buf[0];
				i2c_send_buf[1] = Modbus.mini_type;

				memcpy(&i2c_send_buf[2],OutputLed,24);
				memcpy(&i2c_send_buf[26],InputLed,32);
				if(Modbus.mini_type == MINI_SMALL_ARM)
				{
					for(uint8_t kk = 0;kk < 6;kk++)
					{
						i2c_send_buf[64 + kk] = output_raw[kk] > 512 ? 1 :0;
					}
					for(uint8_t kk = 0;kk < 4;kk++)
					{
						i2c_send_buf[70 + kk] = output_raw[kk + 6] / 4;
					}
				}
				else if(Modbus.mini_type == MINI_BIG_ARM)
				{
					for(uint8_t kk = 0;kk < 12;kk++)
					{
						i2c_send_buf[64 + kk] = output_raw[kk] > 512 ? 1 :0;
						Test[10 + kk] = i2c_send_buf[64 + kk];
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
				stm_i2c_write(S_ALL_NEW,i2c_send_buf,79);

			}
			else if(index > 3)
			{
			// rcv
				//if(Test[21] != 0)
				{
					uint8_t i = 0;
					uint16_t temp = 0;
					//if(Test[22] == 100)
					{
						//Test[22] = 0;
						if(Modbus.mini_type == MINI_SMALL_ARM)
						{
							stm_i2c_read(G_ALL_NEW,&i2c_rcv_buf,50);

							for(i = 0;i < 10;i++)
							{
								//if((i2c_rcv_buf[i] != (uint8_t)outputs[i].switch_status)
								//		&& (i2c_rcv_buf[i] <= 2))
								{
									outputs[i].switch_status = i2c_rcv_buf[i];
									//Test[10 + i] = outputs[i].switch_status;
									check_output_priority_HOA(i);
									flag_read_switch = 1;
								}
							}
							for(i = 0;i < 32 / 2;i++)	  // 88 == 24+64
							{
								//temp = Filter(i,(U16_T)(i2c_rcv_buf[i * 2 + 1 + 24] + i2c_rcv_buf[i * 2 + 24] * 256));
								temp = i2c_rcv_buf[i * 2 + 1 + 24] + i2c_rcv_buf[i * 2 + 24] * 256;
								//if(temp != 0xffff)
								{// rev42 of top is 12U8_T, older rev is 10U8_T

									input_raw[i] = temp;
									//Test[23 + i] = temp;

								}
							}
						}
						else if(Modbus.mini_type == MINI_BIG_ARM)
						{
							stm_i2c_read(G_ALL_NEW,&i2c_rcv_buf,80);

							for(i = 0;i < 24;i++)
							{
									outputs[i].switch_status = i2c_rcv_buf[i];
									//Test[10 + i] = outputs[i].switch_status;
									check_output_priority_HOA(i);
									flag_read_switch = 1;

							}
							for(i = 0;i < 32;i++)	  // 88 == 24+64
							{
								//temp = Filter(i,(U16_T)(i2c_rcv_buf[i * 2 + 1 + 24] + i2c_rcv_buf[i * 2 + 24] * 256));
								temp = i2c_rcv_buf[i * 2 + 1 + 24] + i2c_rcv_buf[i * 2 + 24] * 256;
								//if(temp != 0xffff)
								{// rev42 of top is 12U8_T, older rev is 10U8_T

									input_raw[i] = temp;
									//Test[23 + i] = temp;

								}
							}
						}
					}

					//Test[21] = 0;
				}
				vTaskDelay(1000 / portTICK_RATE_MS);
			}

			vTaskDelay(500 / portTICK_RATE_MS);
		}

#endif

	}

}
#endif
// for control task
S16_T exec_program(S16_T current_prg, U8_T *prog_code);
void Check_All_WR(void);
void pid_controller( S16_T p_number );
U8_T check_whehter_running_code(void);
void Check_Net_Point_Table(void);

#define PID_SAMPLE_COUNT 20
#define PID_SAMPLE_TIME 10

extern U8_T max_dos;
extern U8_T max_aos;
void Bacnet_Control(void)
{
	U16_T i,j;
	U8_T decom;
	static U8_T count_wait_sample = 0;
	static U8_T count_PID;
	static U16_T count_schedule;
	check_graphic_element();
	if(Modbus.mini_type == MINI_BIG_ARM)
	{	//max_dos = BIG_MAX_DOS; max_aos = BIG_MAX_AOS;
		max_dos = 12; max_aos = 12;
	}
	else if(Modbus.mini_type == MINI_SMALL_ARM)
	{	//max_dos = SMALL_MAX_DOS; max_aos = SMALL_MAX_AOS;
		max_dos = 6; max_aos = 4;
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
	for(;;)
	{
		task_test.count[14]++;

		/*{// TEST stack lenght;
			u8 i= 0;
			for(i = 0;i < 16;i++)
			{
			if(task_test.enable[i] == 1)
				task_test.inactive_count[i] = uxTaskGetStackHighWaterMark( main_task_handle[i] );
			}
		}*/

		control_input();
		if(Test[39] == 100)
		{
			//smtp_client_task();
			Test[39] = 0;
		}
		if(Test[39] == 300)
		{
			//smtp_client_task_nossl();
			Test[39] = 0;
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

		control_output();

// check whether external IO are on line
		/*for(i = 0;i < sub_no;i++)
		{
			if(current_online[sub_map[i].id / 8] & (1 << (sub_map[i].id % 8)))
			{
				decom = 0;
			}
			else  // off line
			{
				decom = 1;
			}

			for(j = 0; j < sub_map[i].do_len;j++)
			{
					outputs[sub_map[i].do_start + j].decom = decom;
			}
			for(j = 0; j < sub_map[i].ao_len;j++)
			{
					outputs[sub_map[i].ao_start + j].decom = decom;
			}
		}*/
		count_PID++;  // 1s
		if(count_PID >= PID_SAMPLE_COUNT) // 500MS * PID_SAMPLE_COUNT == PID_SAMPLE_TIME 10S
		{
	   // dealwith controller roution per 1 sec
			for(i = 0;i < MAX_CONS; i++)
			{
				pid_controller( i );
			}
			count_PID = 0;
#if (ARM_MINI || ARM_TSTAT_WIFI)
			Store_Pulse_Counter(0);
			calculate_RPM();
#endif
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
#if BAC_TRENDLOG
		//if( count_1s++  >= 1)
		{
			miliseclast_cur = miliseclast;
			miliseclast = 0;
		}
//		else
//			count_1s = 0;
		check_trendlog_1s(2);
#endif

#if BAC_TRENDLOG
		//trend_log_timer(0); // for standard trend log
#endif
		Check_Net_Point_Table();
		vTaskDelay(500 / portTICK_RATE_MS);

	}

}
#define FAN 0
void TEST_FLASH(void);
void vStartScanTask(unsigned char uxPriority);
extern uint8_t count_reboot;
void app_main()
{

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    //ESP_ERROR_CHECK(example_connect());
	/*#if FAN
   Modbus.mini_type = PROJECT_FAN_MODULE;
    holding_reg_params.which_project = PROJECT_FAN_MODULE;//PROJECT_SAUTER;//
#endif  */
	SW_REV = SOFTREV;
	count_reboot = 0;
	read_default_from_flash();
   uart_init(0);
   uart_init(2);
  // TEST_FLASH();
   ethernet_init();
#if 1//I2C_TASK
   i2c_master_init();
#endif
/*#if FAN
   Modbus.mini_type = PROJECT_FAN_MODULE;
    holding_reg_params.which_project = PROJECT_FAN_MODULE;//PROJECT_SAUTER;//
#endif*/


   /*if(holding_reg_params.which_project == PROJECT_SAUTER)
   {
      stm32_uart_init();
   }
   else if(holding_reg_params.which_project == PROJECT_FAN_MODULE)
   {
		holding_reg_params.fan_module_pwm2 = 0;
		led_pwm_init();
		led_init();
		my_pcnt_init();
		adc_init();
   }*/
    //microphone_init();
    //SSID_Info.IP_Wifi_Status = WIFI_CONNECTED;
    //connect_wifi();
   //Modbus.com_config[0] = MODBUS_SLAVE;  // ONLY FOR TEST
   Modbus.tcp_port = 502;
#if 1//I2C_TASK
    if(Modbus.mini_type == PROJECT_FAN_MODULE)
    {
   		holding_reg_params.fan_module_pwm2 = 0;
   		led_pwm_init();
   		led_init();
   		my_pcnt_init();
   		adc_init();
   		i2c_master_init();
    }

    if(Modbus.mini_type == PROJECT_TRANSDUCER)
	{
		led_pwm_init();
		transducer_switch_init();
		//led_init();
		//adc_init();
		i2c_master_init();
	}
    if(Modbus.mini_type == PROJECT_POWER_METER)
    {
    	i2c_master_init();
    }

    if(Modbus.mini_type == MINI_NANO || Modbus.mini_type == PROJECT_TSTAT9 ||  Modbus.mini_type == MINI_SMALL_ARM
    		|| Modbus.mini_type == MINI_BIG_ARM)
    	xTaskCreate(i2c_master_task,"i2c_master_task", 2048, NULL, 10, &main_task_handle[0]);
#endif
    xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 6, &main_task_handle[1]);
    network_EventHandle = xEventGroupCreate();
    xTaskCreate(tcp_server_task, "tcp_server", 6000, NULL, 2, &main_task_handle[2]);
    xTaskCreate(tcp_client_task, "tcp_client", 6000, NULL, 5, &main_task_handle[3]);
    xTaskCreate(udp_scan_task, "udp_scan", 4096, NULL, 5, &main_task_handle[4]);
    xTaskCreate(bip_task, "bacnet ip", 4096, NULL, 5, &main_task_handle[5]);
    //if(holding_reg_params.which_project != PROJECT_FAN_MODULE)
//#if FAN
    if((Modbus.mini_type == PROJECT_FAN_MODULE) || (Modbus.mini_type == PROJECT_TRANSDUCER) || (Modbus.mini_type == PROJECT_POWER_METER))
        xTaskCreate(i2c_task,"i2c_task", 2048*2, NULL, 10, NULL);
//#endif
    Test[4] = 3;
    //if(Modbus.mini_type == PROJECT_TSTAT9)
    //	xTaskCreate(Lcd_task,"lcd_task",2048, NULL, tskIDLE_PRIORITY + 8,&main_task_handle[6]);

    xTaskCreate(modbus0_task,"modbus0_task",4096, NULL, 11, &main_task_handle[7]);
    xTaskCreate(Master0_Node_task,"mstp0_task",4096, NULL, 4, &main_task_handle[8]);
    xTaskCreate(uart0_rx_task,"uart0_rx_task",2048, NULL, 6, &main_task_handle[9]);

    if((Modbus.mini_type >= MINI_BIG_ARM) && (Modbus.mini_type <= MINI_NANO))
    {
	   xTaskCreate(modbus2_task,"modbus2_task",4096, NULL, 3, &main_task_handle[10]);
	   xTaskCreate(Master2_Node_task,"mstp2_task",4096, NULL, 4, &main_task_handle[11]);
	   xTaskCreate(uart2_rx_task,"uart2_rx_task",2048, NULL, 6, &main_task_handle[12]);
    }

 	xTaskCreate(Timer_task,"timer_task",2048, NULL, 7, &main_task_handle[13]);
	xTaskCreate(Bacnet_Control,"BAC_Control_task",2048, NULL,  12,&main_task_handle[14]);
//	xTaskCreate(rtc_task,"rtc_task", 2048, NULL, 10, NULL);
	vStartScanTask(5);


//	xTaskCreate(smtp_client_task, "smtp_client_task", 2048, NULL, 5, NULL);
}

// for bacnet lib
void uart_send_string(U8_T *p, U16_T length,U8_T port)
{
//#if FAN
	if(Modbus.mini_type == PROJECT_FAN_MODULE)
		holding_reg_params.led_rx485_tx = 2;
//#endif
	if(port == 0)
	{ Test[12]++;
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
