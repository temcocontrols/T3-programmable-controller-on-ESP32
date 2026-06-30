#include "hub_network_manager.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

static const char *TAG = "hub_net_mgr";

static hub_network_manager_status_t s_network_status;

static void hub_network_manager_copy_string(char *dest, size_t dest_len, const char *src)
{
    if ((dest == NULL) || (dest_len == 0)) {
        return;
    }

    snprintf(dest, dest_len, "%s", (src != NULL) ? src : "");
}

static hub_network_interface_t hub_network_manager_select_active_interface(void)
{
    if (s_network_status.eth_link_up && s_network_status.eth_has_ip) {
        return HUB_NETWORK_INTERFACE_ETHERNET;
    }
    if (s_network_status.lte_connected) {
        return HUB_NETWORK_INTERFACE_LTE;
    }

    return HUB_NETWORK_INTERFACE_NONE;
}

static void hub_network_manager_log_status(void)
{
    ESP_LOGI(TAG,
             "status: eth(link=%d ip=%d) lte(connected=%d ip=%s) active=%s",
             s_network_status.eth_link_up,
             s_network_status.eth_has_ip,
             s_network_status.lte_connected,
             s_network_status.lte_ip_addr[0] != '\0' ? s_network_status.lte_ip_addr : "-",
             hub_network_manager_interface_name(s_network_status.active_interface));
}

static void hub_network_manager_refresh_active_interface(void)
{
    hub_network_interface_t previous = s_network_status.active_interface;

    s_network_status.active_interface = hub_network_manager_select_active_interface();
    if (previous != s_network_status.active_interface) {
        ESP_LOGI(TAG,
                 "active network: %s -> %s",
                 hub_network_manager_interface_name(previous),
                 hub_network_manager_interface_name(s_network_status.active_interface));
    }
}

static void hub_network_manager_ensure_initialized(void)
{
    if (!s_network_status.initialized) {
        (void)hub_network_manager_init();
    }
}

esp_err_t hub_network_manager_init(void)
{
    memset(&s_network_status, 0, sizeof(s_network_status));
    s_network_status.initialized = true;
    s_network_status.active_interface = HUB_NETWORK_INTERFACE_NONE;
    ESP_LOGI(TAG, "network manager ready");
    return ESP_OK;
}

void hub_network_manager_set_eth_status(bool link_up, bool has_ip)
{
    hub_network_manager_ensure_initialized();

    bool changed = (s_network_status.eth_link_up != link_up) ||
                   (s_network_status.eth_has_ip != (link_up && has_ip));

    s_network_status.eth_link_up = link_up;
    s_network_status.eth_has_ip = link_up && has_ip;
    hub_network_manager_refresh_active_interface();
    if (changed) {
        hub_network_manager_log_status();
    }
}

void hub_network_manager_set_lte_status(bool connected, const char *ip_addr)
{
    hub_network_manager_ensure_initialized();

    const char *new_ip_addr = (connected && (ip_addr != NULL)) ? ip_addr : "";

    bool changed = (s_network_status.lte_connected != connected) ||
                   (strcmp(s_network_status.lte_ip_addr, new_ip_addr) != 0);

    s_network_status.lte_connected = connected;
    hub_network_manager_copy_string(s_network_status.lte_ip_addr,
                                    sizeof(s_network_status.lte_ip_addr),
                                    new_ip_addr);
    hub_network_manager_refresh_active_interface();
    if (changed) {
        hub_network_manager_log_status();
    }
}

int hub_network_manager_get_active_interface(void)
{
    hub_network_manager_ensure_initialized();
    return (int)s_network_status.active_interface;
}

bool hub_network_manager_has_usable_network(void)
{
    hub_network_manager_ensure_initialized();
    return s_network_status.active_interface != HUB_NETWORK_INTERFACE_NONE;
}

esp_err_t hub_network_manager_get_status(hub_network_manager_status_t *status)
{
    if (status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    hub_network_manager_ensure_initialized();
    *status = s_network_status;
    return ESP_OK;
}

const char *hub_network_manager_interface_name(hub_network_interface_t interface)
{
    switch (interface) {
    case HUB_NETWORK_INTERFACE_NONE:
        return "none";
    case HUB_NETWORK_INTERFACE_ETHERNET:
        return "ethernet";
    case HUB_NETWORK_INTERFACE_LTE:
        return "lte";
    default:
        return "unknown";
    }
}
