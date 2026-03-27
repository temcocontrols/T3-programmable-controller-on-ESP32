# Matter Light Integration Guide

This document describes the Matter light integration added to the TEMCO application.

## Overview

The application now includes Matter light support, allowing control of the light via Matter protocol. The implementation consists of:

- **matter_light.c** - Core Matter light implementation
- **matter_light.h** - Public API for Matter light control
- Integration with AppMain.c for initialization

## Files Changed

### 1. File Rename
- `tcp_server.c` → `AppMain.c` (main application entry point)

### 2. New Files Added
- `matter_light.c` - Matter light cluster implementation
- `matter_light.h` - Matter light public API

### 3. Modified Files
- `main/CMakeLists.txt` - Added matter_light.c to build and esp-matter dependency
- `main/idf_component.yml` - Added esp-matter as a dependency
- `main/AppMain.c` - Added Matter light initialization

## Matter Light API

### Initialization
```c
#include "matter_light.h"

// In app_main():
matter_light_init();
```

### Control Light Power (On/Off)
```c
// Turn light on/off from application code
void my_light_control() {
    // Update Matter attribute from application
    matter_light_update_power(true);  // Turn on
    matter_light_update_power(false); // Turn off
}
```

### Control Light Brightness
```c
// Set brightness level (0-254)
void my_dimmer_control() {
    matter_light_update_brightness(200);  // Set brightness level
}
```

### Override Hardware Control Functions
The Matter light library provides default weak implementations. Override them in your application to control actual hardware:

```c
// In your application code (e.g., io.c or LightSwitch.c)

// Override to control actual LED/light hardware
void matter_light_set_power(bool power) {
    if (power) {
        // Turn on light
        gpio_set_level(LIGHT_GPIO, 1);
        ESP_LOGI(TAG, "Light turned ON");
    } else {
        // Turn off light
        gpio_set_level(LIGHT_GPIO, 0);
        ESP_LOGI(TAG, "Light turned OFF");
    }
}

// Override to control brightness (PWM)
void matter_light_set_brightness(uint8_t level) {
    // Set PWM duty for brightness control
    // Map level (0-254) to PWM duty (0-100%)
    float duty = (float)level / 254.0f * 100.0f;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (uint32_t)duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ESP_LOGI(TAG, "Light brightness set to %d", level);
}
```

## Matter Light Clusters

The Matter light implementation includes:

### On/Off Cluster (0x0006)
- Controls basic on/off state of the light
- Attributes:
  - `OnOff` (0x0000) - Boolean state

### Level Control Cluster (0x0008)
- Controls brightness/dimming
- Attributes:
  - `CurrentLevel` (0x0000) - Brightness level (0-254)

## Integration with Existing Code

The Matter light is integrated with the existing light control code:

1. **LightSwitch.c** - Can be modified to call `matter_light_update_power()`
2. **Light_pwm.c** - Can be modified to call `matter_light_update_brightness()`
3. **io.c** - Can be modified to update Matter when physical buttons are pressed

Example integration in LightSwitch.c:
```c
void LightSwitch_SetPower(bool power) {
    // ... existing hardware control code ...

    // Update Matter attribute
    matter_light_update_power(power);
}
```

## Build Instructions

1. Update ESP-IDF to version 5.5.3 or later:
   ```bash
   idf_component.yml specifies: idf: '>=5.5.3'
   ```

2. Build the project:
   ```bash
   idf.py build
   ```

3. Flash to device:
   ```bash
   idf.py flash
   ```

## Configuration

Matter configuration can be done via menuconfig:
```bash
idf.py menuconfig
```

Look for Matter-related options under:
- " Component config > Matter"

## Commissioning

To commission the Matter device:

1. Device will emit BLE advertisement when in commissioning mode
2. Use Matter controller app (e.g., Matter test app) to commission
3. Default PIN code and discriminator available in logs

## Troubleshooting

### Build Errors
If you get build errors related to Matter, ensure:
1. `idf_component.yml` includes `espressif/esp-matter: '*'`
2. ESP-IDF version is 5.5.3 or higher
3. Run `idf.py fullclean` before rebuilding

### Runtime Issues
1. Check logs for "matter_light" tag to see initialization status
2. Verify hardware connections for GPIO control
3. Ensure Matter thread is enabled if using Thread protocol

## References

- [ESP Matter Documentation](https://github.com/espressif/esp-matter)
- [Matter Specification](https://csa-iot.org/certification/matter-specification-download/)
- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/)
