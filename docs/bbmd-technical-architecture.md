# Technical Architecture Document: Multi-Interface Edge Router & BBMD Gateway

**Target Hardware:** Temco T3 Controller (ESP32-S3 Core)  
**Network Topology:** Physical Ethernet (W5500), Cellular Uplink (A7608E via PPPoS), and Secure Virtual Tunnel (WireGuard)

---

## 1. Executive Summary

This document establishes the architecture for utilizing the ESP32-S3 as a multi-interface industrial router. The system bridges local physical field networks with remote cloud-based orchestration. The core challenge requires simultaneous management of local hardware switching, point-to-point cellular data tunneling, cryptographic virtual private networking, and BACnet broadcast routing over non-broadcast-capable media.

---

## 2. Interface Layering & Network Stack Topology

The system abstracts physical hardware drivers into generic network interfaces managed by the native LwIP (Lightweight IP) stack under the `esp_netif` framework.

1. **Physical Ethernet Interface (eth0):** Powered by the external W5500 MAC/PHY. It communicates with the ESP32-S3 via a software-remapped SPI bus utilizing the GPIO Matrix (Pins 1–5). This acts as the local area connection for downstream field controllers.
2. **Physical Cellular Interface (ppp0):** Powered by the A7608E LTE modem. It relies on a dual-phase bring-up model. Phase 1 executes an AT-command state machine over UART (Pins 17/18) for initialization, provisioning, and signal validation. Phase 2 executes a handover protocol, placing the modem into PPPoS (Point-to-Point Protocol over Serial) mode, establishing a raw binary data path.
3. **Virtual Tunnel Interface (wg0):** A software-defined cryptographic interface operating at Layer 3 inside LwIP. It wraps inbound payloads and transmits encrypted packets seamlessly across whichever physical interface is currently selected by the routing engine.

---

## 3. Dynamic Routing & Failover Architecture

Traffic routing is handled dynamically using LwIP routing tables governed by explicit metrics (priorities). Lower metrics represent higher priority paths.

* **Primary Path (Ethernet Connected):**
  * W5500 Interface Route Priority: `10`
  * PPPoS Interface Route Priority: `50`
  * *Behavior:* WireGuard traffic and general out-of-band packets are dispatched via the W5500. The cellular link remains connected but idle as a warm standby.
* **Failover Path (Ethernet Disconnected):**
  * Link status monitoring detects physical layer disconnection or packet loss on the W5500 interface.
  * LwIP automatically pivots the default route to the PPPoS interface (`50`).
  * *Behavior:* The WireGuard tunnel self-heals transparently. The endpoint updates its peer orientation to match the new public IP assigned by the cellular carrier. No connection teardown or renegotiation is visible to the application layer.

---

## 4. Industrial Protocol Routing: BBMD & BACnet/IP

Because WireGuard and PPPoS are strictly point-to-point network topologies, they do not natively forward Layer 3 IP broadcast packets (such as BACnet `Who-Is` or `I-Am` packets). To allow remote device discovery across the WAN, the T3 controller acts as a central BBMD (BACnet Broadcast Management Device).

### Core BBMD Functions

* **Broadcast Distribution Table (BDT):** Manages mapping tables containing the virtual IP addresses of remote BBMD nodes (e.g., cloud orchestration servers). Incoming broadcast requests are wrapped inside directed unicast BVLC (BACnet Virtual Link Control) envelopes to bypass the tunnel limitations.
* **Foreign Device Registration (FDR):** Allows remote isolated workstations or cloud services to register their presence with the T3 controller. The T3 maintains these leases in local memory and relays broadcast packets to registered foreign nodes.
* **Socket Binding Invariant:** The Temco BACnet application layer stack must bind its UDP socket globally to `INADDR_ANY` (`0.0.0.0`) on Port `47808`. This ensures the application captures incoming BACnet frames arriving from both the local physical Ethernet socket and the virtual WireGuard tunnel interface.
* **Local Re-Broadcast:** When the T3 receives a directed BBMD packet from the cloud tunnel, it strips the BVLC header, extracts the raw network protocol data unit, and drops it as a standard local broadcast onto the physical W5500 subnet. Replies are collected as unicast packets and returned up the tunnel.

---

## 5. System Constraints and Guardrails

### MTU Optimization and Packet Fragmentation

Due to nested headers (`BACnet/IP -> UDP -> WireGuard -> PPPoS -> LTE Layer 2`), the effective Maximum Transmission Unit (MTU) drops significantly below the standard 1500-byte Ethernet limit.

* **Action Required:** The BACnet application layer maximum APDU length parameter must be limited (recommended value: `1024` or `1426` bytes). This prevents unwanted packet fragmentation over the cellular link, which drops throughput and triggers carrier packet-drop rules.

### Cellular Data Preservation

BBMD configurations replicate network broadcasts. If unmanaged, a talkative device on the local W5500 network can easily exhaust the LTE data pool by flooding the tunnel with discovery frames.

* **Action Required:** Implement a strict token-bucket rate limiter on the internal BVLC distribution engine to cap outbound WAN broadcasts without impairing local W5500 operations.

### Resource Allocation (DMA & RTOS Priorities)

* **SPI DMA Allocation:** The remapped W5500 SPI driver must utilize Direct Memory Access (DMA) channel resources. This keeps CPU utilization low during intensive local networking.
* **UART Interrupt Isolation:** The PPPoS background tasks must run at an RTOS task priority that avoids starvation by high-frequency local physical IO tasks, preventing buffer overruns on the incoming cellular serial line.
