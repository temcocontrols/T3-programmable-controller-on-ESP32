#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
//#include "esp_wifi.h"
#include "esp_event.h"
#include "wifi.h"
#include "driver/uart.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "flash.h"
#include "esp_log.h"
//#include "modbus.h"
#include "lwip/sockets.h"
#include "define.h"


static const char *TAG = "WIFI";
extern xSemaphoreHandle CountHandle;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define EXAMPLE_ESP_MAXIMUM_RETRY	10


/* FreeRTOS event group to signal when we are connected*/
EventGroupHandle_t s_wifi_event_group;
extern EventGroupHandle_t network_EventHandle;
//STR_SCAN_CMD Scan_Infor;
STR_SSID	SSID_Info;
bool re_init_wifi = false;
extern unsigned short int Test[50];
static int s_retry_num = 0;
TaskHandle_t Task_handle[7];
extern int task_sock[7];
char debug_array[100];
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
#if DEBUG_INFO_UART0
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
//Fandu : ���ú��� esp_wifi_connect()
//�� wifi �����ٴγ������ȵ㽨�����ӡ�������������ɹ�������ٴν��� CONNECT��GOTIP ������״̬��
//�����������ʧ�ܣ����ٴν��� DISCONNECT ״̬�����η���ѭ����ֱ�����ӳɹ�Ϊֹ�� ��������
//��̫����Ϊʲô event_handler Ϊʲô������ʱ�򲻴��� SYSTEM_EVENT_STA_DISCONNECTED
esp_err_t event_handler_2(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "Connecting to AP...");
        debug_info("event_handler_2 esp_wifi_connect()");
        esp_wifi_connect();
        SSID_Info.IP_Wifi_Status = WIFI_CONNECTED;
        if(SSID_Info.IP_Auto_Manual == 1)
        	SSID_Info.IP_Wifi_Status = WIFI_NORMAL;
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "Connected.");
        debug_info("event_handler_2 SYSTEM_EVENT_STA_GOT_IP");
        wifi_retry_count = 0;
        //wifi_task_running = 1;
        //xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        SSID_Info.IP_Wifi_Status = WIFI_NORMAL;
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
    	//wifi_task_running = 0;
    	 SSID_Info.IP_Wifi_Status = WIFI_DISCONNECTED;
    	//debug_info("Wifi disconnected, try to connect ...");
	   if(0)
	   {// wifi
    	for(int i=0 ;i<7;i++)
    	{
			char temp_test[50];
    		if(Task_handle[i] != 0)
    		{
    			sprintf(temp_test,"shutdown sock %d\r",i);
    			debug_info(temp_test);
    			shutdown(task_sock[i],2);
    			close(task_sock[i]);
    			task_sock[i] = NULL;
    			vTaskDelete( Task_handle[i] );
    			Task_handle[i] = 0;

    			if(CountHandle != NULL)
    			{
    				if(xSemaphoreGive(CountHandle) != pdTRUE)
    				{
    					debug_info("Disconnected Try to Give semaphore and failed!");
    				}
    				else
    					debug_info("Disconnected Give semaphore success!");
    			}

    			vTaskDelay(1000 / portTICK_RATE_MS);
    		}
    		//sprintf(temp_test, "Task_handle[%d] =  %d",i,(int)Task_handle[i]);
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
    	vTaskDelay(2000 / portTICK_RATE_MS);
    	wifi_retry_count ++;
    	//if(wifi_retry_count < 10)
    		esp_wifi_connect();
    	/*else
    	{
    		//wifi_retry_count = 0;
    		//xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    	//	debug_info("run wifi_init_sta()");
    	//	wifi_init_sta();
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

    return ESP_OK;
}



static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{


    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        debug_info("esp_wifi_connect()");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
        	debug_info("s_retry_num ++ ");
            esp_wifi_connect();
            s_retry_num++;
            //ESP_LOGI(TAG, "retry to connect to the AP");
        } else
        {
        	debug_info("s_retry_num  big ,stop try!");
        	 esp_wifi_connect();
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        //ESP_LOGI(TAG,"connect to the AP fail");
    } else
    	if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        const tcpip_adapter_ip_info_t *ip_info = &event->ip_info;
        //ESP_LOGI(TAG, "got ip:%s",
         //        ip4addr_ntoa(&event->ip_info.ip));
        debug_info("wifi got ip!");
        SSID_Info.ip_addr[0] = ip4_addr1(&ip_info->ip);
        SSID_Info.ip_addr[1] = ip4_addr2(&ip_info->ip);
        SSID_Info.ip_addr[2] = ip4_addr3(&ip_info->ip);
        SSID_Info.ip_addr[3] = ip4_addr4(&ip_info->ip);
        SSID_Info.net_mask[0] = ip4_addr1(&ip_info->netmask);
        SSID_Info.net_mask[1] = ip4_addr2(&ip_info->netmask);
        SSID_Info.net_mask[2] = ip4_addr3(&ip_info->netmask);
        SSID_Info.net_mask[3] = ip4_addr4(&ip_info->netmask);
        SSID_Info.getway[0] = ip4_addr1(&ip_info->gw);
        SSID_Info.getway[1] = ip4_addr2(&ip_info->gw);
        SSID_Info.getway[2] = ip4_addr3(&ip_info->gw);
        SSID_Info.getway[3] = ip4_addr4(&ip_info->gw);
        SSID_Info.IP_Wifi_Status = WIFI_NORMAL;
        save_wifi_info();
        s_retry_num = 0;
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
void wifi_init_sta()
{
	//init_ssid_info();

    s_wifi_event_group = xEventGroupCreate();

    CountHandle = xSemaphoreCreateCounting(7,7);
	if(s_wifi_event_group == NULL)
	{
		debug_info("Create event group failed!");
	}
	else
		debug_info("Create event group success!");

	ESP_ERROR_CHECK(esp_event_loop_init(event_handler_2,NULL));



    debug_info("esp_event_loop_create_default");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    debug_info("esp_wifi_init");
    ESP_LOGI(TAG, "esp_wifi_init");

    //--------------Add IP Set---------------
    if(SSID_Info.IP_Auto_Manual == 1)
    {

   	esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    assert(netif);

	esp_netif_dhcpc_stop(netif);
	esp_netif_ip_info_t info_t = {0};
	info_t.ip.addr = ESP_IP4TOADDR(SSID_Info.ip_addr[0],SSID_Info.ip_addr[1],SSID_Info.ip_addr[2],SSID_Info.ip_addr[3]);
	info_t.netmask.addr = ESP_IP4TOADDR(SSID_Info.net_mask[0],SSID_Info.net_mask[1],SSID_Info.net_mask[2],SSID_Info.net_mask[3]);
	info_t.gw.addr = ESP_IP4TOADDR(SSID_Info.getway[0], SSID_Info.getway[1], SSID_Info.getway[2], SSID_Info.getway[3]);
	esp_netif_set_ip_info(netif,&info_t);

    }
    //--------------Add IP Set---------------

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    if(SSID_Info.name[0]!=0)
    {
    	memcpy(wifi_config.sta.ssid, SSID_Info.name, 32);
    	memcpy(wifi_config.sta.password, SSID_Info.password, 32);
    }
    else
    {
    	init_ssid_info();
    }
    SSID_Info.rev = 4;
    if(SSID_Info.bacnet_port == 0)
    	SSID_Info.bacnet_port = 47808;
    if(SSID_Info.modbus_port == 0)
    	SSID_Info.modbus_port = 502;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    //ESP_LOGI(TAG, "wifi_init_sta finished.");
    debug_info("wifi_init_sta finished.");
    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        //ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
        //         EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    	debug_info("wifi connected\r");
    	//SSID_Info.IP_Wifi_Status = WIFI_CONNECTED;
    } else if (bits & WIFI_FAIL_BIT) {
        //ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
        //         EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    	debug_info("wifi disconnected\r");
    	//SSID_Info.IP_Wifi_Status = WIFI_DISCONNECTED;
    } else {
        //ESP_LOGE(TAG, "UNEXPECTED EVENT");
    	debug_info("wifi no connect\r");
    	//SSID_Info.IP_Wifi_Status = WIFI_NO_CONNECT;
    }

    debug_info("esp_wifi_init");

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));

    vEventGroupDelete(s_wifi_event_group);

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

void check_rssi(void)
{
	uint8_t temp_rssi = 0;
	esp_fill_random(&temp_rssi,1);
	temp_rssi /= 15;
	SSID_Info.rssi = temp_rssi - 95;
}


void wifi_task(void *pvParameters)
{
	uint8_t temp_rssi=0;
	//read_default_from_flash();
	//modbus_init();
	//debug_info("Finish flash init........");
	//ESP_ERROR_CHECK(ret);

	if(SSID_Info.MANUEL_EN == 1){
		wifi_init_sta();
	}


    ESP_LOGI(TAG, "Finish wifi init");
    task_test.enable[1] = 1;
	while(1)
	{task_test.count[1]++;
		//if(re_init_wifi)
		//	wifi_init_sta();
		//esp_random();
		esp_fill_random(&temp_rssi,1);
		temp_rssi /= 15;
		SSID_Info.rssi = temp_rssi - 95;
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
		vTaskDelay(3000 / portTICK_RATE_MS);
	}
}

void connect_wifi(void)
{
	debug_info("Finish flash init........");
	wifi_init_sta();
	debug_info("Finish wifi init%%%%%%%%%%");
}
