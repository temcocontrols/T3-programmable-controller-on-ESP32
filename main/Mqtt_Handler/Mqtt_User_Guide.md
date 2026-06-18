# T3 ESP32 Controller - MQTT User Guide

This guide provides instructions on how to enable, configure, and monitor MQTT communication on the **T3 ESP32 Programmable Controller**. It also details how to use **MQTTX** to verify connection status and monitor data transmission.

---

## 1. MQTT Overview
The T3 ESP32 controller integrates a lightweight MQTT client that connects to the public **HiveMQ** broker to publish real-time point changes via **Change of Value (COV)** notifications. This allows integration with cloud platforms, SCADA systems, or custom IoT dashboards.

---

## 2. Modbus Configuration Details

To manage the MQTT service on the T3 controller, you must read or write to specific Modbus register values. The configuration details are as follows:

### MQTT Control Register

| Modbus Register (0-Indexed) | Modbus Address (1-Indexed) | Description | Data Type | Permitted Values | Default |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **24** | **25** | Enable/Disable MQTT Service | Unsigned 8-bit | `0` = Disabled<br>`1` = Enabled | `0` (Disabled) |

> [!NOTE]
> Modbus register changes are automatically saved to the controller's non-volatile storage (NVS) flash partition. This ensures the setting persists across device reboots and power cycles.

### Hardcoded Broker Specifications

Currently, the broker information is hardcoded in the device firmware to facilitate plug-and-play testing:

*   **Broker Address/Host:** `broker.hivemq.com`
*   **Port:** `1883` (Standard unencrypted TCP port)
*   **Protocol:** `mqtt://` (Standard MQTT over TCP)
*   **Encryption/Credentials:** None required (Public testing broker)

> [!WARNING]
> Since the HiveMQ public broker (`broker.hivemq.com`) is a shared, unencrypted public server, it should **not** be used for sensitive production deployments.

---

## 3. Network Prerequisites
For the MQTT client to initiate a connection, the following prerequisites must be met:
1.  The controller must have a valid network connection (Ethernet or Wi-Fi).
2.  The controller must obtain an IP address.
3.  Modbus Register 24 must be written with a value of `1`.

On startup, the MQTT handler task will block and print `Waiting for WiFi connection...` until the network connection is established, after which it will start the MQTT client and connect.

---

## 4. Monitoring MQTT Data with MQTTX

[MQTTX](https://mqttx.app/) is a cross-platform MQTT client tool designed for developers to test MQTT connections, publish, and subscribe to topics.

### A. Downloading and Installing MQTTX
1.  Visit the official website: [https://mqttx.app/](https://mqttx.app/).
2.  Click on the **Download** button.
3.  Choose the installer appropriate for your Operating System:
    *   **Windows:** `.exe` installer or portable version.
    *   **macOS:** `.dmg` installer (Intel or Apple Silicon).
    *   **Linux:** `.AppImage` or `.deb` package.
4.  Run the installer and follow the wizard instructions to complete installation.

### B. Configuring a New Connection in MQTTX
1.  Open the MQTTX application.
2.  Click the **+ New Connection** button in the left sidebar or the center of the window.
3.  Configure the connection settings as follows:
    *   **Name:** `T3 ESP32 Controller` (or any descriptive name)
    *   **Client ID:** Leave as default (randomly generated)
    *   **Host:** `mqtt://broker.hivemq.com`
    *   **Port:** `1883`
    *   **Username / Password:** Leave blank (no credentials required)
    *   **SSL/TLS:** Disabled
    *   **MQTT Version:** `3.1.1` (or `5.0`)
4.  Click the **Connect** button in the top right corner. The connection status circle should turn green and show **Connected**.

### C. Subscribing to T3 Controller Topics
Once connected to the broker, you can subscribe to topics to receive messages published by the controller.

1.  Click the **+ New Subscription** button.
2.  Configure the subscription:
    *   **Topic:** `temco/cov/tstat11/#`
    *   **QoS:** `1`
3.  Click **Confirm**.

> [!IMPORTANT]
> **Active BACnet Subscription Requirement:**
> By default, the controller operates in **BACnet Subscription COV** mode. In this mode, the controller will only publish COV messages to MQTT for objects that have an **active BACnet subscription** registered from a BACnet client/workstation (such as Yabe). 
> If no BACnet client is actively subscribed to a point, the controller will **not** publish its updates to the MQTT broker.
> *   To verify and trigger data, connect to the controller using a BACnet tool and subscribe to the Change-of-Value (COV) properties for the objects you want to monitor.
> *   If you want all values to publish automatically without requiring a BACnet subscription, the device firmware configuration must be compiled with `ALL_COV` enabled.

> [!TIP]
> The wildcard `#` in `temco/cov/tstat11/#` allows you to receive notifications for **all** objects on **all** devices. If you only want to monitor a specific device or object type, use a specific topic (see Topic Structure below).

### D. MQTT Topic Structure and Payload

The controller publishes COV events using JSON format.

#### 1. Device Diagnostics & Connection Handshake Topic
Upon a successful connection to the HiveMQ broker, the controller publishes a hello message to confirm its status:
*   **Topic:** `temco/test/tstat11/pub`
*   **Payload Example:**
    ```json
    {
      "status": "online",
      "device": "tstat11",
      "broker": "HiveMQ"
    }
    ```

#### 2. COV Notification Topic
When a point's value changes, the controller publishes to a topic structured by its device instance and object type:
*   **Topic Format:** `temco/cov/tstat11/device_<Device_ID>/<Object_Type>_<Instance>`
    *   `<Device_ID>`: The unique BACnet device instance number (e.g., `12345`).
    *   `<Object_Type>`: The object type name (e.g., `ANALOG_INPUT`, `BINARY_VALUE`, etc.).
    *   `<Instance>`: The 1-based index of the object (e.g., `1` for Input 1).
*   **Example Topic:** `temco/cov/tstat11/device_12345/ANALOG_INPUT_1`
*   **Payload Example:**
    ```json
    {
      "subscriber_process_id": 1,
      "initiating_device_id": 12345,
      "time_remaining": 60,
      "monitored_object": {
        "type": 0,
        "type_name": "ANALOG_INPUT",
        "instance": 1
      },
      "values": [
        {
          "property_id": 85,
          "property_name": "PRESENT_VALUE",
          "property_array_index": 4294967295,
          "priority": 16,
          "tag": 4,
          "value": 23.5
        }
      ]
    }
    ```

#### E. Initiating Point Subscriptions via MQTT
If you do not want to use a BACnet tool to subscribe to points, you can initiate subscriptions directly via MQTT by publishing a subscription control JSON payload.

The controller listens for subscription messages on the following topics:
*   `temco/test/tstat11/sub`
*   `temco/cov/tstat11/sub`

##### 1. JSON Subscription Request Format
Publish a JSON message containing the target point details:
```json
{
  "action": "subscribe",
  "object_type": "ANALOG_INPUT",
  "instance": 1,
  "lifetime": 300
}
```
*   `action`: Must be `"subscribe"`.
*   `object_type`: The BACnet object type. Can be numeric (e.g. `0` for Analog Input) or a string:
    *   `0` or `"ANALOG_INPUT"`
    *   `1` or `"ANALOG_OUTPUT"`
    *   `2` or `"ANALOG_VALUE"`
    *   `3` or `"BINARY_INPUT"`
    *   `4` or `"BINARY_OUTPUT"`
    *   `5` or `"BINARY_VALUE"`
*   `instance`: The 1-based point number (e.g. `1` for Input 1).
*   `lifetime`: (Optional) Remaining time in seconds before subscription expires. Defaults to `300` (5 minutes) if omitted.

##### 2. JSON Unsubscribe Request Format
To cancel a subscription before it expires, publish an unsubscribe message:
```json
{
  "action": "unsubscribe",
  "object_type": "ANALOG_INPUT",
  "instance": 1
}
```

##### 3. Simultaneous Response Behavior (Dual-Network Notifications)
When a point is subscribed via MQTT:
1.  The controller registers the request and monitors the point for changes.
2.  Upon change, the controller publishes the update to the corresponding MQTT topic.
3.  **Simultaneously**, the controller broadcasts an unconfirmed BACnet COV notification (`Send_UCOV_Notify`) to the entire BACnet network.
4.  This means subscribing via MQTT triggers responses on **both** MQTT and BACnet networks without modifying the underlying BACnet code.

---

## 5. Troubleshooting Checklist

*   **No Messages Received in MQTTX:**
    1.  Verify that Modbus Register 24 is set to `1` (use a Modbus poll tool like Modscan/Yabe to confirm).
    2.  Check that the controller is connected to the same network and has internet access (required to reach `broker.hivemq.com`).
    3.  Verify your Wi-Fi or Ethernet settings.
    4.  Verify you are subscribed to the correct topic pattern in MQTTX (`temco/cov/tstat11/#`).
    5.  Verify that a BACnet client (e.g. Yabe) has successfully subscribed to the target objects' COV. In default mode, no messages are published to MQTT unless an active BACnet subscription exists for that point.
*   **Client disconnects or shows TCP errors:**
    1.  The public HiveMQ broker may be experiencing high load or temporary downtime. Try pinging `broker.hivemq.com` from a PC connected to the same network to check connectivity.
