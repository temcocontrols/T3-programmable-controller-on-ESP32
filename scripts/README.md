# SNMP Test Scripts (Windows)

This repository contains Windows batch scripts used to test **SNMP v2c GET and SET**
operations against an SNMP agent (for example: ESP32-based SNMP agent, Net-SNMP agent,
or other embedded/network devices).

These scripts are intended for **functional testing, integration testing, and
regression validation** of SNMP OIDs.

---

## 1. Prerequisites

- Windows operating system
- Net-SNMP tools installed:
  - `snmpget`
  - `snmpset`
- SNMP agent reachable over IP network
- SNMP v2c enabled on the target device
- `snmpget` and `snmpset` available in system `PATH`

---

## 2. Scripts Overview

This repository contains the following test scripts:

### 2.1 `read_T3_input.bat`  `read_T3_output.bat`  `read_T3_variable.bat`

- Reads all properties of SNMP objects
- Supports reading a single object or a range of objects
- Useful for validating object mapping and configuration

### 2.2 `read_write_T3_output.bat` `read_write_T3_variable.bat`

- Performs SNMP GET → SET → GET validation
- Verifies write access and data persistence
- Supports INTEGER and STRING OIDs using index-based addressing

---

## 3. `read_T3_input.bat`

### 3.1 Purpose

`read_T3_input.bat` `read_T3_output.bat` `read_T3_variable.bat` reads **all properties of SNMP objects** based on an object index.

Each object exposes the following properties:
- Object index
- Configuration type
- Analog value
- Binary value

This script is useful for:
- Validating SNMP object structure
- Verifying configuration-based behavior
- Debugging incorrect OID mapping

---

### 3.2 OIDs Used

For each object index `<index>`:

- Object Index  
  `1.3.6.1.4.1.64991.1.1.1.<index>`

- Configuration Type  
  `1.3.6.1.4.1.64991.1.1.2.<index>`

- Analog Value  
  `1.3.6.1.4.1.64991.1.1.3.<index>`

- Binary Value  
  `1.3.6.1.4.1.64991.1.1.4.<index>`

---

### 3.3 Usage

#### Read all properties of a single object

```bat
read_T3_input.bat <IP> <ObjectIndex>

read_T3_output.bat <IP> <ObjectIndex>

read_T3_variable.bat <IP> <ObjectIndex>
```

#### Read all properties of all input object

```bat
read_T3_input.bat <IP>
```

#### Read all properties of all output object
```bat
read_T3_output.bat <IP>
```

#### Read all properties of a single object
```bat
read_T3_variable.bat <IP>
```

#### Read/write value of a single object

```bat
read_write_T3_output.bat <IP> <ObjectIndex>

read_write_T3_variable.bat <IP> <ObjectIndex>
```