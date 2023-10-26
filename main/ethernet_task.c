#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tcpip_adapter.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "ethernet_task.h"
#include "define.h"
#include "wifi.h"
#include "flash.h"

void eth_start(void);

static const char *TAG = "ethernet_task";
//uint8_t mac_addr[6] = {0};

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{

    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, Modbus.mac_addr);
        debug_info("Ethernet Link Up");
        //ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
        //         mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        Modbus.ethernet_status = ETHERNET_EVENT_CONNECTED;
        break;
    case ETHERNET_EVENT_DISCONNECTED:
    	debug_info("Ethernet Link Down");
        Modbus.ethernet_status = ETHERNET_EVENT_DISCONNECTED;
        break;
    case ETHERNET_EVENT_START:
    	debug_info("Ethernet Started");
        Modbus.ethernet_status = ETHERNET_EVENT_START;
        break;
    case ETHERNET_EVENT_STOP:
    	debug_info("Ethernet Stopped");
        Modbus.ethernet_status = ETHERNET_EVENT_STOP;
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const tcpip_adapter_ip_info_t *ip_info = &event->ip_info;

    debug_info( "Ethernet Got IP Address");
    debug_info( "~~~~~~~~~~~");
    Modbus.ip_addr[0] = ip4_addr1(&ip_info->ip);
    Modbus.ip_addr[1] = ip4_addr2(&ip_info->ip);
    Modbus.ip_addr[2] = ip4_addr3(&ip_info->ip);
    Modbus.ip_addr[3] = ip4_addr4(&ip_info->ip);
    Modbus.subnet[0] = ip4_addr1(&ip_info->netmask);
    Modbus.subnet[1] = ip4_addr2(&ip_info->netmask);
    Modbus.subnet[2] = ip4_addr3(&ip_info->netmask);
    Modbus.subnet[3] = ip4_addr4(&ip_info->netmask);
    Modbus.getway[0] = ip4_addr1(&ip_info->gw);
    Modbus.getway[1] = ip4_addr2(&ip_info->gw);
    Modbus.getway[2] = ip4_addr3(&ip_info->gw);
    Modbus.getway[3] = ip4_addr4(&ip_info->gw);
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    Modbus.ethernet_status = 4;  // GOT IP

    Save_Ethernet_Info();
    debug_info( "~~~~~~~~~~~");
}

esp_eth_handle_t eth_handle = NULL;
void ethernet_init(void)
{
	esp_err_t ret;
	tcpip_adapter_init(); // Commented by Evan: it is recommanded to be replaced by esp_netif_init();
	ret = esp_event_loop_create_default();

	if(ret == ESP_OK)
		debug_info("esp_event_loop_create_default() finished^^^^^^^^");
	ret = tcpip_adapter_set_default_eth_handlers();
	if(ret == ESP_OK)
		debug_info("tcpip_adapter_set_default_eth_handlers() finished^^^^^^^^");

	// check dhcp mode or static mode
	if(Modbus.tcp_type == 0)  // static mode
	{
		ret = tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_ETH);
		tcpip_adapter_ip_info_t ip_info  = {0};
		ip_info.ip.addr = ESP_IP4TOADDR(Modbus.ip_addr[0],Modbus.ip_addr[1],Modbus.ip_addr[2],Modbus.ip_addr[3]);
		ip_info.netmask.addr = ESP_IP4TOADDR(Modbus.subnet[0], Modbus.subnet[1], Modbus.subnet[2], Modbus.subnet[3]);
		ip_info.gw.addr = ESP_IP4TOADDR(Modbus.getway[0],Modbus.getway[1],Modbus.getway[2],Modbus.getway[3]);
		ret = tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_ETH, &ip_info);
	}

	ret = esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL);
	if(ret == ESP_OK)
		debug_info("esp_event_handler_register(ESP_EVENT_ANY_ID) finished^^^^^^^^");
	ret = esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL);
	if(ret == ESP_OK)
		debug_info("esp_event_handler_register(IP_EVENT_ETH_GOT_IP) finished^^^^^^^^");


	eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
	eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
	phy_config.phy_addr = 1;//CONFIG_EXAMPLE_ETH_PHY_ADDR;
	phy_config.reset_gpio_num = 5;//CONFIG_EXAMPLE_ETH_PHY_RST_GPIO;

	mac_config.smi_mdc_gpio_num = 23;//CONFIG_EXAMPLE_ETH_MDC_GPIO;
	mac_config.smi_mdio_gpio_num = 18;//CONFIG_EXAMPLE_ETH_MDIO_GPIO;
	esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);

	esp_eth_phy_t *phy = esp_eth_phy_new_ip101(&phy_config);
	esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);


//	esp_eth_handle_t eth_handle = NULL;
	if(esp_eth_driver_install(&config, &eth_handle)==ESP_OK){
		debug_info("esp_eth_driver_install finished^^^^^^^^");}
	if(esp_eth_start(eth_handle)){
		debug_info("esp_eth_start finished^^^^^^^^");}
}

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
