# MK-312BT Complete Memory Map Reference

Source: [erosoutsider et312-protocol.org](https://github.com/Zefiro/erosoutsider/blob/master/doc/et312-protocol.org)

---

## Memory Organization

The MK-312BT uses a **virtual memory space** for serial protocol access:

| Region | Start | End | Size | Access |
|--------|-------|-----|------|--------|
| Flash Strings | `$0000` | `$00FF` | 256 B | Read-only |
| RAM/Registers | `$4000` | `$43FF` | 1024 B | Read/Write |
| EEPROM | `$8000` | `$81FF` | 512 B | Read/Write |

---

## Flash Region (`$0000–$00FF`)

### Firmware Information
- `$0000–$000F` - Box model string (e.g., "ET312B")
- `$0010–$001F` - Firmware version
- `$0020–$00FF` - LCD strings, mode names, messages

**Access:** Read-only via serial protocol

---

## RAM/Register Region (`$4000–$43FF`)

### System State (`$4000–$4082`)

| Address | Name | Type | Description |
|---------|------|------|-------------|
| `$400F` | POT_LOCKOUT_FLAGS | Flags | Front panel pot lockout control |
| `$4070` | BOX_COMMAND | Command | Execute command (write-only) |
| `$4083` | COMM_CONTROL_FLAG | Flags | Phase, mute, mono/stereo control |

### Current Output Levels

| Address | Name | Range | Description |
|---------|------|-------|-------------|
| `$4064` | CURRENT_LEVEL_A | 0-255 | Channel A output current |
| `$4065` | CURRENT_LEVEL_B | 0-255 | Channel B output current |
| `$420D` | MULTI_ADJUST | 0-255 | Multi-adjust knob value |

### Gate Control

| Address | Name | Range | Description |
|---------|------|-------|-------------|
| `$4090` | CHANNEL_A_GATE | 0-255 | Channel A gate value |
| `$4098` | GATE_ON_TIME | 0-255 | Gate on duration |
| `$4099` | GATE_OFF_TIME | 0-255 | Gate off duration |
| `$409A` | GATE_SELECT | Flags | Gate selection (A/B/Both) |
| `$4190` | CHANNEL_B_GATE | 0-255 | Channel B gate value |

### Mode Control

| Address | Name | Range | Description |
|---------|------|-------|-------------|
| `$407B` | CURRENT_MODE | 0x76-0x8E | Current mode number |
| `$409C` | MODE_RAMP_A | 0-255 | Channel A mode ramp counter |
| `$419C` | MODE_RAMP_B | 0-255 | Channel B mode ramp counter |

### Channel A Modulation (`$409C–$40BF`)

Each parameter group has 9 consecutive registers: value, min, max, rate, step, action_min, action_max, select, timer.

#### Ramp Group (`$409C`)
| Address | Name | Range | Description |
|---------|------|-------|-------------|
| `$409C` | A_RAMP_VALUE | 0-255 | Current ramp value |
| `$409D` | A_RAMP_MIN | 0-255 | Ramp minimum |
| `$409E` | A_RAMP_MAX | 0-255 | Ramp maximum |
| `$409F` | A_RAMP_RATE | 0-255 | Ticks per step |
| `$40A0` | A_RAMP_STEP | 0-255 | Value change per step |
| `$40A1` | A_RAMP_ACTION_MIN | 0-255 | Action at min boundary |
| `$40A2` | A_RAMP_ACTION_MAX | 0-255 | Action at max boundary |
| `$40A3` | A_RAMP_SELECT | Flags | Timer rate and source select |
| `$40A4` | A_RAMP_TIMER | 0-255 | Tick counter accumulator |

#### Intensity Group (`$40A5`)
| Address | Name | Range | Description |
|---------|------|-------|-------------|
| `$40A5` | A_INTENSITY_VALUE | 0-255 | Current intensity value |
| `$40A6` | A_INTENSITY_MIN | 0-255 | Minimum intensity |
| `$40A7` | A_INTENSITY_MAX | 0-255 | Maximum intensity |
| `$40A8` | A_INTENSITY_RATE | 0-255 | Ticks per step |
| `$40A9` | A_INTENSITY_STEP | 0-255 | Value change per step |
| `$40AA` | A_INTENSITY_ACTION_MIN | 0-255 | Action at min boundary |
| `$40AB` | A_INTENSITY_ACTION_MAX | 0-255 | Action at max boundary |
| `$40AC` | A_INTENSITY_SELECT | Flags | Timer rate and source select |
| `$40AD` | A_INTENSITY_TIMER | 0-255 | Tick counter accumulator |

#### Frequency Group (`$40AE`)
| Address | Name | Range | Description |
|---------|------|-------|-------------|
| `$40AE` | A_FREQUENCY_VALUE | 0-255 | Current frequency value |
| `$40AF` | A_FREQUENCY_MIN | 0-255 | Minimum frequency |
| `$40B0` | A_FREQUENCY_MAX | 0-255 | Maximum frequency |
| `$40B1` | A_FREQUENCY_RATE | 0-255 | Ticks per step |
| `$40B2` | A_FREQUENCY_STEP | 0-255 | Value change per step |
| `$40B3` | A_FREQUENCY_ACTION_MIN | 0-255 | Action at min boundary |
| `$40B4` | A_FREQUENCY_ACTION_MAX | 0-255 | Action at max boundary |
| `$40B5` | A_FREQUENCY_SELECT | Flags | Timer rate and source select |
| `$40B6` | A_FREQUENCY_TIMER | 0-255 | Tick counter accumulator |

#### Width Group (`$40B7`)
| Address | Name | Range | Description |
|---------|------|-------|-------------|
| `$40B7` | A_WIDTH_VALUE | 0-255 | Current pulse width value |
| `$40B8` | A_WIDTH_MIN | 0-255 | Minimum width |
| `$40B9` | A_WIDTH_MAX | 0-255 | Maximum width |
| `$40BA` | A_WIDTH_RATE | 0-255 | Ticks per step |
| `$40BB` | A_WIDTH_STEP | 0-255 | Value change per step |
| `$40BC` | A_WIDTH_ACTION_MIN | 0-255 | Action at min boundary |
| `$40BD` | A_WIDTH_ACTION_MAX | 0-255 | Action at max boundary |
| `$40BE` | A_WIDTH_SELECT | Flags | Timer rate and source select |
| `$40BF` | A_WIDTH_TIMER | 0-255 | Tick counter accumulator |

### Select Byte Encoding

The `select` byte in each group encodes both timer rate and value source:

| Bits | Value | Meaning |
|------|-------|---------|
| 1:0 (timer) | `0x00` | No timer (static source) |
| 1:0 (timer) | `0x01` | Timer ~244 Hz |
| 1:0 (timer) | `0x02` | Timer ~30 Hz |
| 1:0 (timer) | `0x03` | Timer ~1 Hz |
| 4:2 (static src, when timer=0) | `0x00` | STATIC (value unchanged) |
| 4:2 (static src, when timer=0) | `0x04` | ADV_PARAM (tracks advanced setting) |
| 4:2 (static src, when timer=0) | `0x08` | MA_KNOB (tracks MA knob, scaled to ma_range) |
| 4:2 (min src, when timer≠0) | index 2 | MA sets sweep minimum each tick |
| 7:5 (rate src, when timer≠0) | index 2 | MA scales sweep rate each tick |

### Action Code Values

| Value | Name | Behavior |
|-------|------|----------|
| `$FF` | REVERSE | Swap min/max, swap actions, continue |
| `$FE` | REV_TOGGLE | Reverse + toggle gate polarity (GATE_ALT_POL) |
| `$FD` | LOOP | Wrap to other boundary |
| `$FC` | STOP | Set select timer bits = 0, freeze value |
| `$00-$DB` | MODULE_n | Execute module number n |

### Channel B Modulation (`$419C–$41BF`)

**Pattern:** Channel B addresses = Channel A addresses + `$0100`

All four groups (ramp/intensity/frequency/width) mirror the Channel A layout with the same 9-field structure at base + `$0100`.

### Power Configuration

| Address | Name | Range | Description |
|---------|------|-------|-------------|
| `$41F4` | POWER_LEVEL | 0-2 | 0=Low, 1=Normal, 2=High |

### Communication Buffer (`$0220–$024B`)

| Address | Name | Size | Description |
|---------|------|------|-------------|
| `$0220` | COMM_BUFFER | 44 bytes | Serial communication data buffer |
| `$022C` | COMM_COUNTER | 1 byte | Buffer position counter |

### Timing

| Address | Name | Size | Description |
|---------|------|------|-------------|
| `$0088` | ROUTINE_TIMER | 4 bytes | System routine timers |

---

## EEPROM Region (`$8000–$81FF`)

### Basic Configuration

| Address | Name | Range | Description |
|---------|------|-------|-------------|
| `$8000` | TOP_MODE | 0x76-0x8E | Last selected mode |
| `$8001` | FAVORITE_MODE | 0x76-0x8E | Favorite mode quick access |
| `$8002` | POWER_LEVEL | 0-2 | Saved power level setting |

### Advanced Parameters (`$800D–$8014`)

| Address | Name | Range | Description |
|---------|------|-------|-------------|
| `$800D` | ADV_RAMP_LEVEL | 0-255 | Ramp level parameter |
| `$800E` | ADV_RAMP_TIME | 0-255 | Ramp time parameter |
| `$800F` | ADV_DEPTH | 0-255 | Depth parameter |
| `$8010` | ADV_TEMPO | 0-255 | Tempo parameter |
| `$8011` | ADV_FREQUENCY | 0-255 | Frequency parameter |
| `$8012` | ADV_EFFECT | 0-255 | Effect parameter |
| `$8013` | ADV_WIDTH | 0-255 | Width parameter |
| `$8014` | ADV_PACE | 0-255 | Pace parameter |

---

## Command Codes (`$4070` Write Operations)

Writing to address `$4070` executes system commands (implemented in `serial_mem.c`):

| Code | Function | Description |
|------|----------|-------------|
| `$00` | RESET_ROUTINE | Re-initialize and restart current mode |
| `$10` | MODE_NEXT | Increment mode index, restart mode |
| `$11` | MODE_PREV | Decrement mode index, restart mode |
| `$12` | MODE_REFRESH | Restart current mode without changing index |

Note: The original MK-312BT firmware had a larger command table. This reimplementation implements the minimum needed for serial protocol compatibility with Buttplug.io and buttshock-py. Other command codes (display control, channel increment, etc.) are ignored.

---

## Mode Numbers

| Value | Mode Name | Description |
|-------|-----------|-------------|
| `$76` | Waves | Smooth sinusoidal waves |
| `$77` | Stroke | Pulsing stroke pattern |
| `$78` | Climb | Gradually increasing intensity |
| `$79` | Combo | Combination of patterns |
| `$7A` | Intense | High intensity pulses |
| `$7B` | Rhythm | Rhythmic patterns |
| `$7C` | Audio 1 | Audio responsive mode 1 |
| `$7D` | Audio 2 | Audio responsive mode 2 |
| `$7E` | Audio 3 | Audio responsive mode 3 |
| `$7F` | Random1 | Random pattern selection 1 |
| `$80` | Random2 | Random pattern selection 2 |
| `$81` | Toggle | Toggle/switching pattern |
| `$82` | Orgasm | Progressive intensity build |
| `$83` | Torment | Intense variable pattern |
| `$84` | Phase 1 | Phase-shifted pattern 1 |
| `$85` | Phase 2 | Phase-shifted pattern 2 |
| `$86` | Phase 3 | Phase-shifted pattern 3 |
| `$87` | User 1 | User-programmable mode 1 |
| `$88` | User 2 | User-programmable mode 2 |
| `$89` | User 3 | User-programmable mode 3 |
| `$8A` | User 4 | User-programmable mode 4 |
| `$8B` | User 5 | User-programmable mode 5 |
| `$8C` | User 6 | User-programmable mode 6 |
| `$8D` | User 7 | User-programmable mode 7 |
| `$8E` | Split | Independent A/B control |

---

## Serial Protocol

### Read Operation
```
Send: [READ_CMD, addr_high, addr_low]
Receive: [value]
```

### Write Operation
```
Send: [WRITE_CMD, addr_high, addr_low, value]
Receive: [ACK]
```

### Encryption

After key exchange, all bytes from host to device are XOR-encrypted with a **static session key**:

```
session_key = box_key ^ host_key ^ 0x55
```

- `box_key` is provided by the device during key exchange (derived from a timer counter at connection time)
- `host_key` is chosen by the host and sent during key exchange
- The session key is constant for the duration of the connection (not rolling)
- Device-to-host messages are **not encrypted**

Example key exchange:
```
Host → Device:  [0x00]               (sync byte, unencrypted)
Device → Host:  [0x07]               (ready)
Host → Device:  [0x2F, host_key, checksum]
Device → Host:  [0x21, box_key, checksum]
session_key = box_key ^ host_key ^ 0x55
```

Subsequent commands from host: each byte XOR'd with `session_key`.

### Protocol Commands
- `$3C` - READ (read 1 byte from address)
- `$0D` (+length nibble) - WRITE (write 1-8 bytes to address)
- `$2F` - KEY_EXCHANGE
- `$08` - RESET

---

## Implementation Notes

### Virtual Address Translation
For serial protocol compatibility, translate virtual addresses to SRAM/EEPROM:

```c
uint8_t read_virtual_address(uint16_t addr) {
    if (addr >= 0x8000 && addr < 0x8200) {
        // EEPROM region
        return eeprom_read_byte((uint8_t*)(addr - 0x8000));
    }
    else if (addr >= 0x4000 && addr < 0x4400) {
        // RAM/Register region
        return read_register(addr - 0x4000);
    }
    else if (addr >= 0x0000 && addr < 0x0100) {
        // Flash string region
        return pgm_read_byte((uint8_t*)addr);
    }
    return 0x00;
}

void write_virtual_address(uint16_t addr, uint8_t value) {
    if (addr >= 0x8000 && addr < 0x8200) {
        // EEPROM region
        eeprom_write_byte((uint8_t*)(addr - 0x8000), value);
    }
    else if (addr >= 0x4000 && addr < 0x4400) {
        // RAM/Register region
        write_register(addr - 0x4000, value);
    }
    // Flash region is read-only
}
```

### Register Read/Write Handlers
Map virtual register addresses to actual variables:

```c
uint8_t read_register(uint16_t offset) {
    switch (offset) {
        case 0x0064: return current_level_a;
        case 0x0065: return current_level_b;
        case 0x0090: return channel_a_gate;
        case 0x0190: return channel_b_gate;
        case 0x00A5: return channel_a_intensity_value;
        case 0x01A5: return channel_b_intensity_value;
        // ... etc
        default: return 0x00;
    }
}

void write_register(uint16_t offset, uint8_t value) {
    switch (offset) {
        case 0x0064: current_level_a = value; break;
        case 0x0065: current_level_b = value; break;
        case 0x0070: execute_command(value); break;
        case 0x0090: channel_a_gate = value; break;
        case 0x0190: channel_b_gate = value; break;
        // ... etc
    }
}
```

---

## References

- **erosoutsider:** https://github.com/Zefiro/erosoutsider
- **buttshock-et312-firmware:** https://github.com/teledildonics/buttshock-et312-firmware
- **buttshock-protocol-docs:** https://github.com/Orgasmscience/buttshock-protocol-docs
- **MK-312 BT:** https://github.com/CrashOverride85/mk312-bt
