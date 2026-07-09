#include "hub_network_manager.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

static const char *TAG = "hub_net_mgr";

static hub_network_manager_status_t s_network_status;

static bool hub_network_manager_policy_is_valid(hub_network_policy_t policy)
{
    return (policy >= HUB_NET_POLICY_AUTO) && (policy <= HUB_NET_POLICY_IGNORE_ETHERNET);
}

static bool hub_network_manager_policy_allows_ethernet(hub_network_policy_t policy)
{
    return (policy == HUB_NET_POLICY_AUTO) || (policy == HUB_NET_POLICY_FORCE_ETHERNET);
}

static bool hub_network_manager_policy_allows_lte(hub_network_policy_t policy)
{
    return (policy == HUB_NET_POLICY_AUTO) ||
           (policy == HUB_NET_POLICY_FORCE_LTE) ||
           (policy == HUB_NET_POLICY_IGNORE_ETHERNET);
}

static void hub_network_manager_copy_string(char *dest, size_t dest_len, const char *src)
{
    if ((dest == NULL) || (dest_len == 0)) {
        return;
    }

    snprintf(dest, dest_len, "%s", (src != NULL) ? src : "");
}

static hub_network_interface_t hub_network_manager_choose_active_interface(void)
{
    bool ethernet_ready = s_network_status.eth_link_up && s_network_status.eth_has_ip;
    bool lte_ready = s_network_status.lte_connected && (s_network_status.lte_ip_addr[0] != '\0');

    switch (s_network_status.policy) {
    case HUB_NET_POLICY_AUTO:
        if (ethernet_ready) {
            return HUB_NETWORK_INTERFACE_ETHERNET;
        }
        if (lte_ready) {
            return HUB_NETWORK_INTERFACE_LTE;
        }
        break;
    case HUB_NET_POLICY_FORCE_ETHERNET:
        if (ethernet_ready) {
            return HUB_NETWORK_INTERFACE_ETHERNET;
        }
        break;
    case HUB_NET_POLICY_FORCE_LTE:
    case HUB_NET_POLICY_IGNORE_ETHERNET:
        if (lte_ready) {
            return HUB_NETWORK_INTERFACE_LTE;
        }
        break;
    default:
        break;
    }

    return HUB_NETWORK_INTERFACE_NONE;
}

const char *hub_network_manager_policy_name(hub_network_policy_t policy)
{
    switch (policy) {
    case HUB_NET_POLICY_AUTO:
        return "auto";
    case HUB_NET_POLICY_FORCE_ETHERNET:
        return "force_ethernet";
    case HUB_NET_POLICY_FORCE_LTE:
        return "force_lte";
    case HUB_NET_POLICY_IGNORE_ETHERNET:
        return "ignore_ethernet";
    default:
        return "unknown";
    }
}

static void hub_network_manager_log_status(void)
{
    ESP_LOGI(TAG,
             "status: policy=%s eth(link=%d ip=%d allowed=%d) lte(connected=%d ip=%s allowed=%d) active=%s",
             hub_network_manager_policy_name(s_network_status.policy),
             s_network_status.eth_link_up,
             s_network_status.eth_has_ip,
             hub_network_manager_policy_allows_ethernet(s_network_status.policy),
             s_network_status.lte_connected,
             s_network_status.lte_ip_addr[0] != '\0' ? s_network_status.lte_ip_addr : "-",
             hub_network_manager_policy_allows_lte(s_network_status.policy),
             hub_network_manager_interface_name(s_network_status.active_interface));
}

static void hub_network_manager_refresh_active_interface(void)
{
    hub_network_interface_t previous = s_network_status.active_interface;

    s_network_status.active_interface = hub_network_manager_choose_active_interface();
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
    s_network_status.policy = HUB_NET_POLICY_AUTO;
    s_network_status.active_interface = HUB_NETWORK_INTERFACE_NONE;
    ESP_LOGI(TAG, "network manager ready");
    return ESP_OK;
}

esp_err_t hub_network_manager_set_policy(hub_network_policy_t policy)
{
    if (!hub_network_manager_policy_is_valid(policy)) {
        return ESP_ERR_INVALID_ARG;
    }

    hub_network_manager_ensure_initialized();
    if (s_network_status.policy == policy) {
        return ESP_OK;
    }

    hub_network_policy_t previous = s_network_status.policy;
    s_network_status.policy = policy;
    hub_network_manager_refresh_active_interface();
    ESP_LOGI(TAG,
             "network policy: %s -> %s",
             hub_network_manager_policy_name(previous),
             hub_network_manager_policy_name(s_network_status.policy));
    hub_network_manager_log_status();
    return ESP_OK;
}

hub_network_policy_t hub_network_manager_get_policy(void)
{
    hub_network_manager_ensure_initialized();
    return s_network_status.policy;
}

bool hub_network_manager_is_ethernet_allowed(void)
{
    hub_network_manager_ensure_initialized();
    return hub_network_manager_policy_allows_ethernet(s_network_status.policy);
}

bool hub_network_manager_is_lte_allowed(void)
{
    hub_network_manager_ensure_initialized();
    return hub_network_manager_policy_allows_lte(s_network_status.policy);
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

void hub_network_manager_select_active_interface(void)
{
    hub_network_manager_ensure_initialized();
    hub_network_manager_refresh_active_interface();
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

const char *hub_network_manager_interface_name(int interface_id)
{
    switch (interface_id) {
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
