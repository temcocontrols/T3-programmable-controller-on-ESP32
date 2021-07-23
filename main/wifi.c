#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "wifi.h"
#include "driver/uart.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "flash.h"
#include "esp_log.h"
#include "deviceparams.h"
//#include "modbus.h"

static const char *TAG = "WIFI";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define EXAMPLE_ESP_MAXIMUM_RETRY	10

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

//STR_SCAN_CMD Scan_Infor;
STR_SSID	SSID_Info;
bool re_init_wifi = false;

static int s_retry_num = 0;

void debug_info(char *string)
{
// #if DEBUG_INFO_UART0
// 	//uart_write_bytes(UART_NUM_0, "\r\n", 1);
// 	uart_write_bytes(UART_NUM_0, (const char *)string, strlen(string));
// 	uart_write_bytes(UART_NUM_0, "\r\n", 2);
// #endif
}

void init_ssid_info()
{
	memset(SSID_Info.name,0,64);
	memset(SSID_Info.password,0,32);
	memcpy(SSID_Info.name, "TP-LINK_wuxian", strlen("TP-LINK_wuxian"));
	memcpy(SSID_Info.password, "87654321", strlen("87654321"));
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            //ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        //ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        const tcpip_adapter_ip_info_t *ip_info = &event->ip_info;
        //ESP_LOGI(TAG, "got ip:%s",
        //         ip4addr_ntoa(&event->ip_info.ip));
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
        SSID_Info.getway[3]= ip4_addr4(&ip_info->gw);
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta()
{
	// init_ssid_info();

    s_wifi_event_group = xEventGroupCreate();

// tcpip_adapter_init();
// ESP_ERROR_CHECK(esp_event_loop_create_default());

    debug_info("esp_event_loop_create_default");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    debug_info("esp_wifi_init");
    ESP_LOGI(TAG, "esp_wifi_init");

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

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    //ESP_LOGI(TAG, "wifi_init_sta finished.");

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
    	holding_reg_params.stm32_uart_send[STM32_LED_WIFI_ON] = 1;
    	SSID_Info.IP_Wifi_Status = WIFI_CONNECTED;
    } else if (bits & WIFI_FAIL_BIT) {
        //ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
        //         EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    	SSID_Info.IP_Wifi_Status = WIFI_DISCONNECTED;
    	holding_reg_params.stm32_uart_send[STM32_LED_WIFI_ON] = 0;
    } else {
        //ESP_LOGE(TAG, "UNEXPECTED EVENT");
    	SSID_Info.IP_Wifi_Status = WIFI_NO_CONNECT;
    	holding_reg_params.stm32_uart_send[STM32_LED_WIFI_ON] = 0;
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);
}

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

void wifi_task(void *pvParameters)
{
	uint8_t temp_rssi=0;
	//read_default_from_flash();
	//modbus_init();
	debug_info("Finish flash init........");
	//ESP_ERROR_CHECK(ret);
	wifi_init_sta();
	debug_info("Finish wifi init%%%%%%%%%%");
    ESP_LOGI(TAG, "Finish wifi init");

	while(1)
	{
		//if(re_init_wifi)
		//	wifi_init_sta();
		//esp_random();
		esp_fill_random(&temp_rssi,1);
		temp_rssi /= 15;
		SSID_Info.rssi = temp_rssi-95;
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
