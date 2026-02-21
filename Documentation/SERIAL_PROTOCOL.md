# MK-312BT Serial Protocol Implementation

This document describes the complete MK-312BT serial protocol as implemented in `serial.c`.

## Protocol Overview

The MK-312BT uses a simple packet-based serial protocol at 19200 baud (8N1) with XOR encryption and checksums. The protocol is compatible with existing MK312BT control software including Buttplug.io and buttshock-py.

## Connection Sequence

### 1. Handshake

The host must first establish communication:

```
Host → Device: 0x00 (sent repeatedly, up to 12 attempts)
Device → Host: 0x07 (acknowledgment)
```

The handshake confirms the device is ready to receive commands.

### 2. Key Exchange

After handshake, establish encryption:

```
Host → Device: [0x2F, host_key, checksum]
Device → Host: [0x21, box_key, checksum]
```

Both sides calculate the encryption key:
```c
encryption_key = box_key ^ host_key ^ 0x55
```

The constant 0x55 is always part of the key derivation.

## Commands

### Read Memory (0x3C)

Reads a single byte from device memory.

**Request:**
```
[0x3C, addr_high, addr_low, checksum]
```

**Response:**
```
[0x22, data_byte, checksum]
```

**Example:** Read from 0x407B (current mode)
```
Send:    [0x3C, 0x40, 0x7B, 0xF7]
Receive: [0x22, 0x76, 0x98]  // Mode 0x76 (Waves)
```

### Write Memory (0x0D + length)

Writes 1-16 bytes to device memory.

**Command Byte Encoding:**
```
cmd_byte = 0x0D + ((total_length - 1) << 4)

Examples:
  Write 1 byte:  total_length = 5, cmd = 0x0D + 0x40 = 0x4D
  Write 2 bytes: total_length = 6, cmd = 0x0D + 0x50 = 0x5D
  Write 3 bytes: total_length = 7, cmd = 0x0D + 0x60 = 0x6D
```

**Request:**
```
[cmd_byte, addr_high, addr_low, data1, data2, ..., checksum]
```

**Response:**
```
[0x06]  // Success
[0x07]  // Error (invalid checksum or other issue)
```

**Example:** Write 0xFF to 0x40A5 (Channel A intensity)
```
Send:    [0x4D, 0x40, 0xA5, 0xFF, 0x91]
Receive: [0x06]
```

### Reset Device (0x08)

Resets the device to initial state.

**Request:**
```
[0x08]
```

**Response:**
```
[0x06]
```

## Memory Address Mapping

The protocol uses virtual addressing to access different memory regions:

### Flash ROM (Read-Only)
```
0x0000-0x00FF   Flash memory (device identification and strings)
  0x00FC        Box model         → returns 0x0C (MK-312BT identifier)
  0x00FD        Firmware ver major → returns 0x01
  0x00FE        Firmware ver minor → returns 0x06  (reports as v1.6)
  0x00FF        Firmware ver internal → returns 0x00
  All other addresses             → returns 0x00
```

Host software reads 0x00FC to confirm the device is an MK-312BT (expects 0x0C).

### RAM (Read/Write)
```
0x4000-0x43FF   Virtual RAM mapped to firmware state (serial_mem.c)

System:
  0x400F        Pot lockout flags (read/write)
  0x4064        Current level A (read-only, from ADC)
  0x4065        Current level B (read-only, from ADC)
  0x406D        Menu state (0x02 = running/inactive)
  0x4070        Box command register (write-only, executes commands)
  0x407B        Current mode (protocol encoding 0x76-0x8E)
  0x4083        Output control flags (phase/mute/stereo)

Channel A:
  0x4090        Channel A gate value (0-255)
  0x4098        Gate on-time
  0x4099        Gate off-time
  0x409A        Gate selection
  0x409C        Mode ramp counter A
  0x40A5-0x40A9 Channel A intensity (value/min/max/rate/select)
  0x40AE-0x40B2 Channel A frequency (value/min/max/rate/select)
  0x40B7-0x40BB Channel A width (value/min/max/rate/select)

Channel B:
  0x4190        Channel B gate value (0-255)
  0x419C        Mode ramp counter B
  0x41A5-0x41A9 Channel B intensity (value/min/max/rate/select)
  0x41AE-0x41B2 Channel B frequency (value/min/max/rate/select)
  0x41B7-0x41BB Channel B width (value/min/max/rate/select)

Config:
  0x41F3        Top mode (protocol encoding)
  0x41F4        Power level (0=Low, 1=Normal, 2=High)
  0x41F5        Split mode A (protocol encoding)
  0x41F6        Split mode B (protocol encoding)
  0x41F7        Favourite mode (protocol encoding)
  0x41F8-0x41FF Advanced parameters (ramp/depth/tempo/freq/effect/width/pace)
  0x420D        Multi-Adjust value (0-255)
  0x4213        Box key (write 0x00 to reset encryption for reconnect)
  0x4088-0x408B Routine timers (4 bytes)
```

Writing to 0x4070 executes box commands (mode select, LCD ops, etc).
See `MEMORY_MAP_REFERENCE.md` for the complete command table.

### EEPROM (Read/Write)
```
0x8000-0x81FF   Mapped to EEPROM via serial_mem.c

Key locations:
  0x8001        IsProvisioned (reads 0x55 = device is provisioned)
  0x8002        Box serial number low byte
  0x8003        Box serial number high byte
  0x8006        ELinkSig1 (reads 0x01)
  0x8007        ELinkSig2 (reads 0x01)
  0x8008        Top mode (current mode, protocol encoding)
  0x8009        Power level (0-2)
  0x800A        Split mode A
  0x800B        Split mode B
  0x800C        Favourite mode
  0x800D        Advanced: Ramp Level
  0x800E        Advanced: Ramp Time
  0x800F        Advanced: Depth
  0x8010        Advanced: Tempo
  0x8011        Advanced: Frequency
  0x8012        Advanced: Effect
  0x8013        Advanced: Width
  0x8014        Advanced: Pace

Reads reflect current runtime config. Writes update config immediately.
Unmapped EEPROM addresses pass through to raw EEPROM hardware.
```

## Checksum Calculation

Simple 8-bit checksum - sum all bytes except the checksum itself:

```c
uint8_t checksum = 0;
for (int i = 0; i < length - 1; i++) {
    checksum += data[i];
}
checksum = checksum & 0xFF;  // Modulo 256
```

## Encryption

### Key Derivation
```c
encryption_key = box_key ^ host_key ^ 0x55
```

### Encryption Rules
1. **Host -> Device**: ALL bytes are encrypted (including command byte)
2. **Device -> Host**: NO encryption (all replies sent in plaintext)

### Encryption Algorithm
```c
encrypted_byte = plaintext_byte ^ encryption_key
```

### Example
```
Plaintext:  [0x3C, 0x40, 0x7B, 0xF7]
Key: 0xA3
Encrypted:  [0x9F, 0xE3, 0xD8, 0x54]  // ALL bytes encrypted
            ^^^^  ^^^^  ^^^^  ^^^^
            |     |     |     |
            |     |     |     +-- 0xF7 ^ 0xA3 = 0x54
            |     |     +-------- 0x7B ^ 0xA3 = 0xD8
            |     +-------------- 0x40 ^ 0xA3 = 0xE3
            +-------------------- 0x3C ^ 0xA3 = 0x9F
```

### Reconnection
To allow reconnection, hosts should write 0x00 to address 0x4213 (BoxKey)
before disconnecting. This resets the encryption state so a fresh handshake
can succeed.

## Protocol State Machine

The implementation uses a state machine to handle variable-length commands:

```
State: IDLE
  ↓ Receive command byte
State: RECEIVING
  ↓ Receive remaining bytes based on command type
State: COMPLETE
  ↓ Verify checksum
  ↓ Execute command
  ↓ Send response
State: IDLE (reset)
```

## Error Handling

### Checksum Failure
- Device responds with 0x07 (error)
- Client should retry the command

### Unknown Command
- Ignored, no response sent
- State machine resets to idle

### Timeout
- No response after 100ms indicates communication failure
- Client should retry handshake sequence

## Implementation Notes

### Buffer Management
- Maximum packet size: 20 bytes (1 cmd + 2 addr + 16 data + 1 checksum)
- RX buffer size: 16 bytes
- Statically allocated, no dynamic memory

### Performance
- Baud rate: 19200 (standard MK312BT)
- Byte transmission time: ~0.52ms per byte
- Maximum write (16 bytes): ~10.4ms
- Typical read command: ~2.1ms

### Thread Safety
- Serial processing called from main loop
- No interrupt-based receive (polling mode)
- No concurrent access issues

## Testing with Python

Example using pyserial:

```python
import serial
import struct

ser = serial.Serial('/dev/ttyUSB0', 19200, timeout=1)

# Handshake
for _ in range(12):
    ser.write(b'\x00')
    if ser.read(1) == b'\x07':
        break

# Key exchange
host_key = 0x00
packet = bytes([0x2F, host_key, 0x2F])  # Simple checksum
ser.write(packet)
response = ser.read(3)
box_key = response[1]
encryption_key = box_key ^ host_key ^ 0x55

# Read current mode (all bytes encrypted after key exchange)
plaintext = bytes([0x3C, 0x40, 0x7B])
checksum = sum(plaintext) & 0xFF
packet = bytes([b ^ encryption_key for b in plaintext + bytes([checksum])])
ser.write(packet)
response = ser.read(3)  # Reply is plaintext
mode = response[1]
print(f"Current mode: 0x{mode:02X}")
```

## Compatibility

This implementation is compatible with:
- **Buttplug.io**: Device control library
- **buttshock-py**: Python MK312BT control library
- **EroScripts**: Community scripts and patterns
- **Custom control software**: Any software implementing the MK312BT protocol

## References

- **C# Reference Implementation**: See `Commands.cs`, `Protocol.cs`, `MK312Constants.cs`
- **Memory Map**: See `MEMORY_MAP_REFERENCE.md`
- **Reverse Engineering**: See `REVERSE_ENGINEERING_FINDINGS.md`
- **buttshock Protocol Docs**: https://github.com/Orgasmscience/buttshock-protocol-docs
