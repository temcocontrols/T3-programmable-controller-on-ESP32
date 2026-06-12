# TEMCO ESP32 固件 BBMD 功能实现与测试手册

> 适用工程：`temco_app`（ESP-IDF）  
> 协议标准：BACnet/IP Annex J（BBMD / Foreign Device）  
> 文档用途：按步骤在 IDF 工程中完成 BBMD 实现并验证  
> 版本基准：已完成 `bip.c` / `bvlc.c` 收发合并（见阶段 0）

---

## 目录

1. [功能目标与角色定义](#1-功能目标与角色定义)
2. [当前代码状态](#2-当前代码状态)
3. [系统架构](#3-系统架构)
4. [配置设计](#4-配置设计)
5. [实现步骤（按顺序执行）](#5-实现步骤按顺序执行)
6. [测试步骤（按顺序执行）](#6-测试步骤按顺序执行)
7. [抓包判定参考](#7-抓包判定参考)
8. [常见问题排查](#8-常见问题排查)
9. [实现检查清单](#9-实现检查清单)
10. [已知问题与修复指引](#10-已知问题与修复指引)
11. [附录：关键文件索引](#附录关键文件索引)

---

## 1. 功能目标与角色定义

### 1.1 要解决的问题

普通 BACnet/IP 依赖子网广播（Who-Is / I-Am / COV 等）。路由器不转发广播，**跨子网设备无法互相发现**。

BBMD 在子网边界转发广播，使跨子网通信成为可能。

### 1.2 三种工作角色

| 角色 | 说明 | 典型部署 |
|------|------|----------|
| **普通设备** | 仅本子网广播，现有默认行为 | 单网段楼宇 |
| **Foreign Device (FD)** | 向 BBMD 注册，广播经 BBMD 转发 | T3 在子网 A，需访问子网 B |
| **BBMD Server** | 维护 BDT/FDT，转发广播 | T3 作为网段边界路由器 |

### 1.3 模式编码建议（`BBMD_EN`）

| 值 | 模式 | `bbmd_en` | 行为摘要 |
|----|------|-----------|----------|
| `0` | 普通（默认） | `0` | 与现网完全一致 |
| `1` | Foreign Device | `0` | 向 BBMD 注册，广播走 Distribute-Broadcast |
| `2` | BBMD Server | `1` | 处理 BDT/FDT，转发广播 |
| `3` | BBMD + FD | `1` + FD 注册 | 同时做 BBMD 并向远端 BBMD 注册 |

> **兼容原则：** `BBMD_EN = 0` 时，所有 BBMD 管理报文被丢弃，广播仍用 `Original-Broadcast-NPDU`。

---

## 2. 当前代码状态

### 2.1 已完成（阶段 0 — 收发合并）

在 IDF 的 `temco_bacnet` 中应已包含以下改动：

| 模块 | 状态 | 说明 |
|------|------|------|
| `private/bip.c` | 已完成 | `bip_receive` 委托 `bvlc_handle_npdu`；FD 广播并入 `bip_send_pdu` |
| `src/bvlc.c` | 已完成 | `bvlc_handle_npdu()` 统一 BVLC 分发；删除 `bvlc_receive`/`bvlc_send_pdu` |
| `include/bip.h` / `bvlc.h` | 已完成 | 新 API；`bbmd_en` 声明 |
| `CMakeLists.txt` | 已完成 | 增加 `lwip` 依赖 |
| `main/tcp_server.c` | 已完成 | `bip_set_udp_sock`、`bip_set_source_addr`、`bip_mpdu_dest` |

### 2.2 已完成（阶段 1~9 主体）

| 模块 | 状态 | 位置 |
|------|------|------|
| `Str_Setting_Info` 扩展 | 已完成 | `main/user_data.h`、`temco_bacnet/private/user_data.h` |
| `bbmd_apply_config()` | 已完成 | `main/modbus.c` — 模式映射、`register_ftd()` 调用、port/ttl 默认值 |
| `dealwith_write_setting()` 挂钩 | 已完成 | `main/modbus.c` — T3000 写 400 字节块时触发 |
| `src/dlenv.c` 编译 | 已完成 | `temco_bacnet/CMakeLists.txt` 第 43 行 |
| `bvlc_intial()` + `bbmd_apply_config()` | 已完成 | `main/tcp_server.c` → `Inital_Bacnet_Server()` |
| 定时维护 | 已完成 | `Timer_task()` 每秒调用 `bvlc_maintenance_timer(1)`、`dlenv_maintenance_timer(1)` |
| FD 注册 TTL 参数化 | 已完成 | `bvlc_encode_register_foreign_device()` 使用 `time_to_live_seconds` |
| IP 变更处理 | 已完成 | `Eth_IP_Change == 1` 时先 `bbmd_apply_config()`，约 5 秒后软复位 |
| Flash 默认值 | 部分完成 | NVS 键 `FLASH_BBMD_EN` 默认 0；`user_data.c` 中 port/ttl 默认 47808/600 |
| Modbus `BBMD_EN` 寄存器 | 已完成 | 寄存器 48（`MODBUS_BBMD_EN`）读写 + flash 保存 |

### 2.3 待完成 / 待修复（阶段 10~12）

| 模块 | 状态 | 说明 |
|------|------|------|
| `bbmd_test_demo()` 临时测试代码 | **需移除** | `Inital_Bacnet_Server()` 中硬编码 BDT 192.168.0.95，量产前必须删除或 `#ifdef` 保护 |
| `bvlc_test_*` API 同步 | **需同步** | IDF 组件 `temco_bacnet` 已有 `bvlc_test_clear_bdt/set_bdt_entry` 等；`work/temco_bacnet` 尚未同步 |
| `dealwith_write_setting()` 遗留逻辑 | **需修复** | 约 3762 行：`bbmd_en = ptr->reg.BBMD_EN` 直接赋值错误（mode 2/3 应设 `bbmd_en=1`）；与下方 3818 行新逻辑重复 |
| Modbus IP/Port/TTL 寄存器 | 未完成 | 仅 `MODBUS_BBMD_EN`（48），缺少 ip/port/ttl 映射（见阶段 11） |
| NVS 独立持久化 ip/port/ttl | 未完成 | 目前仅 `BBMD_EN` 存 NVS；ip/port/ttl 依赖 T3000 400 字节块或 RAM 默认值 |
| `_Static_assert(sizeof <= 400)` | 未完成 | 结构体扩展后未加编译期校验 |
| `bbmd_port` 字节序约定 | 待统一 | 代码中默认 `47808`（主机序）；Annex J 端口字段为网络序 `0xBAC0`；当前 `register_ftd` 经 `htons` 处理，但存储格式需文档化 |
| T3000 界面 BDT 配置 | 未完成 | 仅支持 BACnet 工具 Write-BDT 或临时 `bbmd_test_demo` |
| 模式热切换软复位 | 建议启用 | `dealwith_write_setting` 中 `esp_retboot()` 仍被注释 |
| 联调测试 T0~T4 | 未完成 | 见第 6 节 |

### 2.4 实现进度快照（2026-06-08）

```
阶段 0  收发合并          ████████████ 100%
阶段 1  IDF 编译验证       ██████████░░  85%  （bvlc_test API 待同步）
阶段 2  结构体扩展         ████████████ 100%
阶段 3  bbmd_apply_config  ██████████░░  90%  （遗留 bbmd_en 赋值 bug）
阶段 4  dlenv.c 链接       ████████████ 100%
阶段 5  BACnet 初始化      ██████████░░  90%  （含临时 bbmd_test_demo）
阶段 6  定时维护          ████████████ 100%
阶段 7  TTL 修复           ████████████ 100%
阶段 8  Flash 持久化       ██████░░░░░░  50%  （仅 BBMD_EN NVS）
阶段 9  IP 变更            ████████████ 100%
阶段 10 BDT 配置           ████░░░░░░░░  30%  （仅工具/硬编码）
阶段 11 Modbus 扩展        ███░░░░░░░░░  25%  （仅 EN 寄存器）
阶段 12 量产清理           ░░░░░░░░░░░░   0%
```

---

## 3. 系统架构

### 3.1 分层结构（合并后）

```
应用层 (main/)
  tcp_server.c     bip_task / Scan_network_bacnet_Task / Timer_task
  modbus.c         设置读写 dealwith_write_setting()
  flash.c          持久化
        |
        v
数据链路层 (temco_bacnet/private/bip.c)
  bip_receive()        收包入口，填充源地址
  bip_send_pdu()       发包入口（含 FD 广播判断）
  bip_send_mpdu()      BBMD 即时转发（sendto）
        |
        v
BBMD 业务层 (temco_bacnet/src/bvlc.c)
  bvlc_handle_npdu()   BVLC 功能码分发
  BDT/FDT 表管理
  bvlc_*_forward_npdu() 跨子网转发
  bvlc_register_with_bbmd()  FD 注册
        |
        v
环境层 (temco_bacnet/src/dlenv.c)  <-- 待启用
  register_ftd()           FD 注册入口
  dlenv_maintenance_timer() 租约续期
```

### 3.2 收包流程

```
bip_task: recvfrom(47808)
  -> bip_set_source_addr()        // 填充请求方地址
  -> datalink_receive(BAC_IP)
    -> bip_receive()
      -> bvlc_handle_npdu()       // BBMD 业务处理
  -> npdu_handler()               // BACnet 应用层
  -> sendto(bip_send_buf)         // 单播响应回请求方
```

### 3.3 发包流程

| 场景 | 路径 | BVLC 类型 |
|------|------|-----------|
| 普通广播 | `bip_send_pdu` -> `bip_send_buf` -> sendto(广播地址) | Original-Broadcast |
| FD 广播 | `bip_send_pdu` -> sendto(BBMD地址) | Distribute-Broadcast |
| BBMD 转发 | `bvlc_send_mpdu` -> `bip_send_mpdu` -> sendto | Forwarded-NPDU |
| BBMD 响应 | `bvlc_send_result` -> `bip_send_mpdu` -> sendto(请求方) | Result |

### 3.4 datalink 调用关系

`datalink.c` 保持不变，统一走 `bip_*`：

```c
// datalink_send_pdu -> bip_send_pdu / bip_send_pdu_client
// datalink_receive  -> bip_receive
```

### 3.5 不受 BBMD 影响的功能

- MS/TP（RS485）主从通信
- Modbus RTU / TCP
- TEMCO UDP 1234 设备扫描
- BACnet 私有传输（ptransfer）
- SNTP 时间同步（走 MSTP 广播）

---

## 4. 配置设计

### 4.1 扩展 `Str_Setting_Info`（400 字节限制）

当前 `BBMD_EN` 在 `main/user_data.h` 第 186 行，后面是 `sd_exist`。  
**同步修改** `temco_bacnet/private/user_data.h`。

在 `BBMD_EN` 后增加（共 7 字节）：

```c
uint8_t  BBMD_EN;       // 0=普通 1=FD 2=BBMD 3=BBMD+FD
uint32_t bbmd_ip;       // BBMD IPv4，网络字节序
uint16_t bbmd_port;     // 默认 0xBAC0 (47808)，网络字节序
uint16_t bbmd_ttl;      // FD 租约秒数，建议 60~3600，默认 600
uint8_t  sd_exist;      // 原有字段，位置后移
```

**操作前必须确认：** `sizeof(Str_Setting_Info) <= 400`，否则破坏 T3000 私有传输协议。

验证方法：

```c
_Static_assert(sizeof(Str_Setting_Info) <= 400, "Str_Setting_Info overflow");
```

### 4.2 运行时变量映射

| 配置字段 | 运行时变量 | 文件 |
|----------|------------|------|
| `BBMD_EN == 2 或 3` | `bbmd_en = 1` | `bvlc.c` |
| `BBMD_EN == 0 或 1` | `bbmd_en = 0` | `bvlc.c` |
| `bbmd_ip/port/ttl` | `register_ftd()` 参数 | `dlenv.c` |

### 4.3 出厂默认值

```c
Setting_Info.reg.BBMD_EN   = 0;
Setting_Info.reg.bbmd_ip   = 0;
Setting_Info.reg.bbmd_port = 47808;    // 主机序；register_ftd 内部经 htons 转换
Setting_Info.reg.bbmd_ttl  = 600;
bbmd_en = 0;
```

> **持久化说明：** `BBMD_EN` 单独存 NVS（`FLASH_BBMD_EN`）。`bbmd_ip/port/ttl` 随 T3000 私有传输 400 字节块读写；Modbus 仅写 `BBMD_EN` 时会单独存 NVS，ip/port/ttl 需通过 T3000 设置界面写入。

---

## 5. 实现步骤（按顺序执行）

### 阶段 1：确认阶段 0 合并在 IDF 中生效 ✅ 基本完成

**检查文件：**

- [x] `temco_bacnet/private/bip.c` — 含 `bvlc_handle_npdu` 调用
- [x] `temco_bacnet/src/bvlc.c` — 含 `bvlc_handle_npdu()`，无 `bvlc_receive`
- [x] `temco_bacnet/CMakeLists.txt` — `REQUIRES lwip`
- [x] `main/tcp_server.c` — 含 `bip_set_udp_sock`、`bip_set_source_addr`
- [ ] `work/temco_bacnet` 与 IDF 组件同步 `bvlc_test_*` API（若保留 `bbmd_test_demo`）

**编译验证：**

```bash
idf.py build
```

预期：无 `bvlc_receive` / `bvlc_send_pdu` 链接错误。若 `bbmd_test_demo` 启用且 work 树无 `bvlc_test_*`，会报链接错误。

---

### 阶段 2：扩展配置结构体 ✅ 已完成

**文件：**

1. `main/user_data.h` — 已增加 `bbmd_ip`、`bbmd_port`、`bbmd_ttl`
2. `temco_bacnet/private/user_data.h` — 已同步

**待验证：** 增加 `_Static_assert(sizeof(Str_Setting_Info) <= 400)`。

---

### 阶段 3：实现 BBMD 配置应用函数 ✅ 已实现（待修复遗留逻辑）

**当前实现位置：** `main/modbus.c` → `bbmd_apply_config()`

```c
void bbmd_apply_config(void)
{
    uint8_t mode = Setting_Info.reg.BBMD_EN;
    uint16_t port = Setting_Info.reg.bbmd_port;
    uint16_t ttl = Setting_Info.reg.bbmd_ttl;

    if (mode > 3) {
        mode = 0;
        Setting_Info.reg.BBMD_EN = 0;
    }
    if (port == 0) port = 47808;
    if (ttl == 0) ttl = 600;

    if (mode == 2 || mode == 3) {
        bbmd_en = 1;
    } else {
        bbmd_en = 0;
    }

    if ((mode == 1 || mode == 3) && (Setting_Info.reg.bbmd_ip != 0)) {
        register_ftd(
            (long) Setting_Info.reg.bbmd_ip,
            (int) port,
            (int) ttl);
    }
}
```

**T3000 写设置挂钩**（`dealwith_write_setting()` 约 3818 行）已实现：

```c
if ((Setting_Info.reg.BBMD_EN != ptr->reg.BBMD_EN)
    || (Setting_Info.reg.bbmd_ip != ptr->reg.bbmd_ip)
    || (Setting_Info.reg.bbmd_port != ptr->reg.bbmd_port)
    || (Setting_Info.reg.bbmd_ttl != ptr->reg.bbmd_ttl))
{
    Setting_Info.reg.BBMD_EN   = ptr->reg.BBMD_EN;
    Setting_Info.reg.bbmd_ip   = ptr->reg.bbmd_ip;
    Setting_Info.reg.bbmd_port = ptr->reg.bbmd_port;
    Setting_Info.reg.bbmd_ttl  = ptr->reg.bbmd_ttl;
    bbmd_apply_config();
}
```

**Modbus 寄存器 48**（`MODBUS_BBMD_EN`）写操作已调用 `bbmd_apply_config()` 并保存 NVS。

**待修复：** 约 3762 行遗留代码 `bbmd_en = ptr->reg.BBMD_EN` 应删除，改由上方统一块处理（见 [10.1](#101-dealwith_write_setting-遗留逻辑)）。

---

### 阶段 4：启用 `dlenv.c` 编译 ✅ 已完成

**文件：** `temco_bacnet/CMakeLists.txt` 第 43 行 `"src/dlenv.c"` 已启用。

---

### 阶段 5：BACnet 初始化挂钩 ✅ 已实现（含临时测试代码）

**文件：** `main/tcp_server.c` → `Inital_Bacnet_Server()`

当前代码（约 1927~1930 行）：

```c
bip_set_broadcast_addr(0xffffffff);
bvlc_intial();
bbmd_apply_config();
bbmd_test_demo();   // ⚠️ 临时测试，量产前删除
```

**`bbmd_test_demo()` 说明：** 硬编码 BDT 条目 `192.168.0.95:47808`，仅用于开发联调。正式固件必须移除或加 `#ifdef BBMD_TEST_DEMO` 保护。

**说明：**

- `bvlc_intial()` 清零 `BBMD_Table` 和 `FD_Table`
- 以太网 IP 就绪后才有效；IP 变更时 `Timer_task` 中 `Eth_IP_Change` 分支会再次调用 `bbmd_apply_config()`

---

### 阶段 6：定时维护 ✅ 已完成

**文件：** `main/tcp_server.c` → `Timer_task()` 约 2750 行

```c
bvlc_maintenance_timer(1);
dlenv_maintenance_timer(1);
```

---

### 阶段 7：修复 FD 注册 TTL 硬编码 ✅ 已完成

**文件：** `temco_bacnet/src/bvlc.c` → `bvlc_encode_register_foreign_device()`

当前实现已使用参数 `time_to_live_seconds`，BVLC Length = 6。

---

### 阶段 8：Flash 持久化 ⚠️ 部分完成

**已完成：**

- `flash.c`：NVS 键 `FLASH_BBMD_EN`，默认 0
- `user_data.c`：`Sync_Panel_Info()` 中 `bbmd_port/ttl` 为 0 时设默认 47808/600
- T3000 私有传输：`ptransfer.c` 读写 `Setting_Info.all[400]` 整块，新字段随结构体偏移自动包含

**待完成：**

1. 在 `user_data.h` 或 `flash.c` 增加 `_Static_assert(sizeof(Str_Setting_Info) <= 400)`
2. 可选：为 `bbmd_ip/port/ttl` 增加独立 NVS 键（Modbus 单独配置时不依赖 T3000）
3. 结构体偏移变化后，旧 flash 数据需版本兼容或恢复出厂

---

### 阶段 9：IP 变更处理 ✅ 已完成

**文件：** `main/tcp_server.c` → `Timer_task()` 约 2690 行

```c
if (Eth_IP_Change == 1) {
    if (ip_change_count == 0) {
        bbmd_apply_config();   // IP 变更后立即重注册 FD
    }
    if (ip_change_count++ > 5) {
        /* ... Save_Ethernet_Info ... */
        esp_retboot();         // 约 5 秒后软复位
    }
}
```

---

### 阶段 10：BBMD Server 模式 BDT 配置

BBMD 启动后 BDT 为空，需通过外部工具写入。

**BDT 条目格式（每条 10 字节）：**

| 偏移 | 长度 | 内容 |
|------|------|------|
| 0 | 4 | 远端 BBMD IPv4（网络字节序） |
| 4 | 2 | 端口（网络字节序，通常 0xBAC0） |
| 6 | 4 | 广播掩码（0xFFFFFFFF = 单播到 BBMD） |

**配置方式（二选一）：**

**方式 A — BACnet 工具（推荐测试用）：**  
用 YABE / VTS 向 T3 发送 `Write-Broadcast-Distribution-Table`（`bbmd_en=1` 时 T3 响应）。

**方式 B — T3000 扩展（量产后）：**  
在 T3000 设置界面增加 BDT 配置，通过私有传输或 Modbus 写入。

**示例：** T3 在 192.168.1.10，远端 BBMD 在 192.168.2.10：

```
条目: IP=192.168.2.10, Port=47808, Mask=0xFFFFFFFF
```

---

### 阶段 11：Modbus 寄存器映射 ⚠️ 部分完成

**已完成：** 寄存器 48（`MODBUS_BBMD_EN`）读写 + NVS 保存。

**待扩展**（若 T3000 未支持 BBMD 全参数前需 Modbus 配置）：

| 寄存器 | 内容 | 状态 |
|--------|------|------|
| Holding 48 | `BBMD_EN` | ✅ 已实现 |
| Holding 49~50 | `bbmd_ip`（32 位，高低字） | ❌ 待实现 |
| Holding 51 | `bbmd_port` | ❌ 待实现 |
| Holding 52 | `bbmd_ttl` | ❌ 待实现 |

在 `modbus.c` 读写处理中映射到 `Setting_Info.reg.*`，写后调用 `bbmd_apply_config()`。

---

### 阶段 12：量产前清理（必做）

| 序号 | 操作 | 文件 |
|------|------|------|
| 12-1 | 删除或 `#ifdef BBMD_TEST_DEMO` 保护 `bbmd_test_demo()` 调用 | `tcp_server.c` |
| 12-2 | 删除 `dealwith_write_setting()` 3762~3765 行遗留 `bbmd_en` 直接赋值 | `modbus.c` |
| 12-3 | 将 IDF 组件中 `bvlc_test_*` API 同步到 `work/temco_bacnet`（或移除对它们的依赖） | `bvlc.c`、`bvlc.h` |
| 12-4 | 增加 `_Static_assert(sizeof(Str_Setting_Info) <= 400)` | `user_data.h` |
| 12-5 | 模式变更后考虑启用 `esp_retboot()` | `modbus.c` |
| 12-6 | 执行 T0~T4 全部测试并记录结果 | — |

---

## 6. 测试步骤（按顺序执行）

### 6.1 测试环境准备

**硬件：**

| 设备 | 数量 | 要求 |
|------|------|------|
| 被测 T3 控制器 | 1 | 以太网，刷入被测固件 |
| 参考 BBMD | 1 | 第三方 BBMD，或另一台 T3（BBMD 模式） |
| 交换机/路由器 | 1+ | 至少划分 2 个子网 |
| 抓包电脑 | 1 | Wireshark，与两个子网均可达 |

**网络拓扑示例：**

```
子网 A: 192.168.1.0/24
  +-- BBMD-A:  192.168.1.10  (参考 BBMD 或 T3 BBMD 模式)
  +-- DUT:     192.168.1.100 (被测 T3)
  +-- 工具PC:  192.168.1.50

子网 B: 192.168.2.0/24
  +-- BBMD-B:  192.168.2.10
  +-- 远端设备: 192.168.2.50 (任意 BACnet 设备)
  +-- 路由器连接 A <-> B（不转发广播）
```

**软件：**

- Wireshark，过滤器：`udp.port == 47808`
- BACnet 扫描工具（YABE / BACnet Explorer）
- T3000 配置工具（写 `Str_Setting_Info`）

**测试前确认：**

- [ ] 固件已包含阶段 0~8 全部改动
- [ ] `BBMD_EN = 0` 时设备正常上线
- [ ] 抓包电脑能看到 UDP 47808 流量

---

### 6.2 阶段 T0：普通模式回归（必过）

**配置：** `BBMD_EN = 0`，重启设备。

| 编号 | 操作 | 预期结果 | 判定 |
|------|------|----------|------|
| T0-01 | 同子网工具发 Who-Is | 设备 I-Am 响应 | [ ] |
| T0-02 | T3000 扫描发现设备 | 正常发现 | [ ] |
| T0-03 | Read Property 本地对象 | 读写成功 | [ ] |
| T0-04 | 网络点扫描（同子网） | 远端点可发现 | [ ] |
| T0-05 | COV 订阅同子网网络点 | 收到 COV 通知 | [ ] |
| T0-06 | MS/TP Who-Is/I-Am | 与改前一致 | [ ] |
| T0-07 | Modbus TCP 读写 | 正常 | [ ] |
| T0-08 | 向设备发 Write-BDT | **无响应**（bbmd_en=0 丢弃） | [ ] |
| T0-09 | 重启后 `BBMD_EN` 仍为 0 | flash 默认值正确 | [ ] |

> **T0 全部通过才可进入后续测试。**

---

### 6.3 阶段 T1：Foreign Device 模式

**配置：**

```
BBMD_EN   = 1
bbmd_ip   = BBMD-A 地址（如 192.168.1.10，网络字节序）
bbmd_port = 0xBAC0
bbmd_ttl  = 600
```

重启 DUT（192.168.1.100）。

| 编号 | 操作 | 抓包预期 | 判定 |
|------|------|----------|------|
| T1-01 | 启动后 5 秒内抓包 | DUT -> BBMD-A: Register-Foreign-Device, TTL=600 | [ ] |
| T1-02 | 检查 BBMD 响应 | BBMD-A -> DUT: BVLC-Result, code=0 | [ ] |
| T1-03 | 等待 590~610 秒 | 自动重发 Register-Foreign-Device | [ ] |
| T1-04 | DUT 发起网络点 Who-Is 扫描 | DUT -> BBMD-A: Distribute-Broadcast-to-Network | [ ] |
| T1-05 | 子网 B 设备响应 | 子网 B 设备 I-Am 经 BBMD 转发到达 DUT | [ ] |
| T1-06 | DUT 发现并添加子网 B 网络点 | remote_panel_db 有记录 | [ ] |
| T1-07 | Read Property 子网 B 远端点 | 读写成功 | [ ] |
| T1-08 | 订阅子网 B 点 COV | 收到 COV 通知 | [ ] |
| T1-09 | 同子网 A 设备通信 | 单播正常 | [ ] |
| T1-10 | 断开 BBMD-A | 跨子网失败；同子网 A 仍正常 | [ ] |
| T1-11 | 恢复 BBMD-A | 600 秒内自动重注册，跨子网恢复 | [ ] |
| T1-12 | 改 BBMD_EN=0 重启 | 不再发 Register-FD；T0 全部通过 | [ ] |

---

### 6.4 阶段 T2：BBMD Server 模式

**配置 DUT（192.168.1.10）为 BBMD：**

```
BBMD_EN = 2
```

**用工具向 DUT 写 BDT：**

```
条目1: 192.168.2.10, Port=47808, Mask=0xFFFFFFFF
```

| 编号 | 操作 | 抓包预期 | 判定 |
|------|------|----------|------|
| T2-01 | 发送 Write-BDT | DUT 响应 BVLC-Result=0 | [ ] |
| T2-02 | 发送 Read-BDT | 返回正确 BDT 条目 | [ ] |
| T2-03 | 子网 A 其他设备发 Original-Broadcast（Who-Is） | DUT 发 Forwarded-NPDU 到 192.168.2.10 | [ ] |
| T2-04 | 子网 B 设备收到广播 | I-Am 经转发到达子网 B | [ ] |
| T2-05 | 子网 B 外 FD 向 DUT 注册 | Register-Foreign-Device 成功 | [ ] |
| T2-06 | Read-FDT | 返回 FD 条目 | [ ] |
| T2-07 | 等待 TTL 过期（不续租） | FDT 条目被 bvlc_maintenance_timer 清除 | [ ] |
| T2-08 | FD 发 Distribute-Broadcast | 子网 A 本地设备收到广播 | [ ] |
| T2-09 | 改 BBMD_EN=0 重启 | Write-BDT 无响应；T0 通过 | [ ] |

---

### 6.5 阶段 T3：双模式（可选）

**配置：** `BBMD_EN = 3`，同时配置本地 BDT 和远端 BBMD 地址。

| 编号 | 操作 | 预期 | 判定 |
|------|------|------|------|
| T3-01 | 同时响应 BDT 管理与 FD 注册 | 两种角色均正常 | [ ] |
| T3-02 | 跨双跳子网（A->B->C）广播 | 广播可达第三子网 | [ ] |

---

### 6.6 阶段 T4：边界与异常

| 编号 | 场景 | 操作 | 预期 | 判定 |
|------|------|------|------|------|
| T4-01 | BBMD 地址无效 | bbmd_ip = 0 | 不注册，保持普通广播 | [ ] |
| T4-02 | TTL=0 | bbmd_ttl = 0 | 注册失败或立即过期 | [ ] |
| T4-03 | 错误 BBMD 地址 | 指向不存在的主机 | 注册无响应，跨子网失败，设备不死机 | [ ] |
| T4-04 | 端口非 47808 | bbmd_port = 47809 | 按配置端口通信 | [ ] |
| T4-05 | IP 变更 | 修改 DUT IP 后 | FD 自动重注册 | [ ] |
| T4-06 | 模式热切换 | 运行中改 BBMD_EN | 建议重启后行为正确 | [ ] |
| T4-07 | 广播风暴 | 连续 Who-Is 100 次 | 设备不死机，CPU/内存稳定 | [ ] |

---

## 7. 抓包判定参考

### 7.1 Wireshark 过滤器

```
udp.port == 47808
```

### 7.2 关键报文对照

**FD 注册：**

```
源: DUT (192.168.1.100:47808)
目标: BBMD (192.168.1.10:47808)
BVLC Function: Register-Foreign-Device
TTL: 600
-> 响应 BVLC-Result: Successful Completion (0x0000)
```

**FD 广播 Who-Is：**

```
源: DUT
目标: BBMD
BVLC: Distribute-Broadcast-to-Network
NPDU: Who-Is (-1,-1)
```

**BBMD 转发：**

```
源: BBMD (DUT in BBMD mode)
目标: 远端子网 BBMD 或广播
BVLC: Forwarded-NPDU
Original-Source: <发起方 IP:port>
NPDU: <广播内容>
```

**普通模式（对照）：**

```
BVLC: Original-Broadcast-NPDU
目标: 子网广播地址（如 192.168.1.255:47808）
```

---

## 8. 常见问题排查

| 现象 | 可能原因 | 排查方法 |
|------|----------|----------|
| 编译报 `bvlc_receive` 未定义 | IDF 中仍有旧调用 | 全局搜索，改为 `bip_*` |
| 编译报 `register_ftd` 未定义 | `dlenv.c` 未加入 CMakeLists | 取消注释第 43 行 |
| Write-BDT 无响应 | `bbmd_en=0` | 确认 BBMD_EN=2 且 `bbmd_apply_config()` 已调用 |
| FD 注册无响应 | `bip_udp_sock` 未设置 | 确认 `bip_set_udp_sock(bip_sock)` 在 socket 创建后 |
| BBMD 回包到 0.0.0.0 | 源地址未填充 | 确认 `bip_set_source_addr()` 在 recvfrom 后 |
| FD 注册 TTL 总是 100s | 硬编码未修复 | 检查阶段 7 修改是否生效 |
| 跨子网 Who-Is 失败 | FD 未注册或 BDT 为空 | 抓包确认 Register-FD 和 Distribute-Broadcast |
| 同子网正常，跨子网失败 | 仅 FD 路径问题 | 先过 T1，再测 T2 |
| 模式切换后行为异常 | 未重启 / 遗留 bbmd_en 赋值 | 删除 modbus.c 3762 行；模式变更后 `esp_retboot()` |
| T3000 设置不生效 | 结构体未同步 / SN 校验失败 | 检查 `dealwith_write_setting` 中 SN 匹配逻辑 |
| 启动后 BDT 含 192.168.0.95 | `bbmd_test_demo()` 未移除 | 删除阶段 12-1 临时测试代码 |
| 链接错误 `bvlc_test_*` | work 树未同步 IDF 组件 | 同步 `bvlc_test_clear_bdt` 等 API 或移除调用 |

---

## 9. 实现检查清单

### 代码实现

- [x] **阶段 0** — `bip.c`/`bvlc.c` 收发合并已同步到 IDF
- [ ] **阶段 1** — IDF 编译通过（`bvlc_test_*` API 待 work 树同步）
- [x] **阶段 2** — `Str_Setting_Info` 扩展（待 `_Static_assert`）
- [x] **阶段 3** — `bbmd_apply_config()` + `dealwith_write_setting()` 挂钩（待删遗留逻辑）
- [x] **阶段 4** — `dlenv.c` 加入 CMakeLists
- [x] **阶段 5** — `Inital_Bacnet_Server()` 调用 `bvlc_intial()` + `bbmd_apply_config()`（待删 `bbmd_test_demo`）
- [x] **阶段 6** — `Timer_task()` 调用两个 maintenance_timer
- [x] **阶段 7** — FD 注册 TTL 参数化
- [ ] **阶段 8** — Flash 默认值与 NVS（仅 `BBMD_EN` 完成）
- [x] **阶段 9** — IP 变更后 FD 重注册
- [ ] **阶段 10** — BDT 配置方式（工具可用；T3000 未实现）
- [ ] **阶段 11** — Modbus 扩展（仅 EN 寄存器）
- [ ] **阶段 12** — 量产清理（6 项）

### 测试验证

- [ ] **T0** — 普通模式回归（9 项全过）
- [ ] **T1** — Foreign Device 模式（12 项）
- [ ] **T2** — BBMD Server 模式（9 项）
- [ ] **T3** — 双模式（可选，2 项）
- [ ] **T4** — 边界异常（7 项）

---

## 10. 已知问题与修复指引

### 10.1 `dealwith_write_setting` 遗留逻辑

**位置：** `main/modbus.c` 约 3762~3765 行

**问题：**

```c
if (bbmd_en != ptr->reg.BBMD_EN) {
    bbmd_en = ptr->reg.BBMD_EN;   // 错误：mode=2 时 BBMD_EN=2 但 bbmd_en 应为 1
    save_uint8_to_flash(FLASH_BBMD_EN, bbmd_en);
}
```

`BBMD_EN` 是模式编码（0~3），`bbmd_en` 是 BBMD Server 开关（0/1）。直接赋值会导致 mode=2 时 `bbmd_en=2`，Write-BDT 等 BBMD 管理报文无法正确处理。

**修复：** 删除此块，统一由 3818 行的四字段比较 + `bbmd_apply_config()` 处理；NVS 保存 `Setting_Info.reg.BBMD_EN` 而非 `bbmd_en`。

### 10.2 `bbmd_test_demo()` 硬编码 BDT

**位置：** `main/modbus.c` + `tcp_server.c` 启动调用

硬编码 `192.168.0.95` 作为 BDT 条目，会污染所有启动实例的 BDT 表。联调完成后必须移除。

**依赖 API**（IDF 组件已有，work 树待同步）：

```c
void bvlc_test_clear_bdt(void);
void bvlc_test_clear_fdt(void);
bool bvlc_test_set_bdt_entry(int index, uint32_t ip, uint16_t port, uint32_t mask);
```

### 10.3 `bbmd_port` 字节序

- 存储：`Setting_Info.reg.bbmd_port` 当前以主机序 `47808` 保存
- 传输：`register_ftd()` → `dlenv_bbmd_port_set()` → `htons()` 转为网络序
- T3000 写入时应与现有 IP 字段约定一致（建议统一文档说明）

### 10.4 下一步建议执行顺序

1. 修复 10.1 遗留逻辑（5 分钟）
2. 同步或移除 `bvlc_test_*` + 删除 `bbmd_test_demo` 调用
3. 加 `_Static_assert`
4. 执行 T0 回归测试
5. 按网络拓扑执行 T1/T2
6. 按需扩展 Modbus 寄存器 49~52

---

## 附录：关键文件索引

| 文件 | 职责 |
|------|------|
| `temco_bacnet/private/bip.c` | 数据链路：收发入口 |
| `temco_bacnet/src/bvlc.c` | BBMD 业务：BDT/FDT/转发 |
| `temco_bacnet/src/dlenv.c` | FD 注册与续租 |
| `temco_bacnet/src/datalink.c` | 协议路由（不改） |
| `main/tcp_server.c` | `bip_task`、初始化、定时器 |
| `main/modbus.c` | `bbmd_apply_config()`、Modbus 寄存器 48、T3000 设置应用 |
| `main/modbus.h` | `MODBUS_BBMD_EN`、`bbmd_apply_config()` 声明 |
| `main/user_data.h` | 配置结构体 |
| `main/flash.c` | NVS `FLASH_BBMD_EN` 持久化 |
| `temco_bacnet/private/ptransfer.c` | T3000 400 字节块读写 |

---

*文档版本：2026-06-08（同步代码实现进度，新增阶段 12 与已知问题章节）*
