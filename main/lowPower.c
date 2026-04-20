#include "esp_sleep.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "bacnet/bacdef.h"
//#include "bacnet/cov.h"
//#include "bacnet/basic/object/ai.h" // Example: for Analog Input object

// FreeRTOS task for periodic light sleep and awake cycle
void light_sleep_task(void *pvParameters)
{
    const int sleep_time_sec = 5; // Sleep duration in seconds
    const int awake_time_sec = 20; // Awake duration in seconds after sleep
    while (1) {
        ESP_LOGI("LOW_POWER", "Preparing to enter light sleep for %d seconds...", sleep_time_sec);
        // Disable all previous wakeup sources to avoid conflicts
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
        // Configure timer wakeup
        esp_sleep_enable_timer_wakeup(sleep_time_sec * 1000000ULL);
        // Enter light sleep
        esp_light_sleep_start();
        ESP_LOGI("LOW_POWER", "Woke up from light sleep, staying awake for %d seconds", awake_time_sec);
        // Remain awake for a period, during which other tasks (e.g. Modbus) can run
        vTaskDelay(pdMS_TO_TICKS(awake_time_sec * 1000));
    }
}

// BACnet COV subscription structure and variable
#define MAX_COV_SUBSCRIPTIONS 8
//COV_SUBSCRIPTION cov_subscriptions[MAX_COV_SUBSCRIPTIONS];
#if 0
// Handle COV subscription request
void handle_cov_subscription(BACNET_COV_DATA *cov_data)
{
    // Find an empty subscription slot
    for (int i = 0; i < MAX_COV_SUBSCRIPTIONS; i++) {
        if (!cov_subscriptions[i].flag.valid) {
            cov_subscriptions[i].subscriberProcessIdentifier = cov_data->subscriberProcessIdentifier;
            cov_subscriptions[i].monitoredObjectIdentifier = cov_data->monitoredObjectIdentifier;
            cov_subscriptions[i].lifetime = cov_data->lifetime;
            cov_subscriptions[i].flag.valid = true;
            // Set other parameters if needed
            break;
        }
    }
}

// Check object value change and send COV notification if needed
void check_and_send_cov(uint32_t object_instance, float new_value)
{
    static float last_value[MAX_COV_SUBSCRIPTIONS] = {0};
    float cov_increment = 0.1f; // Threshold for value change

    for (int i = 0; i < MAX_COV_SUBSCRIPTIONS; i++) {
        if (cov_subscriptions[i].flag.valid &&
            cov_subscriptions[i].monitoredObjectIdentifier.instance == object_instance) {
            if (fabs(new_value - last_value[i]) >= cov_increment) {
                // Send COV notification
                bacnet_send_cov_notification(&cov_subscriptions[i], new_value);
                last_value[i] = new_value;
            }
        }
    }
}

// Send BACnet COV notification (pseudo code, adapt to your BACnet stack)
void bacnet_send_cov_notification(COV_SUBSCRIPTION *sub, float value)
{
    BACNET_COV_NOTIFICATION_DATA cov_data = {0};
    cov_data.subscriberProcessIdentifier = sub->subscriberProcessIdentifier;
    cov_data.initiatingDeviceIdentifier = ...; // Your device ID
    cov_data.monitoredObjectIdentifier = sub->monitoredObjectIdentifier;
    cov_data.timeRemaining = sub->lifetime;
    cov_data.listOfValues[0].propertyIdentifier = PROP_PRESENT_VALUE;
    cov_data.listOfValues[0].value = value;
    cov_data.listOfValues_count = 1;

    // Call your BACnet stack's COV notification send function
    cov_notify_send(&cov_data);
}

// Call this function when your object's value is updated
void update_ai_value(uint32_t instance, float new_value)
{
    // ...update your local value...
    check_and_send_cov(instance, new_value);
}

// To use this as a FreeRTOS task, create it in your app_main or initialization code:
// xTaskCreate(light_sleep_task, "LightSleepTask", 2048, NULL, 5, NULL);
#endif
