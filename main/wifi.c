#include "string.h"
#include <stddef.h>
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
#include "esp_heap_caps.h"
//#include "modbus.h"
#include "lwip/sockets.h"
#pragma pack(push)
#include "define.h"
#pragma pack(pop)
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "wifi_web_server.h"

static const char *TAG = "WIFI";
static const char *WIFI_DIAG_TAG = "WIFI_DIAG";
extern SemaphoreHandle_t CountHandle;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_DRIVER_INIT_DONE   BIT0
#define WIFI_DRIVER_INIT_FAILED BIT1
#define EXAMPLE_ESP_MAXIMUM_RETRY	10

void disable_wifi();
/* FreeRTOS event group to signal when we are connected*/
EventGroupHandle_t s_wifi_event_group;
static EventGroupHandle_t s_wifi_driver_init_event_group;
static esp_err_t s_wifi_driver_init_result = ESP_FAIL;
extern EventGroupHandle_t network_EventHandle;
//STR_SCAN_CMD Scan_Infor;
STR_SSID	SSID_Info;
bool ReconnectWithWifi = true;
bool WifiScanComplete = false;
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

            if(ReconnectWithWifi == false)
            {
                break;
            }
            ESP_LOGI(TAG, "Connecting to AP...");
            SSID_Info.IP_Wifi_Status = WIFI_DISCONNECTED;
            if (s_wifi_event_group) {
                xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            }
            esp_wifi_connect();
            break;

        case WIFI_EVENT_SCAN_DONE:
            WifiScanComplete = true;
            break;

        case WIFI_EVENT_STA_DISCONNECTED:

            //wifi_task_running = 0;
            SSID_Info.IP_Wifi_Status = WIFI_DISCONNECTED;
            if (s_wifi_event_group) {
                xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            }
            if(ReconnectWithWifi == false)
            {
                break;
            }

            vTaskDelay(2000 / portTICK_PERIOD_MS);
            wifi_retry_count ++;
            if(wifi_retry_count == 5)
            {
                wifi_start_softap();
                wifi_web_server_start();
            }
            if (wifi_retry_count <= 10)
            {
                esp_wifi_connect();
            }
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
            {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                esp_netif_ip_info_t *ip = &event->ip_info;

                SSID_Info.ip_addr[0] = esp_ip4_addr1(&ip->ip);
                SSID_Info.ip_addr[1] = esp_ip4_addr2(&ip->ip);
                SSID_Info.ip_addr[2] = esp_ip4_addr3(&ip->ip);
                SSID_Info.ip_addr[3] = esp_ip4_addr4(&ip->ip);

                SSID_Info.net_mask[0] = esp_ip4_addr1(&ip->netmask);
                SSID_Info.net_mask[1] = esp_ip4_addr2(&ip->netmask);
                SSID_Info.net_mask[2] = esp_ip4_addr3(&ip->netmask);
                SSID_Info.net_mask[3] = esp_ip4_addr4(&ip->netmask);

                SSID_Info.getway[0] = esp_ip4_addr1(&ip->gw);
                SSID_Info.getway[1] = esp_ip4_addr2(&ip->gw);
                SSID_Info.getway[2] = esp_ip4_addr3(&ip->gw);
                SSID_Info.getway[3] = esp_ip4_addr4(&ip->gw);

                ESP_LOGE(TAG, "IP: " IPSTR, IP2STR(&ip->ip));

                wifi_retry_count = 0;
                SSID_Info.IP_Wifi_Status = WIFI_NORMAL;

                if (s_wifi_event_group)
                    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

                wifi_web_server_stop();

                wifi_mode_t mode;
                esp_wifi_get_mode(&mode);
                if (mode == WIFI_MODE_APSTA) {
                    ESP_LOGI(WIFI_DIAG_TAG, "before esp_wifi_set_mode mode=WIFI_MODE_STA source=got_ip");
                    esp_err_t mode_ret = esp_wifi_set_mode(WIFI_MODE_STA);
                    ESP_LOGI(WIFI_DIAG_TAG, "after esp_wifi_set_mode ret=%s mode=WIFI_MODE_STA source=got_ip",
                             esp_err_to_name(mode_ret));
                }

                break;
            }

        default:
            break;
        }
    }
}

#if 0
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
#endif

#if 1

static bool wifi_initialized = false;
static esp_netif_t *wifi_netif = NULL;

esp_err_t wifi_driver_init_barrier_prepare(void)
{
    if (s_wifi_driver_init_event_group == NULL) {
        s_wifi_driver_init_event_group = xEventGroupCreate();
        if (s_wifi_driver_init_event_group == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    s_wifi_driver_init_result = ESP_FAIL;
    xEventGroupClearBits(s_wifi_driver_init_event_group,
                         WIFI_DRIVER_INIT_DONE | WIFI_DRIVER_INIT_FAILED);
    return ESP_OK;
}

esp_err_t wifi_driver_init_barrier_wait(void)
{
    if (s_wifi_driver_init_event_group == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    xEventGroupWaitBits(s_wifi_driver_init_event_group,
                        WIFI_DRIVER_INIT_DONE | WIFI_DRIVER_INIT_FAILED,
                        pdFALSE,
                        pdFALSE,
                        portMAX_DELAY);
    return s_wifi_driver_init_result;
}

bool wifi_driver_init_barrier_is_done(void)
{
#if !CONFIG_IDF_TARGET_ESP32S3
    return true;
#else
    if (s_wifi_driver_init_event_group == NULL) {
        return false;
    }

    return (xEventGroupGetBits(s_wifi_driver_init_event_group) & WIFI_DRIVER_INIT_DONE) != 0;
#endif
}

static void wifi_driver_init_barrier_signal(esp_err_t result)
{
    if (s_wifi_driver_init_event_group == NULL) {
        return;
    }

    s_wifi_driver_init_result = result;
    xEventGroupSetBits(s_wifi_driver_init_event_group,
                       result == ESP_OK ? WIFI_DRIVER_INIT_DONE : WIFI_DRIVER_INIT_FAILED);
}

static void wifi_log_init_config(const wifi_init_config_t *cfg)
{
    ESP_LOGI(WIFI_DIAG_TAG,
             "cfg layout: sizeof=%u feature_caps_offset=%u magic_offset=%u",
             (unsigned int)sizeof(*cfg),
             (unsigned int)offsetof(wifi_init_config_t, feature_caps),
             (unsigned int)offsetof(wifi_init_config_t, magic));
    ESP_LOGI(WIFI_DIAG_TAG,
             "cfg identity: magic=0x%08x expected=0x%08x match=%d osi=%p expected_osi=%p match=%d",
             (unsigned int)cfg->magic,
             (unsigned int)WIFI_INIT_CONFIG_MAGIC,
             cfg->magic == WIFI_INIT_CONFIG_MAGIC,
             (void *)cfg->osi_funcs,
             (void *)&g_wifi_osi_funcs,
             cfg->osi_funcs == &g_wifi_osi_funcs);
    ESP_LOGI(WIFI_DIAG_TAG,
             "cfg wpa: size=%u version=0x%08x default_match=%d",
             (unsigned int)cfg->wpa_crypto_funcs.size,
             (unsigned int)cfg->wpa_crypto_funcs.version,
             memcmp(&cfg->wpa_crypto_funcs,
                    &g_wifi_default_wpa_crypto_funcs,
                    sizeof(cfg->wpa_crypto_funcs)) == 0);
    ESP_LOGI(WIFI_DIAG_TAG,
             "cfg buffers: static_rx=%d dynamic_rx=%d tx_type=%d static_tx=%d dynamic_tx=%d rx_mgmt_type=%d rx_mgmt_num=%d cache_tx=%d",
             cfg->static_rx_buf_num,
             cfg->dynamic_rx_buf_num,
             cfg->tx_buf_type,
             cfg->static_tx_buf_num,
             cfg->dynamic_tx_buf_num,
             cfg->rx_mgmt_buf_type,
             cfg->rx_mgmt_buf_num,
             cfg->cache_tx_buf_num);
    ESP_LOGI(WIFI_DIAG_TAG,
             "cfg features: csi=%d ampdu_rx=%d ampdu_tx=%d amsdu_tx=%d nvs=%d nano=%d feature_caps=0x%llx",
             cfg->csi_enable,
             cfg->ampdu_rx_enable,
             cfg->ampdu_tx_enable,
             cfg->amsdu_tx_enable,
             cfg->nvs_enable,
             cfg->nano_enable,
             (unsigned long long)cfg->feature_caps);
    ESP_LOGI(WIFI_DIAG_TAG,
             "cfg runtime: rx_ba_win=%d core=%d beacon_max_len=%d mgmt_sbuf_num=%d espnow_max_encrypt_num=%d tx_hetb_queue_num=%d",
             cfg->rx_ba_win,
             cfg->wifi_task_core_id,
             cfg->beacon_max_len,
             cfg->mgmt_sbuf_num,
             cfg->espnow_max_encrypt_num,
             cfg->tx_hetb_queue_num);
}

#if CONFIG_IDF_TARGET_ESP32S3 && TSTAT11_WIFI_MINIMAL_DIAG
static esp_err_t wifi_minimal_init_diag(void)
{
    ESP_LOGW(WIFI_DIAG_TAG,
             "minimal init enabled: no STA/AP netif, event handler, mode, start, mDNS, web, or IP services");

    ESP_LOGI(WIFI_DIAG_TAG, "minimal: before esp_netif_init");
    esp_err_t ret = esp_netif_init();
    ESP_LOGI(WIFI_DIAG_TAG, "minimal: after esp_netif_init ret=%s", esp_err_to_name(ret));
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        wifi_driver_init_barrier_signal(ret);
        return ret;
    }

    ESP_LOGI(WIFI_DIAG_TAG, "minimal: before esp_event_loop_create_default");
    ret = esp_event_loop_create_default();
    ESP_LOGI(WIFI_DIAG_TAG, "minimal: after esp_event_loop_create_default ret=%s", esp_err_to_name(ret));
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        wifi_driver_init_barrier_signal(ret);
        return ret;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    ESP_LOGI(WIFI_DIAG_TAG,
             "task=%s core=%d free_internal=%u free_psram=%u sizeof(wifi_init_config_t)=%u",
             pcTaskGetName(current_task),
             xPortGetCoreID(),
             (unsigned int)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             (unsigned int)sizeof(wifi_init_config_t));
    wifi_log_init_config(&cfg);

    ESP_LOGI(WIFI_DIAG_TAG, "minimal: before esp_wifi_init");
    ret = esp_wifi_init(&cfg);
    ESP_LOGI(WIFI_DIAG_TAG, "minimal: after esp_wifi_init ret=%s", esp_err_to_name(ret));
    if (ret != ESP_OK) {
        wifi_driver_init_barrier_signal(ret);
    }
    return ret;
}
#endif

void wifi_init_sta(void)
{
    esp_err_t ret;

    if (!wifi_initialized)
    {
        s_wifi_event_group = xEventGroupCreate();
        CountHandle = xSemaphoreCreateCounting(7,7);

        ESP_LOGI(WIFI_DIAG_TAG, "before esp_netif_init");
        ret = esp_netif_init();
        ESP_LOGI(WIFI_DIAG_TAG, "after esp_netif_init ret=%s", esp_err_to_name(ret));
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            debug_info("esp_netif_init failed");
        }

        ESP_LOGI(WIFI_DIAG_TAG, "before esp_event_loop_create_default");
        ret = esp_event_loop_create_default();
        ESP_LOGI(WIFI_DIAG_TAG, "after esp_event_loop_create_default ret=%s", esp_err_to_name(ret));
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
            debug_info("event loop create failed");
        }

        if (wifi_netif == NULL)
        {
            wifi_netif = esp_netif_create_default_wifi_sta();

            if (wifi_netif == NULL)
            {
                debug_info("Failed to create STA netif");
                wifi_driver_init_barrier_signal(ESP_ERR_NO_MEM);
                return;
            }
        }

        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
        ESP_LOGI(WIFI_DIAG_TAG,
                 "task=%s core=%d free_internal=%u free_psram=%u sizeof(wifi_init_config_t)=%u",
                 pcTaskGetName(current_task),
                 xPortGetCoreID(),
                 (unsigned int)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                 (unsigned int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                 (unsigned int)sizeof(wifi_init_config_t));
            wifi_log_init_config(&cfg);
        ESP_LOGI(WIFI_DIAG_TAG, "before esp_wifi_init");
        ret = esp_wifi_init(&cfg);
        ESP_LOGI(WIFI_DIAG_TAG, "after esp_wifi_init ret=%s", esp_err_to_name(ret));
        wifi_driver_init_barrier_signal(ret);
        ESP_ERROR_CHECK(ret);

    }
    else
    {
        esp_wifi_stop();
    }

    if(SSID_Info.MANUEL_EN != 1 || Modbus.mini_type == PROJECT_HUB)
    {
        wifi_start_softap();
        ESP_LOGI(WIFI_DIAG_TAG, "before esp_wifi_start mode=SoftAP");
        ret = esp_wifi_start();
        ESP_LOGI(WIFI_DIAG_TAG, "after esp_wifi_start ret=%s mode=SoftAP", esp_err_to_name(ret));
        init_mdns_service();
        wifi_web_server_start();
        return;
    }

    /* -------- STATIC IP -------- */
    if(SSID_Info.IP_Auto_Manual == 1)
    {
        esp_netif_dhcpc_stop(wifi_netif);

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

        esp_netif_set_ip_info(wifi_netif, &info_t);

        if(info_t.gw.addr != 0)
        {
            esp_netif_dns_info_t dns_info = {0};

            IP_ADDR4(&dns_info.ip,
                SSID_Info.getway[0],
                SSID_Info.getway[1],
                SSID_Info.getway[2],
                SSID_Info.getway[3]);

            esp_netif_set_dns_info(wifi_netif, ESP_NETIF_DNS_MAIN, &dns_info);

            IP_ADDR4(&dns_info.ip, 8,8,8,8);
            esp_netif_set_dns_info(wifi_netif, ESP_NETIF_DNS_BACKUP, &dns_info);

            IP_ADDR4(&dns_info.ip, 8,8,4,4);
            esp_netif_set_dns_info(wifi_netif, ESP_NETIF_DNS_FALLBACK, &dns_info);
        }
    }
    else
    {
        esp_netif_dhcpc_start(wifi_netif);
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

    ESP_LOGI(WIFI_DIAG_TAG, "before esp_wifi_set_mode mode=WIFI_MODE_STA");
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    ESP_LOGI(WIFI_DIAG_TAG, "after esp_wifi_set_mode ret=%s mode=WIFI_MODE_STA", esp_err_to_name(ret));
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    ESP_LOGI(WIFI_DIAG_TAG, "before esp_wifi_start mode=WIFI_MODE_STA");
    ret = esp_wifi_start();
    ESP_LOGI(WIFI_DIAG_TAG, "after esp_wifi_start ret=%s mode=WIFI_MODE_STA", esp_err_to_name(ret));
    init_mdns_service();

    if (!wifi_initialized)
    {
        wifi_initialized = true;
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
	        debug_info("wifi failed - starting SoftAP");
	        wifi_start_softap();
	        wifi_web_server_start();
	    } else {
	        debug_info("wifi timeout - starting SoftAP");
	        wifi_start_softap();
	        wifi_web_server_start();
	    }
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
    esp_log_level_set("wifi", ESP_LOG_ERROR);

#if CONFIG_IDF_TARGET_ESP32S3 && TSTAT11_WIFI_MINIMAL_DIAG
    if (Modbus.mini_type != PROJECT_HUB)
    {
        esp_err_t ret = wifi_minimal_init_diag();
        ESP_LOGW(WIFI_DIAG_TAG, "minimal init finished: %s; deleting wifi_task", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }
#endif

    wifi_init_sta();

    ESP_LOGI(TAG, "Finish wifi init1");
    task_test.enable[1] = 1;
	while(1)
	{
        task_test.count[1]++;
	    get_wifi_signal_strength();
		vTaskDelay(3000 / portTICK_PERIOD_MS);
	}
}

void connect_wifi(void)
{
	debug_info("Start Wifi init........");
	wifi_init_sta();
	debug_info("Finish Wifi init........");
}

// 比较两个 4 字节数组是否相等
bool compare_address(const uint8_t *addr1, const uint8_t *addr2) {
    return memcmp(addr1, addr2, 4) == 0;
}

// 检查地址是否为 0.0.0.0
bool is_address_zero(const uint8_t *addr) {
    return addr[0] == 0 && addr[1] == 0 && addr[2] == 0 && addr[3] == 0;
}

