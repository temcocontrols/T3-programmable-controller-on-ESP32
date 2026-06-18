# T3 ESP32 Controller - MQTT Application & Implementation Guide

This document provides a technical overview of the MQTT client implementation on the **T3 ESP32 Programmable Controller** (ESP32-S3 platform). It outlines the architecture, initialization lifecycle, and details the two primary Change-of-Value (COV) paradigms supported by the system.

---

## 1. Architectural Overview

The MQTT integration is located in the [Mqtt_Handler](file:///S:/Shubham/Shubham_Files/TemcoControl/TemcoControl_Tstate_11/T3-programmable-controller-on-ESP32/main/Mqtt_Handler) folder, consisting of:
- [Mqtt_Handler.h](file:///S:/Shubham/Shubham_Files/TemcoControl/TemcoControl_Tstate_11/T3-programmable-controller-on-ESP32/main/Mqtt_Handler/Mqtt_Handler.h): Public APIs, constants, and preprocessor configuration switches.
- [Mqtt_Handler.c](file:///S:/Shubham/Shubham_Files/TemcoControl/TemcoControl_Tstate_11/T3-programmable-controller-on-ESP32/main/Mqtt_Handler/Mqtt_Handler.c): Task initialization, event loop handling, payload serialization (JSON), and polling/filtering routines.

### Key Dependencies
*   **ESP-IDF v5 MQTT Client (`esp-mqtt`)**: Handles transport, TCP connection, TLS (optional), handshakes, and packet retries.
*   **cJSON**: Used for dynamic serialization of BACnet COV data into JSON strings.
*   **BACnet Protocol Stack**: Integrates with BACnet object types, properties, and values.

---

## 2. Initialization & Lifecycle Management

The MQTT client lifecycle is tightly coupled with the controller's main task scheduler and Modbus registers.

```mermaid
flowchart TD
    A[Modbus Register 24 Written] --> B{Value == 1?}
    B -- Yes --> C[Mqtt_Handler_Init]
    B -- No --> D[Mqtt_Deinit]
    C --> E{Task already running?}
    E -- No --> F[xTaskCreate Mqtt_HandlerTask]
    F --> G[Wait for WIFI_CONNECTED_BIT]
    G --> H[esp_mqtt_client_init & Start]
    H --> I[Connect to broker.hivemq.com]
    D --> J[Set mqtt_task_exit = true]
    J --> K[esp_mqtt_client_stop & Destroy]
    K --> L[Delete Task]
```

### A. Task Spawning (`Mqtt_Handler_Init`)
The function `Mqtt_Handler_Init()` is called during system startup or when a write operation occurs on Modbus Register 24 (`MODBUS_ENABLE_MQTT`):
```c
void Mqtt_Handler_Init(void)
{
    if(Modbus.enable_mqtt)
    {
        if (main_task_handle[19] != NULL) return; // Prevent duplicate tasks
        mqtt_task_exit = false;
        xTaskCreate(Mqtt_HandlerTask, "mqtt_handler", 4096, NULL, tskIDLE_PRIORITY + 2, &main_task_handle[19]);
    }
}
```

### B. Task De-initialization (`Mqtt_Deinit`)
When Modbus Register 24 is written with `0`, `Mqtt_Deinit()` sets `mqtt_task_exit = true`. The task loops terminates, unregisters event handlers, stops the MQTT client, destroys the client handle, and deletes itself.

---

## 3. The Event Loop and Connection Handshake

Inside `Mqtt_HandlerTask`, execution is blocked until Wi-Fi reports connection status:
1.  **Wi-Fi Check:** Blocks on `s_wifi_event_group` until `WIFI_CONNECTED_BIT` is set.
2.  **Configuration:** Configures the client with standard unencrypted TCP:
    ```c
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = "mqtt://broker.hivemq.com:1883",
        },
    };
    ```
3.  **Event Registration:** Registers `mqtt_event_handler` for connection updates, publishing failures, subscriptions, and TCP/TLS socket transport diagnostics.
4.  **Handshake Pub/Sub:**
    *   **Subscribes to:** `temco/test/tstat11/sub` (QoS 1)
    *   **Publishes to:** `temco/test/tstat11/pub` with connection payload `{"status":"online","device":"tstat11","broker":"HiveMQ"}`.

---

## 4. Change-of-Value (COV) Execution Paradigms

The codebase supports two distinct mechanisms for detecting and dispatching data changes over MQTT. These are configured via macros in [Mqtt_Handler.h](file:///S:/Shubham/Shubham_Files/TemcoControl/TemcoControl_Tstate_11/T3-programmable-controller-on-ESP32/main/Mqtt_Handler/Mqtt_Handler.h).

---

### Mechanism A: "All COV" Mode (`ALL_COV`)
*Note: This mode has been retired and disabled to optimize CPU cycles and minimize RAM usage by removing large global backup arrays (which would occupy over 1.5KB of memory).*

---

### Mechanism B: BACnet Subscription COV Mode (`BACNET_SUB_COV = 1`)

Rather than scanning all points, this mode follows the event-driven BACnet standard, where only points with active BACnet subscriptions are published.

#### 1. Execution Path
When `#define BACNET_SUB_COV 1`, the MQTT engine intercepts changes triggered during standard BACnet operations inside [app_main.c](file:///S:/Shubham/Shubham_Files/TemcoControl/TemcoControl_Tstate_11/T3-programmable-controller-on-ESP32/main/app_main.c):
1.  **Received COV Event (`Update_COV_Notify`):**
    Intercepted at line 2428 when a remote BACnet device notifies a change.
2.  **Internal State Change (`Update_Value_List`):**
    Intercepted at line 2522 when local points update, packaging the point's type, instance, value, and lifetime before calling the MQTT publish function.

```c
#if BACNET_SUB_COV
    extern uint32_t Instance;
    cov_data.monitoredObjectIdentifier.type = type;
    cov_data.monitoredObjectIdentifier.instance = original_instance;
    cov_data.initiatingDeviceIdentifier = Instance;
    cov_data.subscriberProcessIdentifier = 1;
    cov_data.timeRemaining = 60;

    Mqtt_Handler_Send_COV(&cov_data);
#endif
```

#### 2. Key Benefits
*   **Bandwidth Efficiency:** Drastically reduces network traffic since unsubscribed points do not generate MQTT payloads.
*   **On-Demand Processing:** CPU resources are only spent on serialization and publishing when a client actively monitors the point.

---

### Mechanism C: Standalone MQTT Subscription Mode (With NVS Flash Persistence)

This mode allows clients on the MQTT broker to explicitly subscribe to any of the 6 point types (AI, AO, AV, BI, BO, BV) using JSON messages. The MQTT handler manages these subscriptions completely standalone in `Mqtt_Handler.c`, separate from the BACnet stack.

#### 1. Unified Subscription Table
Active subscriptions are tracked in a unified structure array of size 20:
```c
typedef struct {
    uint8_t object_type;     // 0 to 5 (matches BACnet types)
    uint16_t instance;       // 1-based instance
    uint32_t lifetime;       // Remaining lifetime in seconds
    int32_t last_value;      // Last value for change detection
    bool is_active;          // Is slot active
    bool persist;            // Should persist in NVS
} MQTT_Subscription;
```
This stores the `last_value` directly inside the subscription structure, meaning we only allocate comparison variables for *active* subscriptions, using only 240 bytes of memory in total.

#### 2. NVS Flash Persistence
To prevent flash wear from short-lived subscriptions, the controller only persists subscriptions whose initial `lifetime` is greater than 30 minutes (defined by `MQTT_FLASH_PERSIST_MIN_LIFETIME` macro). 
*   **Persistent Subscriptions:** Written to ESP-IDF Non-Volatile Storage (NVS) under the `"storage"` namespace with the key `"mqtt_subs"` whenever they are added, updated, or expired.
*   **Transient Subscriptions:** Kept only in RAM and are not written to NVS.
*   **Task Startup:** The controller loads persistent subscriptions from NVS via `mqtt_load_subscriptions_from_flash()`.
*   **MQTT Connection:** The client automatically publishes the latest values of all active loaded subscriptions via `mqtt_publish_active_subscriptions()`.

#### 3. Change Detection & Standalone Loop
Inside `Mqtt_HandlerTask`, the background loop executes `mqtt_update_standalone_subscriptions(1)` every **1000ms**:
*   **Lifetime Decrement:** It decrements the lifetime of active subscriptions. When a subscription reaches 0, it is deactivated and NVS is updated.
*   **Change Detection:** It reads the latest value of each point using `mqtt_get_point_value()`. If a value change exceeds the deadband (0.5 for AI ranges <= 49/57, 10.0 for AI range 58, 5.0 default for AO/AV, and exact match for binary points), it updates `last_value` and publishes a mock `BACNET_COV_DATA` via `Mqtt_Handler_Send_COV`.

---

## 5. JSON Serialization and Type Mapping

`Mqtt_Handler_Send_COV` translates structured C structures into a nested JSON schema using cJSON.

### Data Type Translation Matrix
The BACnet application tag type is evaluated and converted into native JSON types:

| BACnet Tag Constants | C Data Type | JSON Output Representation |
| :--- | :--- | :--- |
| `BACNET_APPLICATION_TAG_NULL` | N/A | `null` |
| `BACNET_APPLICATION_TAG_BOOLEAN` | `bool` | `true` or `false` |
| `BACNET_APPLICATION_TAG_UNSIGNED_INT` | `uint32_t` | Number |
| `BACNET_APPLICATION_TAG_SIGNED_INT` | `int32_t` | Number |
| `BACNET_APPLICATION_TAG_REAL` | `float` | Number (Float) |
| `BACNET_APPLICATION_TAG_DOUBLE` | `double` | Number (Double) |
| `BACNET_APPLICATION_TAG_ENUMERATED` | `uint32_t` | Number |
| `BACNET_APPLICATION_TAG_CHARACTER_STRING` | `char[]` | String |
| `BACNET_APPLICATION_TAG_OBJECT_ID` | `BACNET_OBJECT_ID` | Object: `{"type": <num>, "instance": <num>}` |
| *Others (Unsupported)* | N/A | String: `"(unsupported_tag)"` |

### Topic Generation Logic
The topic is constructed dynamically using the mapped `display_instance` to prevent namespaces from colliding on public brokers:
```c
char topic[128];
snprintf(topic, sizeof(topic), "temco/cov/tstat11/device_%lu/%s_%lu",
         cov_data->initiatingDeviceIdentifier,
         bactext_object_type_name(cov_data->monitoredObjectIdentifier.type),
         display_instance);
```
*   **Resulting topic:** `temco/cov/tstat11/device_<device_id>/<object_type_name>_<display_instance>`

---

## 6. Debugging and Diagnostics

To inspect the MQTT client operations and troubleshoot point subscription flows, the component contains built-in debug logging.

### A. Enabling Debug Prints
Set the preprocessor flag `MQTT_DEBUG_EN` to `1` at the top of [Mqtt_Handler.c](file:///S:/Shubham/Shubham_Files/TemcoControl/TemcoControl_Tstate_11/T3-programmable-controller-on-ESP32/main/Mqtt_Handler/Mqtt_Handler.c):
```c
#define MQTT_DEBUG_EN 1
```

### B. Debug Log Structure
When enabled, debug logs are output via ESP-IDF's logging system under the tag `MQTT_HANDLER` prefixed with `[DEBUG]`. The logs cover:
1.  **Command Reception:** Logs the exact incoming topic and payload.
2.  **Subscription Parsing:** Logs the action, object type, instance, and lifetime decoded from cJSON, including bounds checks and slot allocations.
3.  **Point Comparison:** Prints the actual value, cached backup value, absolute difference, deadband threshold, and subscription force flag status for every checked point once per second:
    `Checking AI1: val=23500, backup=0, diff=23500, err=500, force=1`
4.  **Dispatch Outcomes:** Reports success or failure for the MQTT publish (`Mqtt_Handler_Send_COV`) and the BACnet UDP broadcast (`Send_UCOV_Notify`).
