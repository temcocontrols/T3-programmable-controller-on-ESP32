# Hub Protocols — WireGuard, MQTT, SNMP

**Branch:** `modem-tstat11-merge`  
**Product:** Hub (`PROJECT_HUB = 30`)  
**Decision:** Keep `PROJECT_WIREGUARD_GATEWAY == PROJECT_HUB` (same ID 30). WireGuard is a Hub feature, not a separate product type.

**Goal:** On Hub hardware, support **WireGuard**, **MQTT**, and **SNMP**, with underlay WAN selectable for WireGuard as **GSM (LTE/PPPoS)** or **WiFi**, configurable from **T3000** (Modbus and/or BACnet), alongside existing WireGuard config registers.

---

## Overall status

| Protocol | Required on Hub | Current status | Phase |
|----------|-----------------|----------------|-------|
| **WireGuard** | Yes — tunnel over GSM or WiFi | **In progress** — AUTO underlay (WiFi or GSM) implemented; T3000 underlay selector deferred | **1 — Active** |
| **MQTT** | Yes | Present — WiFi-gated; HiveMQ COV path | **2 — Next** |
| **SNMP** | Yes | **Not present** in this firmware tree | **3 — Later** |

Work order: finish and verify **WireGuard** first, then MQTT, then SNMP.

---

## Shared Hub networking context

Protocols that need outbound IP (WireGuard peer handshake, MQTT broker, SNMP traps/queries) depend on a usable WAN.

| Underlay | Driver / path | Hub status today |
|----------|---------------|------------------|
| WiFi STA | `wifi.c` | Available; WireGuard & MQTT currently wait on `WIFI_CONNECTED_BIT` only |
| LTE / GSM | A7608 + `hub_lte_pppos` | Production PPPoS path exists; IP reported to `hub_network_manager` |
| Ethernet W5500 | `ethernet_task` / `hub_w5500` | Init path exists; status reported to network manager |

**Gap:** `hub_network_manager` tracks eth/LTE status and policy, but does **not** yet:

- Apply `esp_netif_set_default_netif()` when active WAN changes  
- Include **WiFi** as a selectable Hub WAN  
- Expose a T3000-writable “WireGuard underlay” preference (GSM vs WiFi)

---

# Phase 1 — WireGuard (start here)

## 1.1 Requirement (target)

1. WireGuard runs on Hub product type **30** (same ID as Hub — intentional).  
2. Tunnel underlay is **configurable**: **GSM** or **WiFi**.  
3. Configuration from **T3000** via existing WireGuard Modbus map, plus a new underlay/transport selector (Modbus first; BACnet if/when T3000 UI needs it).  
4. After peer is up, WireGuard becomes default route for tunneled app traffic (`esp_wireguard_set_default`).  
5. MQTT/SNMP later can ride Hub WAN and/or WireGuard tunnel once routing is correct — out of scope for Phase 1 verification except noting dependency.

## 1.2 What already exists

| Item | Location | Notes |
|------|----------|--------|
| Product ID alias | `define.h` | `PROJECT_WIREGUARD_GATEWAY PROJECT_HUB` → both **30** |
| Task start | `app_main.c` | Starts `wireguard_gateway_task` when `mini_type == PROJECT_WIREGUARD_GATEWAY` |
| Config struct + NVS | `user_data.h`, `flash.c` | enable, keys, local IP, peer IP, port |
| Modbus map | `modbus.h` **2100–2250** | Read/write via `wireguard_read/write_by_block` |
| Tunnel bring-up | `WireGuard_App.c` | init → connect → peer up → ping → `set_default` |
| SNTP for Hub/WG | `sntp_app.c` | Forces NTP when WireGuard gateway type |

### Existing Modbus WireGuard map (T3000 today)

| Register | Meaning |
|----------|---------|
| 2100 | Enable / disable |
| 2101–2133 | Private key |
| 2134–2166 | Peer public key |
| 2167–2199 | Preshared key |
| 2200–2203 | Local tunnel IP |
| 2204 | Port (local + peer) |
| 2205–2208 | Peer endpoint IP |
| …–2250 | Reserved end |

**Not in map today:** underlay / transport select (GSM vs WiFi), status (peer up, underlay used), BACnet objects for WireGuard.

## 1.3 Current behavior (as implemented)

```
app_main (mini_type == 30)
  → wireguard_gateway_task
       → wait AUTO underlay (WiFi OR GSM/LTE PPP IP)   ← implemented
       → wait SNTP done
       → esp_wireguard_init / connect
       → wait peer up
       → ping
       → esp_wireguard_set_default                    ← WG becomes default route
```

Hub WAN default route for eth/LTE is applied by `hub_network_manager_apply_active_route()` when status/netif updates (needed so GSM underlay can carry the WireGuard handshake).

**T3000 underlay selector:** deferred. AUTO only for now.

**Later (when T3000 config is added):**

```
       → read underlay preference (T3000 / NVS)
       → wait selected underlay ready
            • WIFI  → WIFI_CONNECTED_BIT
            • GSM   → LTE PPP IP via hub_network_manager
            • AUTO  → first ready (current behavior)
```
## 1.4 Gaps to close (WireGuard)

| # | Gap | Why it matters | Status / planned fix |
|---|-----|----------------|----------------------|
| W1 | Underlay wait is WiFi-only | GSM Hub never reaches WG setup | **Done** — AUTO waits WiFi or GSM |
| W2 | No Modbus/BACnet underlay selector | T3000 cannot choose GSM vs WiFi | **Deferred** — AUTO only for now |
| W3 | No WAN default-route apply for eth/LTE | Handshake UDP may leave wrong iface | **Done** — `apply_active_route` + netif setters restored |
| W4 | WiFi not in Hub network manager | AUTO / WiFi underlay not first-class | Open — WireGuard checks WiFi event group directly |
| W5 | No runtime status to T3000 | Hard to debug from UI | Optional later |
| W6 | BACnet WireGuard objects | Optional if Modbus enough for T3000 | Defer until T3000 BACnet UI requirement confirmed |

## 1.5 Proposed T3000 config (WireGuard underlay)

**Modbus (preferred first step)** — pick unused register in WireGuard block, e.g.:

| Register (proposal) | Name | Values |
|---------------------|------|--------|
| **2209** | `MODBUS_WIREGUARD_UNDERLAY` | `0` = Auto, `1` = WiFi, `2` = GSM/LTE |
| **2210** (optional) | `MODBUS_WIREGUARD_STATUS` | bitflags / enum: disabled, waiting underlay, connecting, peer_up, error |
| **2211** (optional) | `MODBUS_WIREGUARD_ACTIVE_UNDERLAY` | read-only: which underlay is active |

Persist with other WireGuard NVS keys in `flash.c`.

**BACnet:** add only if T3000 needs object-based config; otherwise keep Modbus-only for Phase 1.

## 1.6 Verification checklist (WireGuard — Phase 1)

Use this as the pass/fail list before moving to MQTT.

### A. Product / enable

- [ ] Hub boots with `mini_type = 30` and WireGuard task starts (expected with shared ID).  
- [ ] With `wireguard_enable = 0`, tunnel does not become default route.  
- [ ] With enable = 1 and valid keys/IPs from T3000/Modbus, config loads from NVS after reboot.

### B. Underlay = WiFi (AUTO)

- [ ] Only WiFi up (or WiFi ready first).  
- [ ] Log: `AUTO underlay ready (wifi)`.  
- [ ] SNTP completes.  
- [ ] Peer reaches up; log shows WireGuard default set.  
- [ ] Ping through tunnel succeeds (existing 10.0.0.1 path or peer).  

### C. Underlay = GSM (AUTO)

- [ ] Only GSM/LTE PPP up (or GSM ready first; WiFi not connected).  
- [ ] Log: `AUTO underlay ready (gsm)`.  
- [ ] WireGuard does **not** block forever on WiFi.  
- [ ] Peer handshake succeeds over LTE.  
- [ ] `set_default` succeeds; tunnel traffic works.

### D. T3000 / Modbus underlay selector

- [ ] **Deferred** — not required for current AUTO implementation.  
- [ ] Existing key/IP/port registers still work.  

### E. Routing sanity

- [ ] Before WG up: default route is underlay (WiFi stack or `[HUB][WAN] active interface=lte`).  
- [ ] After WG up: default route is WireGuard iface.  
- [ ] Losing underlay / reconnect does not brick eth/WiFi/LTE permanently.

## 1.7 Implementation order (WireGuard only)

1. ~~Document + agree Modbus underlay register (2209 proposal).~~ **Skipped for now — AUTO only.**  
2. ~~Restore/apply Hub WAN default route in `hub_network_manager`.~~ **Done.**  
3. ~~Change `wireguard_gateway_task` to wait on AUTO underlay (WiFi or GSM).~~ **Done.**  
4. NVS save/load for underlay — when T3000 selector is added.  
5. Optional status registers.  
6. Run checklist B/C/E on hardware (WiFi + GSM).  
7. Mark Phase 1 complete → start MQTT.

---

## Decision log

| Date | Decision |
|------|----------|
| 2026-09-14 | Keep `PROJECT_HUB` and `PROJECT_WIREGUARD_GATEWAY` as same ID **30**. |
| 2026-09-14 | WireGuard underlay must be **GSM or WiFi**, configurable from T3000 (Modbus first). |
| 2026-09-14 | Verify protocols in this plan file; implement/verify **WireGuard first**, then MQTT, then SNMP. |
| 2026-09-14 | **T3000 underlay config deferred.** Use **AUTO** (first ready of WiFi or GSM). Implement wait + WAN default route now. |
| 2026-09-14 | Bump `SOFTREV` **6602 → 6909** so T3000 WireGuard UI gate (`main*10+sub >= 699`) passes. |
| 2026-09-14 | Hub WireGuard key buffers **64 → 66** to match T3000 `Str_Wireguard_point` BACnet layout (no T3000 change). |

---

# Phase 2 — MQTT (summary only; do after WireGuard)

## Requirement

MQTT client on Hub for cloud/COV (and related) over usable network — ideally same Hub WAN / WireGuard story once routing is stable.

## Current status

| Item | Status |
|------|--------|
| Handler | `Mqtt_Handler/` present; started from `app_main` via `Mqtt_Handler_Init()` |
| Broker | Default `mqtt://broker.hivemq.com:1883` |
| Enable | Modbus-related control (see `Mqtt_User_Guide.md`) |
| Underlay | **Waits on WiFi only** |
| GSM / WireGuard path | Not verified |

## Planned when Phase 2 starts

- Reuse Hub “usable network” (and optionally “prefer tunnel”) instead of WiFi-only wait.  
- Verify publish/subscribe with underlay WiFi and GSM.  
- Confirm behavior when WireGuard is default route (broker via tunnel vs underlay).  
- Update `Mqtt_User_Guide.md` for Hub.

**Status:** Deferred until Phase 1 checklist passes.

---

# Phase 3 — SNMP (summary only)

## Requirement

SNMP on Hub for monitoring/management from NMS / T3000 ecosystem as needed.

## Current status

| Item | Status |
|------|--------|
| Source / component | **No SNMP implementation found** in this repo (`snmp` / `SNMP` grep empty) |
| Stack choice | TBD (e.g. esp-idf SNMP agent, or Temco existing product port) |
| MIB / objects | TBD with product requirements |
| Transport | UDP/161 (and traps) over Hub WAN / tunnel — depends on Phase 1–2 routing |

## Planned when Phase 3 starts

- Choose stack and MIB scope.  
- Bind to Hub netif after WAN/WG stable.  
- T3000 enable/community/port via Modbus/BACnet.  
- Verification against WiFi and GSM underlays.

**Status:** Not started.

---

## References

- `main/WireGuard_App/WireGuard_App.c`  
- `main/modbus.h` (WireGuard registers 2100–2250)  
- `main/HUB_Module/hub_network_manager.*`  
- `main/HUB_Module/hub_lte_pppos.*`  
- `main/Mqtt_Handler/` + `Mqtt_User_Guide.md`  
- Related modem plan: `Documents/Plan/HUB2_A7608_MODEM_INTEGRATION.md`  

---

## Next action

**Hardware verify Phase 1:** flash and confirm WireGuard proceeds on **WiFi-only** and **GSM-only** (log: `AUTO underlay ready (wifi|gsm)`), then peer up + `set_default`.
