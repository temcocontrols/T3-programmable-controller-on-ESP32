#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
//#include "esp_wifi.h"
#include "wifi.h"
#include "driver/uart.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "flash.h"
#include "esp_log.h"
//#include "modbus.h"
#include "lwip/sockets.h"
#include "define.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"


static const char *TAG = "WIFI";
extern SemaphoreHandle_t CountHandle;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define EXAMPLE_ESP_MAXIMUM_RETRY	10

void disable_wifi();
/* FreeRTOS event group to signal when we are connected*/
EventGroupHandle_t s_wifi_event_group;
extern EventGroupHandle_t network_EventHandle;
//STR_SCAN_CMD Scan_Infor;
STR_SSID	SSID_Info;
bool re_init_wifi = false;
extern unsigned short int Test[50];
static int s_retry_num = 0;
TaskHandle_t Wifi_Task_handle[7];
extern int task_sock[7];
void debug_print(char *string,char task_index)
{
#if 0
	char temp_char[200] = {0};
	sprintf(temp_char,"%d : %s\r",task_index,string);
 	uart_write_bytes(UART_NUM_0, (const char *)temp_char, strlen(temp_char));
 	uart_write_bytes(UART_NUM_0, "\r\n", 2);
 	led_sub_tx++;
#endif
}

void debug_info(char *string)
{
#if 1//DEBUG_INFO_UART0
 	//uart_write_bytes(UART_NUM_0, "\r\n", 1);
 	uart_write_bytes(UART_NUM_0, (const char *)string, strlen(string));

 	uart_write_bytes(UART_NUM_0, "\r\n", 2);
 	led_sub_tx++;
 	flagLED_sub_tx = 1;
#endif
}

void init_ssid_info()
{
	memset(SSID_Info.name,0,64);
	memset(SSID_Info.password,0,32);
	memcpy(SSID_Info.name, "TP-LINK_wuxian", strlen("TP-LINK_wuxian"));
	memcpy(SSID_Info.password, "87654321", strlen("87654321"));
	//memcpy(SSID_Info.name, "TEMCO_TEST_2.4G", strlen("TEMCO_TEST_2.4G"));
	//memcpy(SSID_Info.password, "Travel321", strlen("Travel321"));
}

//#define WIFI_RETRY_NEED_INITIAL_COUNT  20
unsigned char wifi_retry_count = 0;
//unsigned char wifi_task_running = 1;
//Fandu : 锟斤拷锟矫猴拷锟斤拷 esp_wifi_connect()
//锟斤拷 wifi 锟斤拷锟斤拷锟劫次筹拷锟斤拷锟斤拷锟饺点建锟斤拷锟斤拷锟接★拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷晒锟斤拷锟斤拷锟斤拷锟劫次斤拷锟斤拷 CONNECT锟斤拷GOTIP 锟斤拷锟斤拷锟斤拷状态锟斤拷
//锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟绞э拷埽锟斤拷锟斤拷俅谓锟斤拷锟� DISCONNECT 状态锟斤拷锟斤拷锟轿凤拷锟斤拷循锟斤拷锟斤拷直锟斤拷锟斤拷锟接成癸拷为止锟斤拷 锟斤拷锟斤拷锟斤拷锟斤拷
//锟斤拷太锟斤拷锟斤拷为什么 event_handler 为什么锟斤拷锟斤拷锟斤拷时锟津不达拷锟斤拷 SYSTEM_EVENT_STA_DISCONNECTED

static void wifi_event_handler(
        void *arg,
        esp_event_base_t event_base,
        int32_t event_id,
        void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:

            ESP_LOGI(TAG, "Connecting to AP...");
            //debug_info("event_handler_2 esp_wifi_connect()");
            esp_wifi_connect();
            SSID_Info.IP_Wifi_Status = WIFI_CONNECTED;
            if(SSID_Info.IP_Auto_Manual == 1)
                SSID_Info.IP_Wifi_Status = WIFI_NORMAL;
            break;

        case WIFI_EVENT_STA_DISCONNECTED:

            //wifi_task_running = 0;
            SSID_Info.IP_Wifi_Status = WIFI_DISCONNECTED;
            //debug_info("Wifi disconnected, try to connect ...");

            if(0)
            {// wifi
                for(int i=0 ;i<7;i++)
                {
                    char temp_test[50];
                    if(Wifi_Task_handle[i] != 0)
                    {
                        sprintf(temp_test,"shutdown sock %d\r",i);
                        //debug_info(temp_test);
                        shutdown(task_sock[i],2);
                        close(task_sock[i]);
                        task_sock[i] = -1;
                        vTaskDelete( Wifi_Task_handle[i] );
                        Wifi_Task_handle[i] = 0;

                        if(CountHandle != NULL)
                        {
                            if(xSemaphoreGive(CountHandle) != pdTRUE)
                            {
                                //debug_info("Disconnected Try to Give semaphore and failed!");
                            }
                            else
							{
                                //debug_info("Disconnected Give semaphore success!");
							}
                        }

                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                    }
                    //sprintf(temp_test, "Wifi_Task_handle[%d] =  %d",i,(int)Wifi_Task_handle[i]);
                    //debug_info(temp_test);
                }

                xEventGroupSetBits(network_EventHandle,BIT1);
                xEventGroupSetBits(network_EventHandle,BIT2);
                xEventGroupSetBits(network_EventHandle,BIT3);
                xEventGroupSetBits(network_EventHandle,BIT4);
                xEventGroupSetBits(network_EventHandle,BIT5);
                xEventGroupSetBits(network_EventHandle,BIT6);
                xEventGroupSetBits(network_EventHandle,BIT7);
            }

            vTaskDelay(2000 / portTICK_PERIOD_MS);
            wifi_retry_count ++;
            //if(wifi_retry_count < 10)
                esp_wifi_connect();
            /*else
            {
                //wifi_retry_count = 0;
                //xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                //  debug_info("run wifi_init_sta()");
                //  wifi_init_sta();
            }*/
            //if(wifi_retry_count > WIFI_RETRY_NEED_INITIAL_COUNT)
            //{
            //  xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            //  Test[0] = 185;
            //  wifi_init_sta();
            //wifi_retry_count = 0;
            //}
            break;

        default:
            break;
        }
    }
    else if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:

            ESP_LOGI(TAG, "Connected.");
            // debug_info("event_handler_2 SYSTEM_EVENT_STA_GOT_IP");
            wifi_retry_count = 0;
            //wifi_task_running = 1;
            //xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            SSID_Info.IP_Wifi_Status = WIFI_NORMAL;
            break;

        default:
            break;
        }
    }
}


static void event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        //debug_info("esp_wifi_connect()");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            //debug_info("s_retry_num ++ ");
            esp_wifi_connect();
            s_retry_num++;
            //ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            //debug_info("s_retry_num  big ,stop try!");
            esp_wifi_connect();
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        //ESP_LOGI(TAG,"connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        const esp_netif_ip_info_t *ip_info = &event->ip_info;

        debug_info("wifi got ip!");

        // Save IP info to SSID_Info struct
        SSID_Info.ip_addr[0] = esp_ip4_addr1(&ip_info->ip);
        SSID_Info.ip_addr[1] = esp_ip4_addr2(&ip_info->ip);
        SSID_Info.ip_addr[2] = esp_ip4_addr3(&ip_info->ip);
        SSID_Info.ip_addr[3] = esp_ip4_addr4(&ip_info->ip);

        SSID_Info.net_mask[0] = esp_ip4_addr1(&ip_info->netmask);
        SSID_Info.net_mask[1] = esp_ip4_addr2(&ip_info->netmask);
        SSID_Info.net_mask[2] = esp_ip4_addr3(&ip_info->netmask);
        SSID_Info.net_mask[3] = esp_ip4_addr4(&ip_info->netmask);

        SSID_Info.getway[0] = esp_ip4_addr1(&ip_info->gw);
        SSID_Info.getway[1] = esp_ip4_addr2(&ip_info->gw);
        SSID_Info.getway[2] = esp_ip4_addr3(&ip_info->gw);
        SSID_Info.getway[3] = esp_ip4_addr4(&ip_info->gw);

        SSID_Info.IP_Wifi_Status = WIFI_NORMAL;

        if(Modbus.ethernet_status != 4)
            multicast_addr = Get_multicast_addr((unsigned char*)&SSID_Info.ip_addr);

        save_wifi_info();
        s_retry_num = 0;

    #if 1 //DNS
        if((SSID_Info.getway[0] != 0) || (SSID_Info.getway[1] != 0) ||
           (SSID_Info.getway[2] != 0) || (SSID_Info.getway[3] != 0))
        {
            esp_netif_dns_info_t dns_info = {0};

            IP4_ADDR(&dns_info.ip.u_addr.ip4, SSID_Info.getway[0], SSID_Info.getway[1], SSID_Info.getway[2], SSID_Info.getway[3]);
            dns_info.ip.type = ESP_IPADDR_TYPE_V4;
            ESP_ERROR_CHECK(esp_netif_set_dns_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), ESP_NETIF_DNS_MAIN, &dns_info));

            IP4_ADDR(&dns_info.ip.u_addr.ip4, 8,8,8,8);
            dns_info.ip.type = ESP_IPADDR_TYPE_V4;
            ESP_ERROR_CHECK(esp_netif_set_dns_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), ESP_NETIF_DNS_BACKUP, &dns_info));

            IP4_ADDR(&dns_info.ip.u_addr.ip4, 8,8,4,4);
            dns_info.ip.type = ESP_IPADDR_TYPE_V4;
            ESP_ERROR_CHECK(esp_netif_set_dns_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), ESP_NETIF_DNS_FALLBACK, &dns_info));
        }
    #endif

        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void on_wifi_disconnect(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...");
    esp_err_t err = esp_wifi_connect();
    if (err == ESP_ERR_WIFI_NOT_STARTED) {
        return;
    }
    SSID_Info.IP_Wifi_Status = WIFI_DISCONNECTED;
    ESP_ERROR_CHECK(err);
}


#if 1
void wifi_init_sta(void)
{
    esp_err_t ret;

    s_wifi_event_group = xEventGroupCreate();
    CountHandle = xSemaphoreCreateCounting(7,7);
#if 1
    /* -------- NETIF INIT (Continue if already done) -------- */
    ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        debug_info("esp_netif_init failed");
    }
#endif
    /* -------- EVENT LOOP INIT (Continue if already created) -------- */
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        debug_info("event loop create failed");
    }

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == NULL) {
        netif = esp_netif_create_default_wifi_sta();
    }
    if (!netif) {
        debug_info("wifi netif create failed");
        return;
    }

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        debug_info("esp_wifi_init failed");
        //return; // don't return — Matter may have already inited it, continue
    }

    if(SSID_Info.MANUEL_EN != 1)
    {
        disable_wifi();
        return;
    }

    /* -------- STATIC IP -------- */
    if(SSID_Info.IP_Auto_Manual == 1)
    {
        esp_netif_dhcpc_stop(netif);

        esp_netif_ip_info_t info_t = {0};

        info_t.ip.addr = ESP_IP4TOADDR(
            SSID_Info.ip_addr[0],
            SSID_Info.ip_addr[1],
            SSID_Info.ip_addr[2],
            SSID_Info.ip_addr[3]);

        info_t.netmask.addr = ESP_IP4TOADDR(
            SSID_Info.net_mask[0],
            SSID_Info.net_mask[1],
            SSID_Info.net_mask[2],
            SSID_Info.net_mask[3]);

        info_t.gw.addr = ESP_IP4TOADDR(
            SSID_Info.getway[0],
            SSID_Info.getway[1],
            SSID_Info.getway[2],
            SSID_Info.getway[3]);

        esp_netif_set_ip_info(netif, &info_t);

        if(info_t.gw.addr != 0)
        {
            esp_netif_dns_info_t dns_info = {0};

            IP_ADDR4(&dns_info.ip,
                SSID_Info.getway[0],
                SSID_Info.getway[1],
                SSID_Info.getway[2],
                SSID_Info.getway[3]);

            esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);

            IP_ADDR4(&dns_info.ip, 8,8,8,8);
            esp_netif_set_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info);

            IP_ADDR4(&dns_info.ip, 8,8,4,4);
            esp_netif_set_dns_info(netif, ESP_NETIF_DNS_FALLBACK, &dns_info);
        }
    }

    wifi_config_t wifi_config = {0};

    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    if(SSID_Info.name[0] != 0)
    {
        memcpy(wifi_config.sta.ssid, SSID_Info.name, 32);
        memcpy(wifi_config.sta.password, SSID_Info.password, 32);
    }
    else
    {
        init_ssid_info();
    }

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    /* -------- WAIT WITH TIMEOUT (No infinite block) -------- */
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(15000));

    if (bits & WIFI_CONNECTED_BIT) {
        debug_info("wifi connected");
    } else if (bits & WIFI_FAIL_BIT) {
        debug_info("wifi failed");
    } else {
        debug_info("wifi timeout");
    }
}
#endif
/*
esp_err_t scan_event_handler(void *ctx, system_event_t *event)
{
	if(event->event_id == SYSTEM_EVENT_SCAN_DONE)
	{
		printf("WiFi Scan Completed!\n");
		printf("Number of access points found: %d\n",event->event_info.scan_done.number);
		uint16_t apCount = event->event_info.scan_done.number;
		if(apCount == 0)
		{
			return 0;
		}
		wifi_ap_record_t *list = (wifi_ap_record_t *) malloc(sizeof(wifi_ap_record_t) *apCount);
		ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, list));

		printf("\n");
		printf("               SSID              | Channel | RSSI |   Auth Mode \n");
		printf("----------------------------------------------------------------\n");
		for(int i = 0; i < apCount; i++)
		{
			//printf("2s | } | M | s\n",(char *)list[i].ssid, list[i].primary, list[i].rssi, get_authmode(list[i].authmode));
			//if(strcmp((const char*)list[i].ssid, (const char*)SSID_Info.name)==0)
			{
				SSID_Info.rssi = list[i].rssi;
			}
		}

		printf("----------------------------------------------------------------\n");


		free(list);
	}

	return ESP_OK;


}
*/

void get_wifi_signal_strength(void) {
    wifi_ap_record_t ap_info;

    // 获取当前连接的 Wi-Fi 接入点信息
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        // 打印信号强度（RSSI）
    	SSID_Info.rssi = ap_info.rssi;
        //ESP_LOGI(TAG, "SSID: %s, RSSI: %d dBm", ap_info.ssid, ap_info.rssi);
    } else {
        //ESP_LOGE(TAG, "Failed to get AP info. Are you connected to a Wi-Fi network?");
    }
}

/*void check_rssi(void)
{
	uint8_t temp_rssi = 0;
	esp_fill_random(&temp_rssi,1);
	temp_rssi /= 15;
	SSID_Info.rssi = temp_rssi - 95;
}*/

void disable_wifi() {
    // 设置 Wi-Fi 模式为 NULL
    esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_NULL);
    if (ret == ESP_OK) {
        printf("Wi-Fi disabled successfully.\n");
    } else {
        printf("Failed to disable Wi-Fi: %s\n", esp_err_to_name(ret));
    }
}

void wifi_task(void *pvParameters)
{
	uint8_t temp_rssi = 0;
	//read_default_from_flash();
	//modbus_init();
	//debug_info("Finish flash init........");
	//ESP_ERROR_CHECK(ret);

	//if(SSID_Info.MANUEL_EN != 0){Test[18] = 600;
		wifi_init_sta();
	//}



    ESP_LOGI(TAG, "Finish wifi init1");
    task_test.enable[1] = 1;
	while(1)
	{task_test.count[1]++;
		//if(re_init_wifi)
		//	wifi_init_sta();
		//esp_random();
		/*esp_fill_random(&temp_rssi,1);
		temp_rssi /= 15;
		SSID_Info.rssi = temp_rssi - 95;*/
	get_wifi_signal_strength();
	    //Initialize the system event handler
		/*
	    ESP_ERROR_CHECK(esp_event_loop_init(scan_event_handler, NULL));
	    wifi_scan_config_t scanConf = {
			.ssid = NULL,
			.bssid = NULL,
			.channel = 0,
			.show_hidden = 1
			};
		ESP_ERROR_CHECK(esp_wifi_scan_start(&scanConf, 0));*/
		vTaskDelay(3000 / portTICK_PERIOD_MS);
	}
}

void connect_wifi(void)
{
	debug_info("Finish flash init........");
	wifi_init_sta();
	debug_info("Finish wifi init%%%%%%%%%%");
}

// 比较两个 4 字节数组是否相等
bool compare_address(const uint8_t *addr1, const uint8_t *addr2) {
    return memcmp(addr1, addr2, 4) == 0;
}

// 检查地址是否为 0.0.0.0
bool is_address_zero(const uint8_t *addr) {
    return addr[0] == 0 && addr[1] == 0 && addr[2] == 0 && addr[3] == 0;
}

