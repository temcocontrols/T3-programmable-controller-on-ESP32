# T3 Firmware Unit Testing Framework

This folder contains a complete host-based unit testing framework for the ESP32 T3 programmable controller firmware. It compiles **all production C source files** (from `main/`, `temco_bacnet/`, and `temco_IO_control/`) using a standard GCC toolchain without requiring target ESP32 hardware or physical peripherals.

The framework achieves this by using a suite of mocks under `mocks/` that stub out:
- **ESP-IDF Peripheral Drivers & APIs** (Wi-Fi, Ethernet, Flash/NVS, GPIO, I2C, SPI, SNTP, OTA)
- **FreeRTOS Operating System Interfaces** (Tasks, queues, semaphores, mutexes, event groups)
- **LwIP Network Sockets** (TCP, UDP, sockets)
- **LVGL UI Components** (Objects, styles, table/button widgets, fonts, screens)

---

## 1. Verified APIs Summary

The expanded test suite verifies **729 production APIs** (175 directly invoked and 554 transitively invoked) across different modules.

### Active Test Suites Summary

| Test Suite | Source File | Description |
| :--- | :--- | :--- |
| **IO Control Tests** | [test_io_control.c](Unit_Test/test_io_control.c) | Scales raw input/output voltages and verifies sensor curves. |
| **MQTT Handler Tests** | [test_mqtt.c](Unit_Test/test_mqtt.c) | Formats change-of-value (COV) events into JSON payloads and handles MQTT publication logic. |
| **Modbus Protocol Tests** | [test_modbus.c](Unit_Test/test_modbus.c) | Verifies standard Modbus 16-bit CRC calculations for transmission packets. |
| **Sensor Utility Tests** | [test_sensors.c](Unit_Test/test_sensors.c) | Computes and validates Sensirion-specific 8-bit CRCs for I2C data transfers. |
| **RTC & Time Conversion Tests** | [test_rtc_time.c](Unit_Test/test_rtc_time.c) | Converts calendar time structures to calendar/epoch time (`time_t`). |
| **BACnet Codecs Tests** | [test_bacnet_codecs.c](Unit_Test/test_bacnet_codecs.c) | Verifies tag encoders/decoders, primitive codecs, string codecs, datetime codecs, and packing. |
| **Math & Driver Tests** | [test_math_drivers.c](Unit_Test/test_math_drivers.c) | Tests infrared temperature sensor calculations, INA228 scaling, and general voltage conversions. |
| **BACnet Object Property Tests** | [test_bacnet_objects.c](Unit_Test/test_bacnet_objects.c) | Verifies valid instances, object name access, and APDU encoding for AI, AO, AV, BI, BO, BV, and Device. |
| **System Logic & Local DB Tests** | [test_system_data.c](Unit_Test/test_system_data.c) | Verifies insertion, lookup, reading, and writing to local, remote, and network points databases. |
| **Automatically Generated Verification** | [test_extra_apis.c](Unit_Test/test_extra_apis.c) | Scaled testing verifying other core firmware logic under host mocks. |

> [!NOTE]
> For the complete, dynamically generated list of all **729 verified APIs** grouped by file, refer to the [Verified APIs Report](Unit_Test/verified_apis.md).

---

## 2. Local Testing (Windows Host with MSYS2/GCC)

To run the unit tests locally on Windows:

1. **Ensure MSYS2/GCC is installed**: Install MSYS2 (UCRT64 or MinGW64 toolchain).
2. **Open PowerShell** and navigate to the `Unit_Test` directory:
   ```
3. **Configure Path & Execute**:
   ```powershell
   # Add MSYS2 compiler binaries to current terminal environment path
   $env:PATH = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;" + $env:PATH

   # Clean and build
   make clean
   make
   ```

The test runner will compile all sources, link them into the `run_tests` executable, and automatically run the binary.

---

## 3. CI/CD Integration Strategy

Because all ESP-IDF, FreeRTOS, LwIP, and LVGL libraries are fully mocked locally, the test suite is entirely platform-agnostic and can run on **standard Linux containers** in a CI/CD pipeline (e.g., GitHub Actions, GitLab CI, or Jenkins) without any specialized hardware runner.

### GitHub Actions Workflow Configuration

To automatically build and execute the unit tests on every pull request or branch push, create a file named `.github/workflows/unit_tests.yml` in the root of your project:
