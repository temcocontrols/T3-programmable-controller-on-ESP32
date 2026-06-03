# HUB2 A7608E-H Modem Integration

**Task tracker:** [HUB2_A7608_MODEM_TODO.html](HUB2_A7608_MODEM_TODO.html)

**Architecture:** [HUB2_A7608_MODEM_BLOCK_DIAGRAM.html](HUB2_A7608_MODEM_BLOCK_DIAGRAM.html)

Source plan: `HUB2_A7608_Modem_Integration_Plan.docx`

---

## Objective

Integrate the **A7608E-H 4G/GNSS modem** into the HUB2 project. The ESP32-S3 will control the modem with AT commands and provide SIM detection, LTE registration, signal status, cellular data connection, GNSS location, and basic remote communication.

The first prototype should focus on stable modem bring-up and status reporting. Full 4G-to-Ethernet routing is a separate high-risk phase because it requires PPP, NAT, and IP forwarding.

---

## Architecture

| Block | Hardware | First target |
|-------|----------|--------------|
| MCU | ESP32-S3 | AT control, status collection, Modbus/BACnet exposure |
| Cellular modem | A7608E-H | LTE registration, IP attach, TCP/MQTT/HTTP, GNSS |
| Ethernet | W5500 | ESP32 Ethernet interface for T3000, Modbus TCP, BACnet/IP |
| Ethernet switch | RTL8309N | Treat as unmanaged switch for the first prototype |
| SIM interface | SIM socket + modem SIM pins | SIM inserted/ready detection and validation |
| RF | LTE main antenna + GNSS antenna | Verify registration, signal quality, and location fix |
| Power | HUB2 modem power rail + switching | Survive idle, LTE registration, and data TX current peaks |

---

## Modem Signal Map

The important modem control pins currently match the LilyGo-Modem-Series ATDebug firmware pin map. AT commands have been verified both with the LilyGo ATDebug image and with the HUB2 firmware AT debug bridge at fixed 115200 baud. The ESP32-S3 USB-to-serial bridge is working.

| Signal | Direction | Purpose | Notes |
|--------|-----------|---------|-------|
| ESP_TX / MODEM_RXD | UART | ESP32-S3 TX to modem RX path | GPIO17; LilyGo-compatible; verified in HUB2 AT bridge |
| ESP_RX / MODEM_TXD | UART | Modem TX to ESP32-S3 RX path | GPIO18; LilyGo-compatible; verified in HUB2 AT bridge |
| PWRKEY | Output | Modem power-on/off key | GPIO15; LilyGo-compatible |
| RESET | Output | Hardware reset | GPIO16; LilyGo-compatible |
| RING | Input | Modem ring / URC wake indication | GPIO6; LilyGo-compatible |
| DTR | Output | Modem sleep / data terminal ready control | GPIO7; LilyGo-compatible |
| FLIGHTMODE | Output | RF disable / airplane mode | Not part of the confirmed important pin set yet |
| STATUS / NETLIGHT | Input | Modem power/network indication | Optional diagnostics; AT registration commands remain authoritative |
| SIM_DET | Input | SIM card presence | If available on schematic or modem module output |
| USB_DP / USB_DM | USB | Debug or modem firmware upgrade | Verify whether production firmware needs it |
| LTE_ANT | RF | Main LTE antenna | Check layout, matching, and connection |
| GNSS_ANT | RF | GNSS antenna | Check active/passive antenna requirements |

---

## Bring-up Rule

Do not start by merging modem logic into the main Temco firmware. First verify hardware with a small AT debug project or LilyGO-style modem example.

Required first checks:

1. Confirm ESP32-S3 flash and boot. **Done with LilyGo-Modem-Series ATDebug firmware.**
2. Measure A7608E-H power rail during power-on, registration, and data transmission.
3. Verify PWRKEY, RESET, RING, and DTR behavior. **Core pin map matches LilyGo-Modem-Series.**
4. Confirm UART `AT` / `OK` communication. **Done with LilyGo ATDebug firmware and HUB2 firmware AT bridge at 115200 baud.**
5. Validate SIM card detection and SIM electrical lines. **`+CPIN: READY` has been observed through the HUB2 AT bridge; physical SIM detect and electrical validation remain open.**
6. Check LTE and GNSS antenna connections. **LTE has registered with CSQ 14; GNSS and antenna margin remain open.**
7. Confirm whether the USB path can support debug or firmware upgrade. **ESP32-S3 USB-to-serial bridge is working; modem USB path still needs production decision.**

Current status: parsed modem status is validated. `a7608_refresh_status()` reports `CONNECTED`, operator `46001`, CSQ/RSSI, and IPv4 `10.205.135.212` in the HUB2 AT debug path. The next step is to validate the newly added GNSS enable/read/disable path on hardware, then run a small TCP/MQTT/HTTP upload test.

---

## A7608 Driver Scope

Create one reusable modem module. AT command handling should stay inside this module instead of being scattered through application code.

Suggested files:

- `main/a7608.c`
- `main/a7608.h`

Current status: the first-pass `a7608.c/.h` driver has been added to the `main` component and registered in `main/CMakeLists.txt`. It provides UART setup, LilyGo-compatible default pins, PWRKEY/RESET/DTR helpers, generic AT command send/wait, status refresh, GNSS enable/read/disable helpers, and a modem state enum. For HUB2 bring-up, `PROJECT_HUB_AT_DEBUG` can start `a7608_at_debug_task()` to provide a transparent USB-to-modem AT bridge.

Public responsibilities:

| Area | Required behavior |
|------|-------------------|
| Power | Power on, power off, reset, flight mode control |
| AT framework | Send command, wait for expected response, timeout, retry |
| SIM | Detect SIM inserted and SIM ready state |
| Network | LTE registration, attach status, operator, signal quality |
| IP | PDP context, cellular IP address, connect/disconnect |
| GNSS | Enable, disable, and read location |
| Diagnostics | Clear logs for field troubleshooting |

First integration call target:

```c
a7608_config_t modem_cfg;
a7608_get_default_config(&modem_cfg);
a7608_init(&modem_cfg);
a7608_refresh_status();
```

Only add that call path in the HUB2 project type after confirming it will not conflict with the existing LoRa or RS485 UART usage.

Suggested initial AT commands:

```text
AT
ATI
CPIN?
CSQ
CREG?
CEREG?
CGATT?
CGDCONT?
CGPADDR
NETOPEN
IPADDR
IPCLOSE
GNSS commands from the A7608E-H AT manual
```

Verified so far in HUB2 firmware: the transparent AT bridge starts at 115200 baud and the modem returns `+CPIN: READY`. The TX/RX mapping must remain ESP_TX/MODEM_RX=GPIO17 and ESP_RX/MODEM_TX=GPIO18. The conservative AT status snapshot has validated A7608E-H identity, SIM READY, CSQ/RSSI, CEREG/CREG registered, CGATT attached, operator `46001`, APN `3GNET`, and IPv4 address `10.205.135.212`. The parsed `a7608_refresh_status()` output is now declared done for SIM/LTE/operator/IP status. The debug task also enables GNSS, reads `AT+CGNSSINFO`, and leaves GNSS enabled for manual fix observation.

Open idle observation: after about 20 minutes, the bridge printed escaped non-AT bytes such as `\x05`, `\x12`, and `\x90`. These are raw non-printable UART bytes, not normal A7608 text URCs. When this appears, immediately send `AT`, `AT+IPR?`, `AT+CSCLK?`, `AT+CPIN?`, and `AT+CSQ` through the bridge. If `AT` still returns `OK`, treat the burst as a noise/filtering/debug-output issue for now; if `AT` no longer responds, investigate modem sleep, UART line integrity, and any UART1 ownership conflict.

---

## Modem State Machine

```text
OFF
  - Modem power disabled or confirmed off.
  - PWRKEY may be asserted to start boot.

BOOTING
  - Power rail and STATUS are monitored.
  - Basic AT response is required before moving on.

AT_READY
  - Modem responds to AT.
  - Query ATI and basic firmware information.

SIM_READY
  - CPIN? reports READY.
  - SIM detect and SIM electrical checks are complete.

REGISTERED
  - CREG? / CEREG? indicates network registration.
  - CSQ and operator name are available.

ATTACHED
  - CGATT? confirms packet service attach.
  - PDP context is configured.

CONNECTED
  - Cellular IP is available.
  - TCP/MQTT/HTTP command path can send data.

ERROR
  - Timeout, no AT response, SIM failure, registration failure, or data failure.
  - Retry, reset, or power-cycle based on error class.
```

---

## Cellular Data Policy

First connect out from HUB2. Do not implement router behavior in the first prototype.

Target for the first prototype:

1. Establish cellular data connection through AT commands.
2. Test TCP client, MQTT, or HTTP upload using modem commands.
3. Send basic HUB2 device status through the cellular link.
4. Reconnect after LTE dropout.
5. Reset or power-cycle the modem if it stops responding.

If HUB2 must later act as a 4G router for Ethernet devices, plan a separate phase for PPP, NAT, and IP forwarding.

---

## Temco Firmware Integration

Integrate only after the standalone driver is stable.

| Integration point | Required result |
|-------------------|-----------------|
| Board config | HUB2 project type and pin definitions |
| Ethernet | W5500 initialized and stable |
| Modem | A7608 driver initialized after power checks |
| System status | Modem status added to the status structure |
| Modbus | Status readable from Modbus registers |
| BACnet | Optional BACnet objects for modem status |
| T3000 | Status visible for support and diagnostics |

Recommended status items:

| Status item | Purpose |
|-------------|---------|
| SIM inserted / ready | Shows whether the SIM card is usable |
| LTE registered | Shows whether the modem is registered on the network |
| CSQ / RSSI | Shows cellular signal quality |
| Operator name | Shows the current mobile carrier |
| Cellular IP address | Shows modem data connection status |
| GNSS latitude / longitude | Shows location when GNSS is enabled |
| Modem uptime | Helps identify modem resets |
| Modem reset count | Helps diagnose instability |
| Last error code | Useful for support and field troubleshooting |
| Ethernet link status | Shows W5500/Ethernet status |
| Battery / input voltage | Helps diagnose power issues |

---

## W5500 And Ethernet Switch Verification

Treat W5500 as the ESP32 Ethernet interface. Treat RTL8309N as an unmanaged hardware switch unless VLAN, port isolation, or per-port status is later required.

1. ESP32 obtains static IP or DHCP through W5500.
2. T3000 can discover the HUB2 board.
3. Modbus TCP works through Ethernet.
4. BACnet/IP discovery works through Ethernet.
5. Multiple Ethernet ports pass traffic normally.
6. LTE modem and Ethernet run at the same time without instability.

---

## Implementation Order

| Phase | Focus | Deliverable |
|-------|-------|-------------|
| Phase 1 | Hardware bring-up | Pin map, power sequence, 115200 AT bridge log, SIM READY log, power test, issue list |
| Phase 2 | A7608 driver | `a7608.c/.h`, AT framework, modem state machine, transparent AT debug task, status refresh validation |
| Phase 3 | Cellular data | LTE data connection, TCP/MQTT/HTTP test, reconnect and recovery |
| Phase 4 | Temco integration | HUB2 config, W5500, modem status, Modbus/BACnet/T3000 visibility |
| Phase 5 | Ethernet verification | W5500, RTL8309N switch behavior, Modbus TCP, BACnet/IP |
| Phase 6 | Stability test | 24 to 72 hour run, weak signal, SIM, antenna, Ethernet, power recovery |

---

## Main Risks

- The A7608E-H has high peak current demand. Weak power design may reset the modem or ESP32-S3.
- USB/battery switching copied from LilyGO-style designs may not be seamless.
- UART AT mode is good for status upload and simple TCP/MQTT/HTTP, but not for a high-speed 4G router.
- True 4G-to-Ethernet routing requires PPP, NAT, and IP forwarding.
- LTE and GNSS performance depends heavily on antenna layout, grounding, and matching.
- Running W5500, modem, Wi-Fi, BACnet, and Modbus together may require careful memory and task-priority management.

---

## Bench Checklist

- ESP32-S3 can be flashed and boots reliably.
- A7608E-H power rail remains stable during registration and data TX.
- `AT` / `OK` works repeatedly after cold boot and reset at 115200 baud.
- `CPIN?` reports READY; SIM removal and reinsertion are detected and recovered.
- LTE registration and cellular IP attach are logged.
- GNSS enable/read/disable works without breaking LTE status.
- W5500 Ethernet, Modbus TCP, and BACnet/IP still work while modem status polling runs.
- Modem reset recovery works when AT commands time out.
- 24 to 72 hour test passes with LTE reconnect and Ethernet cable plug/unplug.

---

## Ship Gate

All **P0** tasks in [HUB2_A7608_MODEM_TODO.html](HUB2_A7608_MODEM_TODO.html) are closed. HUB2 can boot, bring up Ethernet, control A7608E-H through AT commands, report modem status to T3000, survive LTE reconnect/reset events, and pass a 24 to 72 hour stability test.
