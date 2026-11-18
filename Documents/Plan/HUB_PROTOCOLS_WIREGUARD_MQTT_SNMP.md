# Hub Protocols — WireGuard, MQTT, SNMP

**Branch:** `modem-tstat11-merge` (Hub) · T3000: `bhavik_mqtt_snmp`  
**Product:** Hub (`PROJECT_HUB = 30`)  
**Decision:** Keep `PROJECT_WIREGUARD_GATEWAY == PROJECT_HUB` (same ID 30). WireGuard is a Hub feature, not a separate product type.

**Goal:** On Hub hardware, support **WireGuard**, **MQTT**, and **SNMP**, with underlay WAN selectable for WireGuard as **GSM (LTE/PPPoS)** or **WiFi**, configurable from **T3000** (Modbus and/or BACnet), alongside existing WireGuard config registers.

---

## Overall status

| Protocol | Required on Hub | Current status | Phase |
|----------|-----------------|----------------|-------|
| **WireGuard** | Yes — tunnel over GSM or WiFi | **Done (AUTO)** — waits WiFi or GSM; T3000 underlay selector still deferred | **1 — Mostly complete** |
| **MQTT** | Yes | **Done** — config + WAN AUTO (WiFi/LTE), Modbus/BACnet, T3000 dialog; broker connect verified (HiveMQ) | **2 — Complete (basic)** |
| **SNMP** | Yes | **Done (basic)** — uSNMP agent, system MIBs, Modbus/BACnet, T3000 dialog; `snmpwalk` OK; coldStart trap TX coded | **3 — Complete (basic)** |

Work order progress: WireGuard AUTO → MQTT → SNMP agent/walk → trap TX (receive test still open).

---

## Shared Hub networking context

Protocols that need outbound IP (WireGuard peer handshake, MQTT broker, SNMP traps/queries) depend on a usable WAN.

| Underlay | Driver / path | Hub status today |
|----------|---------------|------------------|
| WiFi STA | `wifi.c` | Available; MQTT/SNMP AUTO wait on WiFi or LTE |
| LTE / GSM | A7608 + `hub_lte_pppos` | Production PPPoS path exists; IP reported to `hub_network_manager` |
| Ethernet W5500 | `ethernet_task` / `hub_w5500` | Init path exists; status reported to network manager |

**Network manager:** tracks eth/LTE/policy; applies default route for Hub WAN. WireGuard still uses AUTO (WiFi event bit or LTE connected). WiFi is not fully first-class inside `hub_network_manager` (W4 still open for WG polish).

---

# Phase 1 — WireGuard

## 1.1 Requirement (target)

1. WireGuard runs on Hub product type **30** (same ID as Hub — intentional).  
2. Tunnel underlay is **configurable**: **GSM** or **WiFi**.  
3. Configuration from **T3000** via existing WireGuard Modbus map, plus a new underlay/transport selector (Modbus first; BACnet if/when T3000 UI needs it).  
4. After peer is up, WireGuard becomes default route for tunneled app traffic (`esp_wireguard_set_default`).  
5. MQTT/SNMP can ride Hub WAN and/or WireGuard tunnel once routing is correct.

## 1.2 What already exists

| Item | Location | Notes |
|------|----------|--------|
| Product ID alias | `define.h` | `PROJECT_WIREGUARD_GATEWAY PROJECT_HUB` → both **30** |
| Task start | `app_main.c` | Starts WireGuard manager; gateway task only if enable=1 |
| Config struct + NVS | `user_data.h`, `flash.c` | enable, keys, local IP, peer IP, port |
| Modbus map | `modbus.h` **2100–2250** | Read/write via `wireguard_read/write_by_block` |
| Tunnel bring-up | `WireGuard_App.c` | init → connect → peer up → ping → `set_default` |
| SNTP for Hub/WG | `sntp_app.c` | NTP for handshake |

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
  → WireGuard manager
       → if enable=0: gateway task not running
       → if enable=1:
            → wait AUTO underlay (WiFi OR GSM/LTE PPP IP)
            → wait SNTP done
            → esp_wireguard_init / connect
            → wait peer up
            → ping
            → esp_wireguard_set_default
```

**T3000 underlay selector:** deferred. AUTO only for now.

## 1.4 Gaps to close (WireGuard)

| # | Gap | Why it matters | Status / planned fix |
|---|-----|----------------|----------------------|
| W1 | Underlay wait is WiFi-only | GSM Hub never reaches WG setup | **Done** — AUTO waits WiFi or GSM |
| W2 | No Modbus/BACnet underlay selector | T3000 cannot choose GSM vs WiFi | **Deferred** — AUTO only for now |
| W3 | No WAN default-route apply for eth/LTE | Handshake UDP may leave wrong iface | **Done** — `apply_active_route` |
| W4 | WiFi not in Hub network manager | AUTO / WiFi underlay not first-class | Open — WG checks WiFi event group directly |
| W5 | No runtime status to T3000 | Hard to debug from UI | Optional later |
| W6 | BACnet WireGuard objects | Optional if Modbus enough for T3000 | Defer until T3000 BACnet UI requirement confirmed |

## 1.5 Proposed T3000 config (WireGuard underlay) — deferred

| Register (proposal) | Name | Values |
|---------------------|------|--------|
| **2209** | `MODBUS_WIREGUARD_UNDERLAY` | `0` = Auto, `1` = WiFi, `2` = GSM/LTE |
| **2210** (optional) | `MODBUS_WIREGUARD_STATUS` | disabled / waiting / connecting / peer_up / error |
| **2211** (optional) | `MODBUS_WIREGUARD_ACTIVE_UNDERLAY` | read-only active underlay |

## 1.6 Verification checklist (WireGuard)

### A. Product / enable

- [ ] Hub boots with `mini_type = 30` and WireGuard manager starts.  
- [ ] With `wireguard_enable = 0`, tunnel does not become default route.  
- [ ] With enable = 1 and valid keys/IPs from T3000/Modbus, config loads from NVS after reboot.

### B. Underlay = WiFi (AUTO)

- [ ] Log: `AUTO underlay ready (wifi)`.  
- [ ] Peer up; WireGuard default set.

### C. Underlay = GSM (AUTO)

- [ ] Log: `AUTO underlay ready (gsm)`.  
- [ ] Peer handshake over LTE; `set_default` OK.

### D. T3000 underlay selector

- [x] **Deferred** — not required for current AUTO implementation.  

### E. Routing sanity

- [ ] Before/after WG up, default route behaves as expected.

## 1.7 Implementation order (WireGuard)

1. ~~Document underlay register.~~ **Skipped — AUTO only.**  
2. ~~WAN default route.~~ **Done.**  
3. ~~AUTO underlay wait.~~ **Done.**  
4. NVS underlay selector — when T3000 UI added.  
5. Hardware verify B/C/E (WiFi + GSM).  

---

## Decision log

| Date | Decision |
|------|----------|
| 2026-09-14 | Keep `PROJECT_HUB` and `PROJECT_WIREGUARD_GATEWAY` as same ID **30**. |
| 2026-09-14 | WireGuard underlay must be **GSM or WiFi**, configurable from T3000 (Modbus first). |
| 2026-09-14 | Verify protocols in this plan file; implement/verify **WireGuard first**, then MQTT, then SNMP. |
| 2026-09-14 | **T3000 underlay config deferred.** Use **AUTO** (first ready of WiFi or GSM). |
| 2026-09-14 | Bump `SOFTREV` **6602 → 6909** for T3000 WireGuard UI gate. |
| 2026-09-14 | Hub WireGuard key buffers **64 → 66** to match T3000 `Str_Wireguard_point`. |
| 2026-10-08 | MQTT + SNMP implemented on Hub + T3000 (`bhavik_mqtt_snmp`); BACnet cmds **46/146** (MQTT), **47/147** (SNMP). |
| 2026-10-08 | SNMP stack = **uSNMP** (`components/snmp_agent`); enterprise OID **P.64991.30**. |
| 2026-10-08 | uSNMP `memcopy` renamed to `usnmp_memcopy` — clash with BACnet `memcopy` broke OID parse / walk. |
| 2026-10-08 | SNMP `sysUpTime` uses `esp_timer` (not `time(NULL)`) to avoid NTP jump. |
| 2026-10-08 | Trap encode buffers moved to static BSS; `hub_snmp` stack increased — coldStart no longer overflows task stack. |

---

# Phase 2 — MQTT

## Requirement

MQTT client on Hub for cloud/COV over usable WAN (WiFi or LTE), configurable from T3000.

## Current status — **basic complete**

| Item | Status |
|------|--------|
| Handler | `Mqtt_Handler/` — WAN wait (WiFi or GSM), then start client |
| Config | `Str_Mqtt` / Modbus **2301–2450**, BACnet **46/146**, NVS |
| T3000 | MQTT dialog + button (below WireGuard) |
| Broker | Verified connect to `mqtt://broker.hivemq.com:1883` (WiFi) |
| TLS / QoS extras | Minimal path; advanced TLS/QoS not a focus |
| GSM path | Code waits LTE; hardware verify with SIM still recommended |
| Via WireGuard tunnel | Not specially verified |

### Modbus MQTT (summary)

| Range | Content |
|-------|---------|
| 2301–2450 | Enable, URI, client ID, user/pass, topics, etc. (`Str_Mqtt`) |

### Verification

- [x] Enable from T3000 / Modbus; NVS persist.  
- [x] AUTO underlay WiFi → MQTT connects and publishes/subscribes (HiveMQ test).  
- [ ] AUTO underlay GSM → MQTT over LTE (needs SIM).  
- [ ] Optional: broker reachability when WireGuard is default route.

**Status:** Phase 2 basic done. Optional polish: GSM verify, WG-as-default routing, TLS.

---

# Phase 3 — SNMP

## Requirement

SNMP agent on Hub for NMS monitoring; config from T3000; traps to configured manager.

## Current status — **basic complete**

| Item | Status |
|------|--------|
| Stack | **uSNMP** in `components/snmp_agent` |
| Wrapper | `main/HUB_Module/hub_snmp.c` — WAN AUTO wait, MIB tree, agent loop |
| Config | `Str_Snmp_point` / Modbus **2451–2550**, BACnet **47/147**, NVS |
| T3000 | SNMP dialog + button (below WireGuard / MQTT) |
| Listen | UDP **161** (configurable) |
| Enterprise | `P.64991.30` → `.1.3.6.1.4.1.64991.30` |
| System MIBs | `B.1.1.0`…`B.1.7.0` + `B.1.4.20` (agent IP as OCTET_STRING) + private stub `P.64991.30.1.0` |
| Walk | **Verified** — full `snmpwalk -v2c -c public <hub> 1.3.6.1.2.1.1` |
| Traps | coldStart TX on agent start if trap dest IP set; port/community from config; stack-safe buffers |
| Trap RX test | **Pending** — confirm with `snmptrapd` / Wireshark on PC |

### Modbus SNMP (summary)

| Registers (approx) | Content |
|--------------------|---------|
| Enable / running / port | Agent control |
| RO / RW community | Strings |
| Trap dest IP + port | coldStart / future traps |
| sysName / sysLocation / sysContact | Writable system strings |

### Bugs fixed during bring-up (2026-10-08)

| Issue | Fix |
|-------|-----|
| Guru Meditation in `mibsetvalue` | NULL guards; `miblistadd` OOM-safe |
| All B.*.*.* looked like duplicates; walk crash | Rename uSNMP `memcopy` → `usnmp_memcopy` (BACnet symbol clash) |
| Walk stopped after sysContact | `B.1.4.20` as **OCTET_STRING** (not ASN.1 IpAddress) |
| sysUpTime ~353 days | Use `esp_timer_get_time()` |
| Stack overflow on coldStart | Static trap scratch + larger `hub_snmp` stack |

### Verification

- [x] Agent starts after WiFi (log: `SNMP agent started … mibs=9`).  
- [x] `snmpwalk` system group returns descr / OID / uptime / contact / IP / name / location / services.  
- [x] Private stub OID registered.  
- [x] coldStart send path implemented (no stack overflow after buffer move).  
- [ ] Receive coldStart on PC (`snmptrapd -f -Lo` or Wireshark `udp.port == 162`) with trap dest = PC IP.  
- [ ] SNMP over LTE underlay.  
- [ ] SET on rw community (sysName/contact/location).  
- [ ] Richer private MIB / more trap types (linkUp, enterprise) as needed.

**Status:** Phase 3 basic agent + walk done. Next: trap receive test, then optional SETs / GSM / richer MIB.

---

## References

- `main/WireGuard_App/WireGuard_App.c`  
- `main/modbus.h` — WG 2100–2250, MQTT 2301–2450, SNMP 2451–2550  
- `main/HUB_Module/hub_network_manager.*`  
- `main/HUB_Module/hub_snmp.*`  
- `main/Mqtt_Handler/`  
- `components/snmp_agent/`  
- Related modem plan: `Documents/Plan/HUB2_A7608_MODEM_INTEGRATION.md`  

---

## Next actions

1. **SNMP trap RX** — set trap dest to PC, rebuild/flash, confirm coldStart on port 162.  
2. **Optional** — SNMP SET test; MQTT/SNMP on GSM with SIM.  
3. **Commit** Hub + T3000 MQTT/SNMP changes when requested (no auto-commit).  
4. **WireGuard** — finish hardware checklist B/C/E; underlay selector remains deferred.
