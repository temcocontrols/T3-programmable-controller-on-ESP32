/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_ota.h>

#include <common_macros.h>
#include <matter_controller.h>

#include <app/server/CommissioningWindowManager.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/clusters/on_off_server.h>
#include <app/clusters/level_control.h>

#include "matter_light.h"
#include "wifi.h"

static char matter_light_debug_buf[256];

/* Default values */
#define MATTER_LIGHT_ENDPOINT_ID 1
#define MATTER_LIGHT_ON_OFF_CLUSTER_ID chip::app::Clusters::OnOff::Id
#define MATTER_LIGHT_LEVEL_CONTROL_CLUSTER_ID chip::app::Clusters::LevelControl::Id

/* Global Matter attributes */
static bool matter_light_on_off = false;
static uint8_t matter_light_level = 254;

/**
 * @brief Callback for on_off attribute change
 */
static esp_err_t matter_light_on_off_callback(const ChipDevice &device, const ChipCluster &cluster,
                                              const ChipAttribute &attribute, void *priv_data)
{
    if (!cluster.cluster_id == MATTER_LIGHT_ON_OFF_CLUSTER_ID) {
        return ESP_FAIL;
    }

    if (attribute.attribute_id == chip::app::Clusters::OnOff::Attributes::OnOff::Id) {
        /* Get the value */
        uint8_t *value = (uint8_t *)priv_data;
        matter_light_on_off = *value ? true : false;
        sprintf(matter_light_debug_buf, "Matter Light On/Off: %d", matter_light_on_off);
        debug_info(matter_light_debug_buf);

        /* Call your light control function here */
        matter_light_set_power(matter_light_on_off);
    }

    return ESP_OK;
}

/**
 * @brief Callback for level control attribute change
 */
static esp_err_t matter_light_level_callback(const ChipDevice &device, const ChipCluster &cluster,
                                             const ChipAttribute &attribute, void *priv_data)
{
    if (cluster.cluster_id != MATTER_LIGHT_LEVEL_CONTROL_CLUSTER_ID) {
        return ESP_FAIL;
    }

    if (attribute.attribute_id == chip::app::Clusters::LevelControl::Attributes::CurrentLevel::Id) {
        /* Get the value */
        uint8_t *value = (uint8_t *)priv_data;
        matter_light_level = *value;
        sprintf(matter_light_debug_buf, "Matter Light Level: %d", matter_light_level);
        debug_info(matter_light_debug_buf);

        /* Call your light dimming function here */
        matter_light_set_brightness(matter_light_level);
    }

    return ESP_OK;
}

/**
 * @brief Initialize Matter Light
 */
esp_err_t matter_light_init(void)
{
    debug_info("Initializing Matter Light");

    esp_matter_attr_t *attr = NULL;
    esp_matter_endpoint_t *endpoint = NULL;
    esp_matter_cluster_t *cluster = NULL;

    /* Create endpoint */
    endpoint = esp_matter_endpoint_create(esp_matter_get_device(), MATTER_LIGHT_ENDPOINT_ID,
                                         esp_matter_get_dimmable_light());
    if (!endpoint) {
        debug_info("Failed to create endpoint");
        return ESP_FAIL;
    }

    /* Get on_off cluster */
    cluster = esp_matter_cluster_from_endpoint(endpoint, MATTER_LIGHT_ON_OFF_CLUSTER_ID);
    if (!cluster) {
        debug_info("Failed to get on_off cluster");
        return ESP_FAIL;
    }

    /* Create on_off attribute */
    attr = esp_matter_attribute_create(cluster, chip::app::Clusters::OnOff::Attributes::OnOff::Id,
                                       MATTER_LIGHT_ON_OFF_CLUSTER_ID, esp_matter_uint8(false));
    if (!attr) {
        debug_info("Failed to create on_off attribute");
        return ESP_FAIL;
    }

    /* Register attribute callback */
    esp_matter_attribute_change_callback_add(attr, matter_light_on_off_callback);

    /* Get level control cluster */
    cluster = esp_matter_cluster_from_endpoint(endpoint, MATTER_LIGHT_LEVEL_CONTROL_CLUSTER_ID);
    if (!cluster) {
        debug_info("Failed to get level control cluster");
        return ESP_FAIL;
    }

    /* Create current level attribute */
    attr = esp_matter_attribute_create(cluster, chip::app::Clusters::LevelControl::Attributes::CurrentLevel::Id,
                                       MATTER_LIGHT_LEVEL_CONTROL_CLUSTER_ID, esp_matter_uint8(254));
    if (!attr) {
        debug_info("Failed to create level attribute");
        return ESP_FAIL;
    }

    /* Register attribute callback */
    esp_matter_attribute_change_callback_add(attr, matter_light_level_callback);

    debug_info("Matter Light initialized successfully");
    return ESP_OK;
}

/**
 * @brief Get Matter Light On/Off state
 */
bool matter_light_get_power(void)
{
    return matter_light_on_off;
}

/**
 * @brief Set Matter Light On/Off state - Override this function in your application
 */
void matter_light_set_power(bool power) __attribute__((weak));
void matter_light_set_power(bool power)
{
    sprintf(matter_light_debug_buf, "Default: Setting light power to %d", power);
    debug_info(matter_light_debug_buf);
    matter_light_on_off = power;
}

/**
 * @brief Get Matter Light brightness level
 */
uint8_t matter_light_get_brightness(void)
{
    return matter_light_level;
}

/**
 * @brief Set Matter Light brightness - Override this function in your application
 */
void matter_light_set_brightness(uint8_t level) __attribute__((weak));
void matter_light_set_brightness(uint8_t level)
{
    sprintf(matter_light_debug_buf, "Default: Setting light brightness to %d", level);
    debug_info(matter_light_debug_buf);
    matter_light_level = level;
}

/**
 * @brief Update Matter Light On/Off attribute from application
 */
esp_err_t matter_light_update_power(bool power)
{
    matter_light_on_off = power;
    /* Update Matter attribute - you may need to implement this based on your Matter framework */
    sprintf(matter_light_debug_buf, "Matter Light power updated to: %d", power);
    debug_info(matter_light_debug_buf);
    return ESP_OK;
}

/**
 * @brief Update Matter Light brightness attribute from application
 */
esp_err_t matter_light_update_brightness(uint8_t level)
{
    matter_light_level = level;
    /* Update Matter attribute - you may need to implement this based on your Matter framework */
    sprintf(matter_light_debug_buf, "Matter Light brightness updated to: %d", level);
    debug_info(matter_light_debug_buf);
    return ESP_OK;
}
