# BACnet BBMD Implementation Plan

**Project:** T3 Programmable Controller on ESP32  
**Stack:** TEMCO fork of Steve Karg bacnet-stack v0.8.0 (`temco_bacnet`)  
**Platform:** ESP-IDF 5.5.3  
**Document status:** Final plan (consolidates codebase analysis + WireGuard overlap review)  
**Last updated:** June 2026

---

## Table of contents

1. [Executive summary](#1-executive-summary)
2. [Scope and goals](#2-scope-and-goals)
3. [Current architecture](#3-current-architecture)
4. [What exists vs what is missing](#4-what-exists-vs-what-is-missing)
5. [Pre-existing defects (fix first)](#5-pre-existing-defects-fix-first)
6. [Operating modes](#6-operating-modes)
7. [WireGuard overlap and strategy](#7-wireguard-overlap-and-strategy)
8. [Implementation phases](#8-implementation-phases)
9. [File change matrix](#9-file-change-matrix)
10. [Configuration and T3000 integration](#10-configuration-and-t3000-integration)
11. [Testing plan](#11-testing-plan)
12. [Risks and mitigations](#12-risks-and-mitigations)
13. [Effort estimate](#13-effort-estimate)
14. [Open questions](#14-open-questions)
15. [References](#15-references)

---

## 1. Executive summary

This firmware already **compiles Annex J BBMD logic** in `temco_bacnet/src/bvlc.c` (`BBMD_ENABLED=1`). The protocol implementation is largely present. What is missing is **integration with the live UDP path** on the ESP32.

| Finding | Detail |
|---------|--------|
| **Nature of work** | Wiring and hardening — not writing BBMD from scratch |
| **Primary gap** | Receive uses `bip_receive()`; send uses a no-op `bvlc_send_mpdu()` |
| **Config gap** | `Setting_Info.reg.BBMD_EN` exists but is unused (no Modbus, NVS, or runtime sync) |
| **Architecture note** | Two separate BACnet/IP paths (server + client/master) must both be considered |
| **WireGuard note** | No WireGuard code in this repo today; if added elsewhere, scope BBMD as **gateway/interop**, not duplicate remote access |

**Recommended approach:** Fix shared BACnet/IP transport first, enable server-side BBMD for gateway SKUs, defer Foreign Device client work until WireGuard topology is decided, and unify remote-connectivity configuration in T3000.

---

## 2. Scope and goals

### In scope

- Enable BACnet/IP Broadcast Management Device (BBMD) per **ASHRAE Annex J**
- Support **Broadcast Distribution Table (BDT)** and **Foreign Device Table (FDT)**
- Optional **Foreign Device** registration for cross-subnet discovery
- Runtime configuration via flash / Modbus / T3000 (following existing patterns)
- Coexistence with MS/TP, Modbus, and existing network-point scanning

### Out of scope (initial release)

- BACnet Secure Connect / TLS
- IPv6 BBMD
- Full BACnet router (`BAC_ROUTING` is off; `MAX_NUM_DEVICES = 1`)
- Running full BBMD + always-on WireGuard on all SKUs without memory review

### Success criteria

| Scenario | Pass |
|----------|------|
| BBMD on subnet B receives Write-BDT | BVLC-Result `0x0000` |
| Foreign device registers on BBMD | Entry visible in Read-FDT |
| Local Original-Broadcast-NPDU on BBMD | Forwarded-NPDU sent to BDT peers |
| Who-Is across subnets (BBMD or FD path) | I-Am received from remote devices |
| Mode = Off | No regression on same-subnet BACnet |
| MS/TP + IP gateway | No regression on RS485 polling |

---

## 3. Current architecture

### 3.1 High-level stack layout

```
┌─────────────────────────────────────────────────────────────────┐
│  main/tcp_server.c                                              │
│  ┌──────────────┐  ┌────────────────────────┐  ┌─────────────┐ │
│  │  bip_task    │  │ Scan_network_bacnet_   │  │ Bacnet_     │ │
│  │  UDP :47808  │  │ Task (BAC_IP_CLIENT)   │  │ Control     │ │
│  │  (server)    │  │ udp_client_send()      │  │ (1 Hz loop) │ │
│  └──────┬───────┘  └───────────┬────────────┘  └──────┬──────┘ │
└─────────┼──────────────────────┼──────────────────────┼─────────┘
          │                      │                      │
          ▼                      ▼                      ▼
┌─────────────────────────────────────────────────────────────────┐
│  temco_bacnet                                                   │
│  datalink.c ──► bip.c (active)     bvlc.c (compiled, partial)   │
│                 npdu_handler / apdu / device objects            │
└─────────────────────────────────────────────────────────────────┘
```

### 3.2 Dual BACnet/IP paths

The firmware deliberately uses **two paths**, not one datalink:

| Path | Protocol tag | Socket | Send model | Source address |
|------|--------------|--------|------------|----------------|
| **Server** | `BAC_IP` | `bip_sock` (bound to 47808) | Deferred: `bip_send_buf` → `sendto(bip_source_addr)` | **Not set** (commented out in `bip_task`) |
| **Client / master** | `BAC_IP_CLIENT` | `udp_send_sock` (ephemeral, per send) | Immediate `sendto` to `Send_bip_address[]` | Set correctly in `udp_client_send` |

**Server path** — passive device role: Who-Is → I-Am, Read Property, COV, etc.

**Client path** — active master role: network scan, remote point reads, time sync, proprietary panel discovery (`Scan_network_bacnet_Task`).

### 3.3 Datalink dispatch (today)

```c
// temco_bacnet/src/datalink.c
datalink_receive(BAC_IP)      → bip_receive()     // NOT bvlc_receive()
datalink_send_pdu(BAC_IP)     → bip_send_pdu()
datalink_receive(BAC_IP_CLIENT) → bip_receive()
datalink_send_pdu(BAC_IP_CLIENT) → bip_send_pdu_client()
```

Commented-out macros in `datalink.h` show the **intended** BBMD integration (`bvlc_send_pdu` / `bvlc_receive`) was never activated.

### 3.4 Key tasks and priorities

| Task | Priority | UDP port | Role |
|------|----------|----------|------|
| `bip_task` | 1 | 47808 | BACnet/IP server receive/send |
| `Scan_network_bacnet_Task` | idle+1 | 47808 (client) | Network point scan, Who-Is |
| `Bacnet_Control` | 3 | — | 1 s loop — schedules, COV, SNTP |
| `Master0/2_Node_task` | 4 / 8 | RS485 | BACnet MS/TP |

---

## 4. What exists vs what is missing

### Already implemented (compiled)

| Component | Location | Notes |
|-----------|----------|-------|
| Full BBMD/BVLC handlers | `temco_bacnet/src/bvlc.c` | Write/Read BDT, Register/Read/Delete FDT, Forwarded-NPDU, Distribute-Broadcast |
| BDT / FDT tables | `bvlc.c` | Up to 128 entries each — recommend reducing for ESP32 |
| `bvlc_intial()`, `bvlc_maintenance_timer()` | `bvlc.c` | Never called from application |
| Foreign device helpers | `temco_bacnet/src/dlenv.c` | **Not in CMakeLists.txt** |
| `BBMD_ENABLED=1` | `temco_bacnet/include/bacnet.h` | Active when `BIP` is defined |
| Settings field | `Setting_Info.reg.BBMD_EN` | `user_data.h` — unused |

### Not operational

| Gap | Location | Impact |
|-----|----------|--------|
| Wrong receive entry | `datalink.c` | BBMD BVLC function codes ignored |
| `bvlc_send_mpdu()` stub | `bvlc.c:684–713` | All forwarding is a no-op |
| BBMD cases in `bip.c` | `#if 0` block | Dead duplicate — do not revive |
| Outbound broadcast forwarding | `bip.c:698–703` | Commented out (active in `bvlc_receive`) |
| UDP source address (server) | `tcp_server.c:483–484` | FDT / BVLC-Result replies broken |
| `sin` not filled in `bvlc_receive` | `bvlc.c` ~900 | Same source-address bug |
| `dlenv.c` excluded from build | `CMakeLists.txt:43` | No FD registration / TTL renewal |
| No init/maintenance calls | — | FDT entries never expire |

---

## 5. Pre-existing defects (fix first)

These block BBMD and should be treated as **shared BACnet/IP transport** work (also benefits any future WireGuard integration).

### 5.1 Undefined `mtu[MAX_MPDU_IP]` buffer

`bvlc.c` declares `extern uint8_t far mtu[MAX_MPDU_IP]` but the definition in `bip.c` is commented out. All BBMD encode/send helpers use this buffer.

**Action:** Define `bvlc_mtu_buf[MAX_MPDU_IP]` in `bip.c` (prefer dedicated buffer, not aliased to `bip_send_buf`).

### 5.2 `bip_socket()` not implemented

Declared in `bip.h`, called from `bvlc.c`, no function body in the repo.

**Note:** `bip_set_socket(47808)` stores the literal **47808** in `BIP_Socket`, reused as the BACnet **port embedded in MAC addresses** — not the UDP file descriptor. The actual socket is `bip_sock` in `tcp_server.c`.

**Action:** Implement `bip_socket()` returning `bip_sock`. Keep `bip_set_socket(47808)` for port-in-address semantics; add `bip_set_udp_fd(int fd)` if needed.

### 5.3 Server source address not recorded

In `bip_task`, population of `BIP_src_addr` from `recvfrom()` is commented out. Client path in `udp_client_send` does this correctly.

**Action:** Copy IPv4 source from `(struct sockaddr_in *)&bip_source_addr` into `BIP_src_addr[6]`.

### 5.4 `bvlc_receive()` leaves `sin` at zero

Even after switching to `bvlc_receive`, `struct sockaddr_in sin` is never filled from `BIP_src_addr` before the BVLC switch.

**Action:** Add `bip_fill_src_sockaddr(struct sockaddr_in *sin)` and call from receive paths.

### 5.5 `dlenv.c` port byte-order risk

`bbmd_port = 0xBAC0` with `htons((uint16_t) bbmd_port)` may double-swap on ESP32 little-endian. Verify before FD registration testing.

---

## 6. Operating modes

Extend `Setting_Info.reg.BBMD_EN` from a boolean into a **mode enum**:

| Mode | Value | Server (`BAC_IP`) | Client (`BAC_IP_CLIENT`) |
|------|-------|-------------------|--------------------------|
| **Disabled** | 0 | `bip_receive` / `bip_send_pdu` (current behaviour) | Unchanged |
| **BBMD** | 1 | `bvlc_receive`; forward broadcasts via BDT/FDT | No FD behaviour |
| **Foreign Device** | 2 | Handle `BVLC_RESULT` for registration ACK | Broadcasts → `Distribute-Broadcast-To-Network` |

### Runtime gating in `bvlc_receive`

Today, `bvlc_receive` **unconditionally forwards** Original-Broadcast when BDT entries exist. Gate BBMD admin and forwarding on `bbmd_mode == BBMD` only.

### Recommended table sizes (ESP32)

| Constant | Default in stack | Recommended |
|----------|------------------|-------------|
| `MAX_BBMD_ENTRIES` | 128 | 8 |
| `MAX_FD_ENTRIES` | 128 | 16 |

---

## 7. WireGuard overlap and strategy

### 7.1 Status in this repository

**No WireGuard code, dependencies, or configuration** were found in `T3-programmable-controller-on-ESP32` (June 2026 review). Related work may exist in a separate branch or repository (e.g. Bhavik's). This section defines how that work should interact with BBMD.

### 7.2 Same problem, different layer

| | BBMD (Annex J) | WireGuard (VPN) |
|---|----------------|-----------------|
| **Layer** | BACnet BVLC on UDP 47808 | L3 encrypted tunnel |
| **Mechanism** | Forward BACnet broadcasts between subnets | Flat virtual subnet — normal BIP broadcasts work |
| **Scope** | BACnet only | All IP on the tunnel |
| **Typical use** | Multi-subnet BMS sites | Remote access, cloud, site-to-site |

They solve overlapping **product goals** (“see BACnet devices across network boundaries”) but are not interchangeable.

### 7.3 Functional overlap map

```
                    ┌─────────────────────────────────────┐
                    │     Shared infrastructure           │
                    │  • UDP 47808 bind / send / recv     │
                    │  • Netif selection (eth vs VPN)     │
                    │  • Broadcast / discovery policy     │
                    │  • T3000 / Modbus / NVS config      │
                    │  • FreeRTOS tasks + RAM budget      │
                    └──────────────┬──────────────────────┘
                                   │
              ┌────────────────────┴────────────────────┐
              ▼                                         ▼
     ┌─────────────────┐                    ┌─────────────────┐
     │  BBMD plan      │                    │  WireGuard plan │
     │  bvlc BDT/FDT   │                    │  tunnel / peers │
     │  FD registration│                    │  routing / keys │
     └─────────────────┘                    └─────────────────┘
```

| Capability | Risk if both implemented blindly |
|------------|-----------------------------------|
| Cross-subnet Who-Is | Duplicate: VPN reachability **or** BBMD forward |
| Remote T3000 access | Duplicate: VPN **or** Foreign Device to BBMD |
| Peer / gateway IP config | Two confusing “remote IP” fields in T3000 |
| Periodic keepalive | WG handshake **and** FD TTL re-registration |

### 7.4 When BBMD is still needed (even with WireGuard)

- Third-party BMS on another subnet **without** joining the VPN
- Firewalls that block UDP 47808 except at designated BBMD hosts
- BACnet BTL / **B-BBMD** profile requirements
- Mixed-vendor sites where only Temco devices are on VPN

### 7.5 When WireGuard replaces BBMD (for Temco-internal use)

- All BACnet peers on the **same VPN subnet**
- T3000 / network-point scan is **VPN-first**
- No requirement to serve non-VPN BMS on physical LANs

### 7.6 Recommended connectivity models

| Model | Primary | BBMD role |
|-------|---------|-----------|
| **A — VPN-first** | WireGuard mesh or hub | Optional; BMS interop only |
| **B — BACnet-native** | BBMD / Foreign Device | Primary for cross-subnet |
| **C — Hybrid** *(likely for Temco)* | VPN for remote tools; one site gateway runs BBMD | Gateway panel only |

### 7.7 Shared transport API (new)

Introduce a netif-aware send path before deep BBMD or WireGuard-specific code:

```c
typedef enum {
    BACNET_NETIF_ETH  = 0,
    BACNET_NETIF_VPN  = 1,   /* WireGuard, when present */
    BACNET_NETIF_AUTO = 2,
} bacnet_netif_t;

int bip_udp_send_to(bacnet_netif_t ifc,
                    struct sockaddr_in *dest,
                    const uint8_t *data,
                    uint16_t len);
```

Both BBMD (`bvlc_send_mpdu`) and future WireGuard-aware BACnet binding should use this single path.

### 7.8 “Do not duplicate” checklist

| Implement once | Avoid |
|----------------|-------|
| Cross-subnet discovery | VPN flat subnet **or** BBMD — not both as default on every device |
| Remote engineer access | VPN **or** FD-to-BBMD as primary |
| UDP 47808 outbound | One send helper with optional netif |
| T3000 remote config | Single “Remote connectivity” section |

---

## 8. Implementation phases

### Phase 0 — Transport prerequisites *(~1 day)*

**Goal:** Fix latent broken symbols; no BBMD behaviour change yet.

| # | Task | File(s) |
|---|------|---------|
| 0.1 | Define `bvlc_mtu_buf[MAX_MPDU_IP]` | `temco_bacnet/private/bip.c` |
| 0.2 | Implement `bip_socket()` → return `bip_sock` | `bip.c`, `tcp_server.c` |
| 0.3 | Fix `BIP_src_addr` on server receive (IPv4) | `main/tcp_server.c` |
| 0.4 | Add `bip_fill_src_sockaddr()` | `bip.c` or `tcp_server.c` |
| 0.5 | Fix `dlenv.c` port byte order | `temco_bacnet/src/dlenv.c` |

**Exit:** Project links; test `sendto` on port 47808 visible in Wireshark.

---

### Phase 1 — UDP send layer *(~1–2 days)*

**Goal:** Working `bvlc_send_mpdu()`.

| # | Task | File(s) |
|---|------|---------|
| 1.1 | Implement `bip_udp_send_to()` using `sendto(bip_sock, …)` | `main/tcp_server.c` |
| 1.2 | Replace stub in `bvlc_send_mpdu()` | `temco_bacnet/src/bvlc.c` |
| 1.3 | Mutex around `sendto` on shared socket | `tcp_server.c` |
| 1.4 | Expose UDP fd after `socket()`+`bind()` in `bip_task` | `tcp_server.c` |

**Exit:** Manual `bvlc_send_mpdu` test reaches a known host.

---

### Phase 2 — Server BBMD *(~2–3 days)*

**Goal:** Device acts as BBMD on its subnet (gateway SKUs).

| # | Task | File(s) |
|---|------|---------|
| 2.1 | When `bbmd_mode == BBMD`: `datalink_receive(BAC_IP)` → `bvlc_receive` | `datalink.c` |
| 2.2 | Fill `sin` from `BIP_src_addr` in `bvlc_receive` | `bvlc.c` |
| 2.3 | Gate BBMD-only BVLC cases on runtime mode | `bvlc.c` |
| 2.4 | Call `bvlc_intial()` from `Inital_Bacnet_Server()` | `tcp_server.c` |
| 2.5 | Call `bvlc_maintenance_timer(1)` from `Bacnet_Control` (1 Hz) | `tcp_server.c` |
| 2.6 | Remove dead `#if 0` BBMD block | `bip.c` |
| 2.7 | Lab: hardcoded BDT entry (one peer) before T3000 config | test hook |

**Exit:** Write-BDT, Register-FD, Original-Broadcast forwarding verified on two subnets.

---

### Phase 3 — Foreign Device client *(~2–3 days, **defer if VPN-first**)*

**Goal:** Cross-subnet scan and network points via remote BBMD.

| # | Task | File(s) |
|---|------|---------|
| 3.1 | Enable `dlenv.c` in CMakeLists | `temco_bacnet/CMakeLists.txt` |
| 3.2 | Boot registration when `bbmd_mode == FD` | `Inital_Bacnet_Server` / `Bacnet_Control` |
| 3.3 | `dlenv_maintenance_timer(1)` in `Bacnet_Control` | `tcp_server.c` |
| 3.4 | Handle `BVLC_RESULT` on `BAC_IP` server path | `bvlc.c` / `datalink.c` |
| 3.5 | FD broadcast fix: `Distribute-Broadcast-To-Network` in client send | `bip_send_pdu_client` or route via `bvlc_send_pdu` |
| 3.6 | Optional: `#define BBMD_CLIENT_ENABLED 1` for remote BDT tools | `bacnet.h` |

**Defer this phase** until WireGuard topology and T3000 remote-access story are decided.

---

### Phase 4 — Configuration and persistence *(~3–5 days)*

**Goal:** Operator-configurable modes; align with T3000 and WireGuard UX.

Follow the **`mstp_network` precedent**:

| Layer | Precedent | BBMD equivalent |
|-------|-----------|-----------------|
| Runtime | `Modbus.mstp_network` | `Modbus.bbmd_mode`, `bbmd_addr`, `bbmd_port`, `bbmd_ttl` |
| Flash struct | `Setting_Info.reg.mstp_network_number` | Extend `Setting_Info.reg` |
| NVS | `FLASH_MSTP_NETWORK` | `FLASH_BBMD_*` keys |
| Modbus | `MODBUS_MSTP_NETWORK` (reg 47) | New registers — coordinate with T3000 |
| Panel sync | `modbus.c` ~3675 | Add BBMD sync block |

**BDT persistence:**

- **v1:** RAM only (Write-BDT from engineering tool; lost on reboot)
- **v2:** NVS blob for N entries (production BBMD gateways)

**Merge** remote-connectivity UI with WireGuard config — avoid duplicate “remote IP” settings.

---

### Phase 5 — Integration testing *(~3–5 days)*

See [Section 11](#11-testing-plan).

---

### Minimal first PR (recommended)

1. Phase 0 (all items)
2. Phase 1 (UDP send)
3. Phase 2 with hardcoded BDT + `bbmd_mode` test flag
4. Maintenance timer in `Bacnet_Control`

Validates BBMD in the lab before T3000 or Foreign Device work.

---

## 9. File change matrix

| File | Changes |
|------|---------|
| `temco_bacnet/src/bvlc.c` | Implement `bvlc_send_mpdu`; fill `sin`; runtime mode gates; reduce table sizes |
| `temco_bacnet/private/bip.c` | Define MTU buffer; `bip_socket()`; source helper; remove `#if 0` duplicate |
| `temco_bacnet/src/datalink.c` | Mode-based dispatch to `bvlc_*` vs `bip_*` |
| `temco_bacnet/CMakeLists.txt` | Enable `dlenv.c` (Phase 3) |
| `temco_bacnet/include/bacnet.h` | Optional `BBMD_CLIENT_ENABLED`; table size overrides |
| `main/tcp_server.c` | `BIP_src_addr`; `bip_udp_send_to()`; init/maintenance; socket fd |
| `main/user_data.h` | Extended BBMD config struct |
| `temco_bacnet/private/user_data.h` | Mirror struct changes |
| `main/flash.c` | NVS load/save |
| `main/modbus.c` / `main/modbus.h` | Register map |
| `main/define.h` | `Modbus.bbmd_*` runtime fields |

---

## 10. Configuration and T3000 integration

### Existing unused field

```c
// main/user_data.h
uint8_t BBMD_EN;   // present in struct; never read, written, or synced
```

### Proposed settings (extend `Setting_Info.reg`)

| Field | Type | Purpose |
|-------|------|---------|
| `bbmd_mode` | `uint8_t` | 0=Off, 1=BBMD, 2=Foreign Device |
| `bbmd_address` | `uint32_t` | Remote BBMD IP (network byte order) — FD mode |
| `bbmd_port` | `uint16_t` | Default 47808 |
| `bbmd_ttl` | `uint16_t` | FD lease seconds (e.g. 60–600) |
| `bdt_count` | `uint8_t` | Valid BDT entries — BBMD mode |
| `bdt_entries[N]` | struct | `{ ip, port, mask }` per entry |

### Boot behaviour

| Mode | Actions |
|------|---------|
| Off | Plain BIP; no BBMD init beyond defaults |
| BBMD | `bvlc_intial()` → load BDT from NVS (when implemented) |
| Foreign Device | `dlenv_bbmd_*_set()` → `dlenv_register_as_foreign_device()` |

---

## 11. Testing plan

### Lab topology

- Two subnets separated by a router (no BACnet broadcast crossing)
- Wireshark on UDP 47808 (`bacnet` dissector)
- At least one BACnet device per subnet
- Optional: WireGuard tunnel between subnets for hybrid (Model C) tests

### Test matrix

| # | Scenario | Mode | Pass criteria |
|---|----------|------|---------------|
| 1 | Same-subnet regression | Off | Who-Is, RP, COV unchanged |
| 2 | Write-BDT to BBMD | BBMD | BVLC-Result `0x0000` |
| 3 | Register foreign device | BBMD | Read-FDT shows entry |
| 4 | Local broadcast on BBMD subnet | BBMD | Forwarded-NPDU to BDT peers |
| 5 | Who-Is from FD on subnet A | FD | I-Am from subnet B |
| 6 | Power cycle BBMD gateway | BBMD | BDT restored (v2 persistence) |
| 7 | MS/TP + IP with BBMD | BBMD | No MS/TP polling regression |
| 8 | VPN-only discovery | VPN-first | Who-Is works without BBMD |
| 9 | Hybrid: VPN + local BMS | Model C | BMS on LAN via BBMD; Temco via VPN |
| 10 | T3000 mode change | Config | Mode switch without reflash |

### Module-level checks

- `bvlc_send_mpdu` loopback / second PC
- BDT encode/decode round-trip
- FDT TTL expiry (~TTL + 30 s grace)

---

## 12. Risks and mitigations

| Risk | Severity | Mitigation |
|------|----------|------------|
| Thread safety on `bip_sock` | High | Mutex on all `sendto` |
| Server reply-to-source breaks proactive sends | Medium | Immediate send for BBMD; audit server-initiated broadcasts |
| Dual socket: FD ACK on wrong socket | High | Register via `bip_sock`; handle `BVLC_RESULT` on server path |
| `mtu` / `bip_socket` undefined | High | Phase 0 before any BBMD test |
| Memory (128-entry tables) | Medium | Reduce to 8/16 entries |
| BBMD + WireGuard config duplication | Medium | Unified T3000 UX; architecture decision first |
| MS/TP gateway + Forwarded-NPDU | Low | Explicit hybrid test (scenario 7) |

---

## 13. Effort estimate

| Phase | Duration | Priority |
|-------|----------|----------|
| 0 — Prerequisites | 1 day | **P0** |
| 1 — UDP transport | 1–2 days | **P0** |
| 2 — Server BBMD | 2–3 days | **P1** (gateway SKUs) |
| 3 — Foreign Device | 2–3 days | **P2** (defer if VPN-first) |
| 4 — Config / T3000 | 3–5 days | **P1** (after architecture lock) |
| 5 — Test and harden | 3–5 days | **P1** |
| **Total** | **12–18 days** | |

If WireGuard is the primary remote strategy, BBMD effort may reduce by **~30–50%** by skipping or narrowing Phase 3 and limiting Phase 2 to gateway devices.

---

## 14. Open questions

1. **Product role:** Which T3 models act as BBMD gateway vs VPN peer vs plain LAN device?
2. **WireGuard repo:** Where is Bhavik's branch? Needed for merged transport and config plan.
3. **Topology:** Full mesh, hub-and-spoke, or laptop-to-site only?
4. **BACnet bind target:** Ethernet only, VPN only, or both with policy?
5. **T3000:** Dedicated BBMD page vs unified “Remote connectivity” with WireGuard?
6. **BDT persistence:** Accept RAM-only BDT for v1?
7. **BMS interop:** Must third-party BMS on physical LAN work without VPN?

---

## 15. References

### In-repo files (primary)

| File | Relevance |
|------|-----------|
| `temco_bacnet/src/bvlc.c` | BBMD/BDT/FDT implementation |
| `temco_bacnet/private/bip.c` | BIP encode/decode; send buffers |
| `temco_bacnet/src/datalink.c` | Transport dispatch |
| `temco_bacnet/src/dlenv.c` | Foreign device registration (not built) |
| `temco_bacnet/include/bvlc.h` | BBMD API |
| `main/tcp_server.c` | `bip_task`, `udp_client_send`, `Bacnet_Control` |
| `main/user_data.h` | `BBMD_EN` settings |
| `main/modbus.c` | Register map patterns (`MODBUS_MSTP_NETWORK`) |
| `temco_bacnet/src/h_npdu.c` | BIP ↔ MS/TP gateway (orthogonal to BBMD) |

### Standards and external

- ASHRAE 135 — BACnet/IP Annex J (BBMD, BDT, FDT)
- Steve Karg bacnet-stack — upstream BBMD reference implementation
- ESP-IDF 5.5.3 — LWIP UDP, FreeRTOS tasks

---

*Document maintained in `docs/BBMD-Implementation-Plan.md`. Update when WireGuard integration lands or T3000 register map is finalized.*
