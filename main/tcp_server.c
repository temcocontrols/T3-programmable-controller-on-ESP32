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

//#include "point.h"
#include "define.h"
#include "rs485.h"
#include "fifo.h"
#include "freertos/event_groups.h"
//#include "types.h"
#define PORT CONFIG_EXAMPLE_PORT

//static const char *TAG = "Example";
static const char *TCP_TASK_TAG = "TCP_TASK";
static const char *UDP_TASK_TAG = "UDP_TASK";

STR_SCAN_CMD Scan_Infor;
//uint8_t tcp_send_packet[1024];
EXT_RAM_ATTR uint8_t modbus_wifi_buf[500];
uint16_t modbus_wifi_len;
uint8_t reg_num;
/*static bool isSocketCreated = false;
extern double ambient;
extern double object;
extern float mlx90614_ambient;
extern float mlx90614_object;*/
extern uint32_t Instance;

void modbus_task(void *arg);
void Bacnet_Control(void) ;

//计数信号量相关 信号量句柄 最大计数值，初始化计数值 （计数信号量管理是否有资源可用。）
//MAX_COUNT 最大计数量，最多有几个资源
xSemaphoreHandle CountHandle;

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
   if(Modbus.ethernet_status == 2)
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
   if(Modbus.product_model == 88)
      memcpy(Scan_Infor.panelname,(char*)"T3-XB-ESP   ",12);
   else
      memcpy(Scan_Infor.panelname,(char*)"AirLab-esp32",12);

//   state = 1;
//   scanstart = 0;

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


static void bip_task(void *pvParameters)
{
   // char rx_buffer[600];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    uint16_t pdu_len = 0;
    BACNET_ADDRESS far src; /* source address */
    bip_set_socket(47808);
    while (1) {
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
            int len = recvfrom(sock, PDUBuffer_BIP, sizeof(PDUBuffer_BIP) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            bip_len = len;
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
               if (source_addr.sin6_family == PF_INET) {
                  inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                  ESP_LOGI(UDP_TASK_TAG, "IPV4 receive data");
               } else if (source_addr.sin6_family == PF_INET6) {
                  inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
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
							sendto(sock, (uint8_t *)&bip_send_buf, bip_send_len, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
							bip_send_len = 0;
							memset(bip_send_buf,0,MAX_MPDU_IP);
						}
					}
				}



                 /* UdpData(0);
                  ESP_LOGI(UDP_TASK_TAG, "receive data buffer[0] = 0x64");
                  int err = sendto(sock, (uint8_t *)&Scan_Infor, sizeof(STR_SCAN_CMD), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
                  if (err < 0) {
                     ESP_LOGE(UDP_TASK_TAG, "Error occurred during sending: errno %d", errno);
                     break;
                  }*/
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

static void udp_server_task(void *pvParameters)
{
    char rx_buffer[600];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    while (1) {
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

            // Error occurred during receiving
            if (len < 0) {
               ESP_LOGE(UDP_TASK_TAG, "recvfrom failed: errno %d", errno);
               break;
            }
            // Data received
            else 
            {
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
                     break;
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

//static uint8_t previousSock;
#define MAX_TCP_CONN  7
EXT_RAM_ATTR int sock[MAX_TCP_CONN];
EXT_RAM_ATTR int Sock_table[MAX_TCP_CONN];
//char len;
EXT_RAM_ATTR char rx_buffer[MAX_TCP_CONN][512];
xSemaphoreHandle sem_tcp_server;
int Modbus_Tcp(uint16_t len,int sock,char* rx_buffer)
{
	uint8_t modbus_send_buf[500];
	uint16_t modbus_send_len;
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
		responseModbusCmd(WIFI, (uint8_t *)rx_buffer, len ,modbus_send_buf,&modbus_send_len);
		if(modbus_send_len > 0)
		{
			int err = send(sock, (uint8_t *)&modbus_send_buf, modbus_send_len, 0);
			if (err < 0)
			{
				return -1;
				//ESP_LOGE(TCP_TASK_TAG, "Error occurred during sending: errno %d", errno);
				//break;
			}
			return err;
		}
	}

	return -2;

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
			{
				//Test[30]++;
				ret = Modbus_Tcp(len,remoteInfo.sock,rx_buffer[task_index]);
				if(ret < 0)
				{
					debug_print("Modbus_Tcp ret < 0 error! ",task_index);
					break;
				}
                //Test[5] = len;
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
	xEventGroupSetBits(network_EventHandle,CONNECTED_BIT|TASK1_BIT|TASK2_BIT|TASK3_BIT|TASK4_BIT|TASK5_BIT|TASK6_BIT|TASK7_BIT); //Fandu : CONNECTED_BIT这里还需要处理 wifi是否连接的信号量
	while(1)
	{
			taskCount++;
			debug_info("tcp_server_task is running\r");
		    int addr_family;
			addr_family 			 = AF_INET;
			ip_protocol 			 = IPPROTO_IP;
			// 本地IP设为0 应该底层会自动设置本地IP 端口固定到7681
			struct sockaddr_in localAddr;
			localAddr.sin_addr.s_addr 	= htonl(INADDR_ANY);
			localAddr.sin_family		= AF_INET;
			localAddr.sin_port			=htons(502);
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
			debug_info( "Socket created\r");

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
            char debug_buffer[100] =  {0};
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



EXT_RAM_ATTR uint8_t  PDUBuffer[MAX_APDU];
uint8_t Station_NUM;
uint32_t Instance;
uint8_t MAX_MASTER;
extern U8_T base_in;
extern U8_T base_out;
void Bacnet_Initial_Data(void);
void Inital_Bacnet_Server(void)
{
	uint32 ltemp = 0;
	Set_Object_Name((char *)"ESP-TEST");
	Device_Init();

	//Instance = 1000;
/*	ltemp = 0;
	ltemp |= AT24CXX_ReadOneByte(EEP_INSTANCE_LOWORD);
	ltemp |= (uint32)AT24CXX_ReadOneByte(EEP_INSTANCE_LOWORD+1)<<8;
	ltemp |= (uint32)AT24CXX_ReadOneByte(EEP_INSTANCE_LOWORD+2)<<16;
	ltemp |= (uint32)AT24CXX_ReadOneByte(EEP_INSTANCE_LOWORD+3)<<24;*/
	if((ltemp == 0xffffffff)||(ltemp == 0))
		Instance = ((uint32)Modbus.serialNum[3]<<24)|((uint32)Modbus.serialNum[2]<<16)|((uint32)Modbus.serialNum[1]<<8) | Modbus.serialNum[0];
	else
		Instance = ltemp;

	// tbd:
	Station_NUM = Modbus.address;//AT24CXX_ReadOneByte(EEP_STATION_NUMBER);
	MAX_MASTER = 254;
	Modbus.mini_type = 6;
	memset(panelname,"T3-XB_ESP",15);
	panel_number = Station_NUM;
	Bacnet_Initial_Data();

	Device_Set_Object_Instance_Number(Instance);
	address_init();
	bip_set_broadcast_addr(0xffffffff);


	AIS = MAX_INS + 1;
	AOS = MAX_AOS + 1;
	//AVS = MAX_AVS + 1;
	BOS = 0;
	base_in = MAX_INS + 1;
	base_out = MAX_AOS + 1;

//	Count_VAR_Object_Number();
	Count_IN_Object_Number();
	Count_OUT_Object_Number();

}
EXT_RAM_ATTR FIFO_BUFFER Receive_Buffer2;
EXT_RAM_ATTR uint8_t Receive_Buffer_Data2[512];
void Master_Node_task(void)
{

	uint16_t pdu_len = 0;
	uint8_t *uart_rsv = (uint8_t*)malloc(512);
		//uint8_t *stm32_uart_rsv = (uint8_t*)malloc(512);
		BACNET_ADDRESS  src;
	//	static u16 protocal_timer = SWITCH_TIMER;
		//modbus.protocal_timer_enable = 0;
	//	bacnet_inital();
		Inital_Bacnet_Server();
		dlmstp_init(NULL);
		//Recievebuf_Initialize(0);
		FIFO_Init(&Receive_Buffer2, &Receive_Buffer_Data2[0],
		         sizeof(Receive_Buffer_Data2));
	//	print("MSTP Task\r\n");
		//delay_ms(100);
		//Modbus.com_config[0] = BACNET_MASTER;

		for (;;)
	    {
			if(Modbus.com_config[0] == BACNET_MASTER)
			{
				pdu_len = datalink_receive(&src, &PDUBuffer[0], sizeof(PDUBuffer), 0,	0);
				if(pdu_len)
				{
					npdu_handler(&src, &PDUBuffer[0], pdu_len,0/* BAC_MSTP*/);
				}
			}
	// 		stack_detect(&test[1]);
			/*SilenceTime = SilenceTime + 5;
			if(SilenceTime > 10000)
			{
				SilenceTime = 0;
			}*/
			vTaskDelay(10 / portTICK_RATE_MS);
	    }


}


uint8_t get_protocal(void)
{
	return Modbus.com_config[0];
}

void uart_rx_task(void)
{
	uint8_t i;
	uint8_t *uart_rsv = (uint8_t*)malloc(512);

		for (;;)
	    {
			if(Modbus.com_config[0] == BACNET_MASTER)
			{
				int len = uart_read_bytes(UART_NUM_0, uart_rsv, 200, 100 / portTICK_RATE_MS);

				if(len > 0)
				{					/*for(i = 0;i < len;i++)
					{
						FIFO_Put(&Receive_Buffer2, uart_rsv[i]);

					}*/
					Timer_Silence_Reset();
				}

			}
			else
				vTaskDelay(5000 / portTICK_RATE_MS);
	    }
		free(uart_rsv);


}

#define CONFIG_FREERTOS_HZ 1000
uint32_t system_timer = 0;
void Timer_task(void)
{
	for (;;)
	{
		SilenceTime = SilenceTime + 10;
		if(SilenceTime > 10000)
		{
			SilenceTime = 0;
		}
		system_timer = system_timer + 10;
		vTaskDelay(10 / portTICK_RATE_MS);
	}

}

// for control task
S16_T exec_program(S16_T current_prg, U8_T *prog_code);
void Check_All_WR(void);
void pid_controller( S16_T p_number );
U8_T check_whehter_running_code(void);


#define PID_SAMPLE_COUNT 20
#define PID_SAMPLE_TIME 10
void Bacnet_Control(void)
{
	U16_T i,j;
	U8_T decom;
	static U8_T count_wait_sample = 0;
	static U8_T count_PID;
	static U16_T count_schedule;
	check_graphic_element();
	for(i = 0;i < MAX_OUTS;i++)
	{
		check_output_priority_array(i,0);
#if OUTPUT_DEATMASTER
			clear_dead_master();
#endif
	}

	Check_All_WR();
	for(;;)
  {
		control_input();

		if(check_whehter_running_code() == 1)
		for( i = 0; i < MAX_PRGS; i++/*, ptr++*/ )
		{
			if( programs[i].bytes )
			{
				if(programs[i].on_off	== 1)
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

		//control_output();
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
#if (ARM_MINI || ARM_CM5 || ARM_TSTAT_WIFI)
		CheckIdRoutines();
#endif

#if (ARM_MINI || ASIX_MINI)
		if(Modbus.com_config[1] == 7)
		{
			if(count_reset_zigbee < 30) 	count_reset_zigbee++;
			else
			{
				last_reset_zigbee++;
				count_reset_zigbee = 0;

	//  only for big minipanel, com1 must select rs232 jumper

				Check_Zigbee();

			}

		}

#endif

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

		//check_trendlog_1s(20);

		vTaskDelay(500 / portTICK_RATE_MS);
	//	taskYIELD();
	}

}

void app_main()
{

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    //ESP_ERROR_CHECK(example_connect());
	esp_err_t ret;
	Test[10] = 11;
   read_default_from_flash();
   mass_flash_init();
   modbus_init();
   //debug_info("modbus init finished^^^^^^^^");

   ethernet_init();
   ret = i2c_master_init();

    holding_reg_params.which_project = PROJECT_FAN_MODULE;//PROJECT_SAUTER;//
  // tcpip_socket_init();
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

    if(holding_reg_params.which_project == PROJECT_FAN_MODULE)
    {
   		holding_reg_params.fan_module_pwm2 = 0;
   		led_pwm_init();
   		led_init();
   		my_pcnt_init();
   		adc_init();
    }
    xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 6, NULL);
    network_EventHandle = xEventGroupCreate();
    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 2, NULL);
    xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);
    xTaskCreate(bip_task, "bacnet ip", 4096, NULL, 5, NULL);
    //if(holding_reg_params.which_project != PROJECT_FAN_MODULE)
    xTaskCreate(i2c_task,"i2c_task", 2048*2, NULL, 10, NULL);
    xTaskCreate(modbus_task,"modbus_task",4096, NULL, 3, NULL);
    //if(holding_reg_params.which_project != PROJECT_FAN_MODULE)
    {
       //xTaskCreate(input_task,"input_task",1024, NULL, 4, NULL);
    }
//    xTaskCreate(ble_mesh_task,"ble_mesh_task",4096*2, NULL, 7, NULL);
    xTaskCreate(Master_Node_task,"mstp_task",2048, NULL, 4, NULL);
    xTaskCreate(uart_rx_task,"uart_rx_task",2048, NULL, 6, NULL);
	xTaskCreate(Timer_task,"timer_task",1024, NULL, 7, NULL);
	xTaskCreate(Bacnet_Control,"BAC_Control_task",2048, NULL, tskIDLE_PRIORITY + 8,NULL);

	debug_info("App finished test1^^^^^^^^");
   // xTaskCreate(detect_tcp_task, "detect_tcp_task", 1024,NULL, 6,NULL);
   // xTaskCreate(pyq1548_task,"pyq1548_task",1024*2, NULL, 10, NULL);
   // xTaskCreate(&ota_task, "ota_example_task", 8192, NULL, 5, NULL);
}

// for bacnet lib
void uart_send_string(U8_T *p, U16_T length,U8_T port)
{
	uart_write_bytes(UART_NUM_0, (const char *)p, length);
}

/*char get_current_mstp_port(void)
{

		return -1;
}*/

U8_T RS485_Get_Baudrate(void)
{
	return 9;//uart2_baudrate;
}


