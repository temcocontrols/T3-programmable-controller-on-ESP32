#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "ethernet_task.h"
#include "hub_w5500.h"
#include "define.h"
#include "wifi.h"
#include "esp_netif_ip_addr.h"
#include "flash.h"


#include "lwip/dns.h"
void eth_start(void);

static const char *TAG = "ethernet_task";
//uint8_t mac_addr[6] = {0};

#define W5500_PHYCFGR_REG_ADDR          ((uint32_t)(0x002E << 16))

static const char *eth_event_name(int32_t event_id)
{
    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        return "CONNECTED";
    case ETHERNET_EVENT_DISCONNECTED:
        return "DISCONNECTED";
    case ETHERNET_EVENT_START:
        return "START";
    case ETHERNET_EVENT_STOP:
        return "STOP";
    default:
        return "UNKNOWN";
    }
}

static void eth_log_mac(const char *prefix, const uint8_t mac[6])
{
    ESP_LOGI(TAG, "%s %02x:%02x:%02x:%02x:%02x:%02x",
             prefix,
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{

    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;
    ESP_LOGI(TAG, "ETH event: %s (%ld)", eth_event_name(event_id), (long)event_id);
    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        if (esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, Modbus.mac_addr) == ESP_OK) {
            eth_log_mac("Ethernet Link Up, MAC:", Modbus.mac_addr);
        } else {
            ESP_LOGW(TAG, "Ethernet Link Up, MAC read failed");
        }
        debug_info("Ethernet Link Up");
        //ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
        //         mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        Modbus.ethernet_status = ETHERNET_EVENT_CONNECTED;
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Ethernet Link Down");
    	debug_info("Ethernet Link Down");
        Modbus.ethernet_status = ETHERNET_EVENT_DISCONNECTED;
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
    	debug_info("Ethernet Started");
        Modbus.ethernet_status = ETHERNET_EVENT_START;
        break;
    case ETHERNET_EVENT_STOP:Test[19]++;
        ESP_LOGW(TAG, "Ethernet Stopped");
    	debug_info("Ethernet Stopped");
        Modbus.ethernet_status = ETHERNET_EVENT_STOP;
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg,
                                 esp_event_base_t event_base,
                                 int32_t event_id,
                                 void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    Test[1]++;
    debug_info("Ethernet Got IP Address");
    debug_info("~~~~~~~~~~~");

    ESP_LOGI(TAG, "Ethernet Got IP Address");

    Modbus.ip_addr[0] = esp_ip4_addr1(&ip_info->ip);
    Modbus.ip_addr[1] = esp_ip4_addr2(&ip_info->ip);
    Modbus.ip_addr[2] = esp_ip4_addr3(&ip_info->ip);
    Modbus.ip_addr[3] = esp_ip4_addr4(&ip_info->ip);

    Modbus.subnet[0] = esp_ip4_addr1(&ip_info->netmask);
    Modbus.subnet[1] = esp_ip4_addr2(&ip_info->netmask);
    Modbus.subnet[2] = esp_ip4_addr3(&ip_info->netmask);
    Modbus.subnet[3] = esp_ip4_addr4(&ip_info->netmask);

    Modbus.getway[0] = esp_ip4_addr1(&ip_info->gw);
    Modbus.getway[1] = esp_ip4_addr2(&ip_info->gw);
    Modbus.getway[2] = esp_ip4_addr3(&ip_info->gw);
    Modbus.getway[3] = esp_ip4_addr4(&ip_info->gw);

    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));

    Modbus.ethernet_status = 4;  // GOT IP

#if 1 // DNS
    if ((Modbus.getway[0] != 0) ||
        (Modbus.getway[1] != 0) ||
        (Modbus.getway[2] != 0) ||
        (Modbus.getway[3] != 0))
    {
        esp_netif_dns_info_t dns_info = {0};

        // MAIN DNS = Gateway
        IP4_ADDR(&dns_info.ip.u_addr.ip4,
                     Modbus.getway[0],
                     Modbus.getway[1],
                     Modbus.getway[2],
                     Modbus.getway[3]);
        dns_info.ip.type = ESP_IPADDR_TYPE_V4;

        esp_netif_t *eth_netif = esp_netif_get_handle_from_ifkey("ETH_DEF");

        ESP_ERROR_CHECK(esp_netif_set_dns_info(eth_netif,
                                               ESP_NETIF_DNS_MAIN,
                                               &dns_info));

        // BACKUP DNS = 8.8.8.8
        IP4_ADDR(&dns_info.ip.u_addr.ip4, 8, 8, 8, 8);
        ESP_ERROR_CHECK(esp_netif_set_dns_info(eth_netif,
                                               ESP_NETIF_DNS_BACKUP,
                                               &dns_info));

        // FALLBACK DNS = 8.8.4.4
        IP4_ADDR(&dns_info.ip.u_addr.ip4, 8, 8, 4, 4);
        ESP_ERROR_CHECK(esp_netif_set_dns_info(eth_netif,
                                               ESP_NETIF_DNS_FALLBACK,
                                               &dns_info));
    }
#endif

    multicast_addr = Get_multicast_addr((unsigned char *)&Modbus.ip_addr);

    Save_Ethernet_Info();

    debug_info("~~~~~~~~~~~");
}

esp_eth_handle_t eth_handle = NULL;

extern uint8_t count_reboot;

static void w5500_status_poll_task(void *pvParameters)
{
    esp_eth_handle_t handle = (esp_eth_handle_t)pvParameters;
    uint32_t count = 0;

    while (1) {
        uint32_t phycfg = 0;
        esp_eth_phy_reg_rw_data_t reg = {
            .reg_addr = W5500_PHYCFGR_REG_ADDR,
            .reg_value_p = &phycfg,
        };
        esp_err_t phy_ret = esp_eth_ioctl(handle, ETH_CMD_READ_PHY_REG, &reg);

        eth_speed_t speed = ETH_SPEED_10M;
        eth_duplex_t duplex = ETH_DUPLEX_HALF;
        esp_err_t speed_ret = esp_eth_ioctl(handle, ETH_CMD_G_SPEED, &speed);
        esp_err_t duplex_ret = esp_eth_ioctl(handle, ETH_CMD_G_DUPLEX_MODE, &duplex);

        if (phy_ret == ESP_OK) {
            ESP_LOGI(TAG,
                     "W5500 poll[%lu]: PHYCFGR=0x%02lx link=%lu speed_bit=%lu duplex_bit=%lu opmode=%lu opsel=%lu reset=%lu driver_speed=%s(%s) driver_duplex=%s(%s) eth_status=%u",
                     (unsigned long)count,
                     phycfg & 0xff,
                     phycfg & 0x01,
                     (phycfg >> 1) & 0x01,
                     (phycfg >> 2) & 0x01,
                     (phycfg >> 3) & 0x07,
                     (phycfg >> 6) & 0x01,
                     (phycfg >> 7) & 0x01,
                     speed == ETH_SPEED_100M ? "100M" : "10M",
                     esp_err_to_name(speed_ret),
                     duplex == ETH_DUPLEX_FULL ? "full" : "half",
                     esp_err_to_name(duplex_ret),
                     Modbus.ethernet_status);
        } else {
            ESP_LOGE(TAG,
                     "W5500 poll[%lu]: PHYCFGR read failed: %s speed_ret=%s duplex_ret=%s eth_status=%u",
                     (unsigned long)count,
                     esp_err_to_name(phy_ret),
                     esp_err_to_name(speed_ret),
                     esp_err_to_name(duplex_ret),
                     Modbus.ethernet_status);
        }

        count++;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static void w5500_force_phy_all_capable(esp_eth_handle_t handle)
{
    uint32_t phycfg = 0xf8;
    esp_eth_phy_reg_rw_data_t reg = {
        .reg_addr = W5500_PHYCFGR_REG_ADDR,
        .reg_value_p = &phycfg,
    };

    esp_err_t ret = esp_eth_ioctl(handle, ETH_CMD_WRITE_PHY_REG, &reg);
    ESP_LOGW(TAG, "W5500 force PHYCFGR write 0x%02lx: %s", phycfg & 0xff, esp_err_to_name(ret));

    vTaskDelay(pdMS_TO_TICKS(20));

    phycfg = 0;
    ret = esp_eth_ioctl(handle, ETH_CMD_READ_PHY_REG, &reg);
    if (ret == ESP_OK) {
        ESP_LOGW(TAG,
                 "W5500 force PHYCFGR readback: 0x%02lx link=%lu speed_bit=%lu duplex_bit=%lu opmode=%lu opsel=%lu reset=%lu",
                 phycfg & 0xff,
                 phycfg & 0x01,
                 (phycfg >> 1) & 0x01,
                 (phycfg >> 2) & 0x01,
                 (phycfg >> 3) & 0x07,
                 (phycfg >> 6) & 0x01,
                 (phycfg >> 7) & 0x01);
    } else {
        ESP_LOGE(TAG, "W5500 force PHYCFGR readback failed: %s", esp_err_to_name(ret));
    }
}

static esp_err_t ethernet_attach_and_start(esp_netif_t *eth_netif)
{
    esp_err_t ret = ESP_OK;

    if (eth_handle != NULL)
    {
        ESP_LOGI(TAG, "Attaching Ethernet netif to driver handle %p", eth_handle);
        ret = esp_netif_attach(eth_netif,
            esp_eth_new_netif_glue(eth_handle));

        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "esp_netif_attach OK, starting Ethernet");
            ret = esp_eth_start(eth_handle);
            if(ret == ESP_OK) {
                ESP_LOGI(TAG, "esp_eth_start OK");
                debug_info("esp_eth_start finished^^^^^^^^");
                w5500_force_phy_all_capable(eth_handle);
                xTaskCreate(w5500_status_poll_task, "w5500_poll", 4096, eth_handle, 4, NULL);
            } else {
                ESP_LOGE(TAG, "esp_eth_start failed: %s", esp_err_to_name(ret));
                debug_info("esp_eth_start failed");
            }
        }
        else
        {
            ESP_LOGE(TAG, "esp_netif_attach failed: %s", esp_err_to_name(ret));
            debug_info("esp_netif_attach failed");
        }
    }
    else
    {
        ESP_LOGE(TAG, "Ethernet not started (handle NULL)");
        debug_info("Ethernet not started (handle NULL)");
        ret = ESP_FAIL;
    }

    return ret;
}

esp_err_t ethernet_init(void)
{
    esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "ethernet_init begin: mini_type=%u tcp_type=%u", Modbus.mini_type, Modbus.tcp_type);

    ESP_ERROR_CHECK(esp_netif_init());

    ret = esp_event_loop_create_default();
    if(ret == ESP_OK) {
        ESP_LOGI(TAG, "esp_event_loop_create_default OK");
        debug_info("esp_event_loop_create_default() finished^^^^^^^^");
    } else if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "default event loop already exists");
    } else {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(ret));
    }
    // else
    //  Test[0]++;

    /* tcpip_adapter_set_default_eth_handlers(); */
    // Not required in IDF5 (handled by esp_netif)
    debug_info("tcpip_adapter_set_default_eth_handlers() finished^^^^^^^^");

    /* Create default ETH netif */
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&cfg);
    if (!eth_netif)
    {
        ESP_LOGE(TAG, "eth_netif creation failed");
        debug_info("eth_netif creation failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "eth_netif created: %p", eth_netif);

    // check dhcp mode or static mode
    if(Modbus.tcp_type == 0)  // static mode
    {
        Test[2]++;

        esp_netif_dhcpc_stop(eth_netif);

        esp_netif_ip_info_t ip_info  = {0};

        ip_info.ip.addr = ESP_IP4TOADDR(
            Modbus.ip_addr[0],Modbus.ip_addr[1],
            Modbus.ip_addr[2],Modbus.ip_addr[3]);

        ip_info.netmask.addr = ESP_IP4TOADDR(
            Modbus.subnet[0], Modbus.subnet[1],
            Modbus.subnet[2], Modbus.subnet[3]);

        ip_info.gw.addr = ESP_IP4TOADDR(
            Modbus.getway[0],Modbus.getway[1],
            Modbus.getway[2],Modbus.getway[3]);

        esp_netif_set_ip_info(eth_netif, &ip_info);

        ESP_LOGI(TAG, "Ethernet static IP configured: ip=" IPSTR " mask=" IPSTR " gw=" IPSTR,
             IP2STR(&ip_info.ip), IP2STR(&ip_info.netmask), IP2STR(&ip_info.gw));

        debug_info("tcpip_adapter_set_ip_info() finished^^^^^^^^");

#if 1//DNS
        esp_netif_dns_info_t dns_info = {0};

        if(ip_info.gw.addr != 0)
        {
            IP4_ADDR(&dns_info.ip.u_addr.ip4,
                Modbus.getway[0],Modbus.getway[1],
                Modbus.getway[2],Modbus.getway[3]);

            dns_info.ip.type = IPADDR_TYPE_V4;

            esp_netif_set_dns_info(
                eth_netif,
                ESP_NETIF_DNS_MAIN,
                &dns_info);

            IP4_ADDR(&dns_info.ip.u_addr.ip4,8,8,8,8);
            esp_netif_set_dns_info(
                eth_netif,
                ESP_NETIF_DNS_BACKUP,
                &dns_info);

            IP4_ADDR(&dns_info.ip.u_addr.ip4,8,8,4,4);
            esp_netif_set_dns_info(
                eth_netif,
                ESP_NETIF_DNS_FALLBACK,
                &dns_info);
        }
#endif
    }

    ret = esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL);
    if(ret == ESP_OK) {
        ESP_LOGI(TAG, "ETH event handler registered");
        debug_info("esp_event_handler_register(ESP_EVENT_ANY_ID) finished^^^^^^^^");
    } else {
        ESP_LOGE(TAG, "ETH event handler register failed: %s", esp_err_to_name(ret));
    }
    // else
    //  ;//Test[4]++;

    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL);
    if(ret == ESP_OK) {
        ESP_LOGI(TAG, "ETH got-ip handler registered");
        debug_info("esp_event_handler_register(IP_EVENT_ETH_GOT_IP) finished^^^^^^^^");
    } else {
        ESP_LOGE(TAG, "ETH got-ip handler register failed: %s", esp_err_to_name(ret));
    }
    // else
    //  ;//Test[5]++;

#if CONFIG_IDF_TARGET_ESP32

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

    /* Configure ESP32 EMAC specific config */
    eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();

    if( Modbus.mini_type == MINI_SMALL_ARM ||
        Modbus.mini_type == MINI_BIG_ARM ||
        Modbus.mini_type == PROJECT_CO2)
    {
        esp32_emac_config.smi_gpio.mdc_num = 23;
        esp32_emac_config.smi_gpio.mdio_num = 18;
    }

    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(
        &esp32_emac_config,
        &mac_config
    );

    if (!mac)
    {
        debug_info("MAC init failed");
        return ESP_FAIL;
    }

    phy_config.phy_addr = 1;//CONFIG_EXAMPLE_ETH_PHY_ADDR;
    phy_config.reset_gpio_num = 5;//CONFIG_EXAMPLE_ETH_PHY_RST_GPIO;

    esp_eth_phy_t *phy = esp_eth_phy_new_ip101(&phy_config);

    if (!phy)
    {
        debug_info("PHY init failed");
        return ESP_FAIL;
    }

    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);

    /* ===== YOUR ORIGINAL CONDITION PRESERVED ===== */
    if( Modbus.mini_type == MINI_SMALL_ARM ||
        Modbus.mini_type == MINI_BIG_ARM ||
        Modbus.mini_type == PROJECT_CO2)
    {
        if(esp_eth_driver_install(&config, &eth_handle)==ESP_OK){
            debug_info("esp_eth_driver_install finished^^^^^^^^");
        }
        else
        {
            eth_handle = NULL;  // prevent crash
            if(count_reboot < 5)
                esp_restart();
        }
    }
    else
    {
        if((ret = esp_eth_driver_install(&config, &eth_handle))==ESP_OK)
            debug_info("esp_eth_driver_install finished^^^^^^^^");
        else
            eth_handle = NULL;  // prevent crash
    }
    /* ============================================= */

#endif

#if CONFIG_IDF_TARGET_ESP32S3
    if(Modbus.mini_type == PROJECT_HUB)
    {
        ESP_LOGI(TAG, "Installing PROJECT_HUB W5500 driver");
        ret = hub_w5500_install(&eth_handle);
        if(ret == ESP_OK) {
            ESP_LOGI(TAG, "hub_w5500_install OK, handle=%p", eth_handle);
            debug_info("hub_w5500_install finished^^^^^^^^");
        } else {
            ESP_LOGE(TAG, "hub_w5500_install failed: %s", esp_err_to_name(ret));
            eth_handle = NULL;
        }
    }
    else
    {
        ESP_LOGW(TAG, "ESP32S3 Ethernet requires PROJECT_HUB W5500 config, mini_type=%u", Modbus.mini_type);
        debug_info("ESP32S3 Ethernet requires PROJECT_HUB W5500 config");
    }
#endif

    if(eth_handle != NULL)
        ret = ethernet_attach_and_start(eth_netif);

    ESP_LOGI(TAG, "ethernet_init end: ret=%s eth_handle=%p status=%u",
             esp_err_to_name(ret), eth_handle, Modbus.ethernet_status);

#if 1//DNS
//  dns_init();
#endif

    return ret;
}
#if 0
extern uint8_t count_reboot;
void ethernet_check_task( void *pvParameters)
{
	for(;;){
		if((ESP_ERR_NOT_FINISHED == eth_status)&&(count_reboot<6)){
			ethernet_init();
		}
		vTaskDelay(1000/portTICK_PERIOD_MS);
	}
}
#endif

void eth_start(void)
{
	/*uint8_t ret;
	//esp_eth_start(eth_handle);ret = esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL);
	ret = esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL);
	if(ret == ESP_OK)
	{
		debug_info("esp_event_handler_register(IP_EVENT_ETH_GOT_IP) finished^^^^^^^^");
	}*/
}
