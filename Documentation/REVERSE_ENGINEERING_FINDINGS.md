# MK-312BT Reverse Engineering Findings

## Sources
- [erosoutsider Documentation](https://github.com/Zefiro/erosoutsider) - Complete memory map and protocol
- [buttshock-et312-firmware](https://github.com/teledildonics/buttshock-et312-firmware) - Disassembly tools and annotations
- [buttshock-protocol-docs](https://github.com/Orgasmscience/buttshock-protocol-docs) - Serial protocol documentation
- [MK-312 BT Project](https://github.com/CrashOverride85/mk312-bt) - Hardware clone implementation
- Firmware dump: 312-M006_Bootloader.txt (strings extracted)

---

## Official Memory Map (from erosoutsider)

### Memory Regions

| Region | Address Range | Size | Purpose |
|--------|---------------|------|---------|
| Flash | `$0000–$00FF` | 256 bytes | Firmware strings, version info |
| RAM/Registers | `$4000–$43FF` | 1KB | CPU registers, I/O, system state |
| EEPROM | `$8000–$81FF` | 512 bytes | Persistent storage |

### Key Register Addresses

#### System Control
- `$400F` - Front panel potentiometer lockout flags
- `$4070` - Box command execution
- `$4083` - Phase, mute/mono/stereo, switch control

#### Current Levels
- `$4064` - Current Level A (0–255)
- `$4065` - Current Level B (0–255)
- `$420D` - Multi Adjust value

#### Channel A Modulation (`$40A5–$40BE`)
- `$40A5` - Intensity: value
- `$40A6` - Intensity: minimum
- `$40A7` - Intensity: maximum
- `$40A8` - Intensity: rate
- `$40A9` - Intensity: select
- `$40AE` - Frequency: value
- `$40AF` - Frequency: minimum
- `$40B0` - Frequency: maximum
- `$40B1` - Frequency: rate
- `$40B2` - Frequency: select
- `$40B7` - Width: value
- `$40B8` - Width: minimum
- `$40B9` - Width: maximum
- `$40BA` - Width: rate
- `$40BB` - Width: select

#### Channel B Modulation (`$41A5–$41BE`)
Mirrors Channel A structure at offset +0x0100

#### Gate Control
- `$4090` - Channel A Gate
- `$4098` - Gate on-time
- `$4099` - Gate off-time
- `$409A` - Gate selection (A/B/A+B)
- `$4190` - Channel B Gate

#### Mode Selection
- `$407B` - Current mode number
- `$409C` - Mode ramp counter (Channel A)
- `$419C` - Mode ramp counter (Channel B)
- `$41F4` - Power level (LOW/NORMAL/HIGH)

#### Communication
- `$0220–$024B` - Communication buffer (44 bytes)
- `$022C` - Counter for communication buffer
- `$0083` - COMM_CONTROL_FLAG

#### Timers
- `$0088–$008B` - Routine timers (4 bytes)

### EEPROM Layout

#### Basic Configuration
- `$8000` (`0x00`) - Top mode (last selected)
- `$8001` (`0x01`) - Favorite mode
- `$8002` (`0x02`) - Power level

#### Advanced Parameters (`$800D–$8014`)
- Ramp level
- Ramp time
- Depth
- Tempo
- Frequency
- Effect
- Width
- Pace

---

## Comparison: Official vs Current Implementation

### ✅ CORRECT

1. **EEPROM Configuration**
   - Your code: `EEPROM_ADDR_TOP_MODE 0x00` ✅
   - Your code: `EEPROM_ADDR_FAVORITE 0x01` ✅
   - Your code: `EEPROM_ADDR_POWER_LEVEL 0x02` ✅

2. **Hardware Constants**
   - F_CPU: 8MHz ✅
   - USART baud rate: 19200 ✅
   - Timer1 prescaler: 8 ✅
   - ADC prescaler: 128 ✅

3. **Protocol Constants**
   - WRITE: 0x0E ✅
   - READ: 0x0F ✅
   - LINK_ON: 0x0C ✅
   - LINK_OFF: 0x0D ✅
   - HEARTBEAT: 0x08 ✅
   - CRC polynomial: 0x1021 ✅

4. **Pin Definitions**
   - All ADC channel assignments correct ✅
   - SPI pins correct ✅
   - LCD pins correct ✅
   - Button pins correct ✅
   - DAC CS pin correct ✅

5. **Parameter Limits**
   - MAX_INTENSITY: 255 ✅
   - MAX_FREQUENCY: 9 ✅
   - MAX_WIDTH: 50 ✅

### ⚠️ NEEDS VERIFICATION

1. **Memory Mapping**
   - Your implementation uses simple C structs in Arduino SRAM
   - Original uses **virtual addressing** (`$4000–$43FF` region)
   - Your approach is functionally equivalent for a reimplementation ✅
   - But serial protocol expects addresses like `$4064` not `0x0064`

2. **Channel A/B Offset Pattern**
   - Original: Channel B = Channel A + 0x100
   - Example: `$4090` (Ch A Gate) → `$4190` (Ch B Gate)
   - Your struct layout doesn't enforce this pattern
   - Matters for serial protocol compatibility ⚠️

3. **Communication Buffer**
   - Your code: 44 bytes + counter ✅
   - Should be at address `$0220–$024B` in virtual space
   - Functional but addressing may differ

---

## Command Table (`$4070` Execution)

From erosoutsider documentation, when writing to address `$4070`, these commands execute:

| Code | Function |
|------|----------|
| 0x00 | Reset routine |
| 0x02 | Display status |
| 0x03 | Select menu |
| 0x05 | Start favorite mode |
| 0x07 | Edit parameters |
| 0x08-0x11 | Mode navigation |
| 0x13-0x15 | LCD operations |
| 0x19-0x1B | Channel operations |
| 0x21 | Ramp control |

**Status:** Not found in your current implementation. This is likely part of the serial protocol handler.

---

## Mode Numbers

From firmware dump strings, the mode order is:
1. Waves
2. Stroke
3. Climb
4. Combo
5. Intense
6. Rhythm
7. Audio 1
8. Audio 2
9. Audio 3
10. Random1
11. Random2
12. Toggle
13. Orgasm
14. Torment
15. Phase 1
16. Phase 2
17. Phase 3
18. User 1
19. User 2
20. User 3
21. User 4
22. User 5
23. User 6
24. User 7
25. Split

Your implementation includes these modes ✅

---

## Firmware Strings (Extracted from Dump)

### Boot Messages
- "Shut Off Power!!"
- "SelfTest OK v1.7"
- "Initializing..."
- "MK-312 T"
- "Press Any Key..."

### Menu Prompts
- "~ Selects Mode"
- "Press ~or OK"
- "Start Ramp Up?"
- "Set Split Mode?"
- "Set As Favorite?"
- "Set Pwr Level?"
- "Link Slave Unit?"
- "Save Settings?"
- "Reset Settings?"
- "Adjust Advanced?"

### Parameters
- "RampLevl"
- "RampTime"
- "Depth"
- "Tempo"
- "Freq."
- "Effect"
- "Width"
- "Pace"

### Status Messages
- "Battery:"
- "Failure"
- "Pwr Lev:"
- "Adjust?"
- "Menus:"
- "Low"
- "Normal"
- "High"
- "Saved!"
- "Linked!"
- "Master"
- "Slave"
- "Ramping"

**Status:** Menu system implemented in `menu.c/h` ✅

---

## CallTable Functions

From buttshock-et312-firmware annotations, CallTable is an array of 40 function pointers at flash address `0x1c70-0x1cc0`.

Your implementation has these functions defined in `call_table.c` ✅

The functions match expected behavior based on disassembly analysis.

---

## Hardware Architecture

### ATMega16A Specifications
- Flash: 16KB (bootloader uses 512 bytes → 15,872 for firmware)
- SRAM: 1KB
- EEPROM: 512 bytes
- Clock: 8 MHz
- Harvard architecture (separate program/data memory)

### Peripherals Used
- USART (serial @ 19200 baud)
- SPI (DAC communication for intensity control)
- ADC (6 channels for pots/audio)
- Timer1 (Channel A biphasic pulse generation, CTC mode /8)
- Timer2 (Channel B biphasic pulse generation, CTC mode /8)
- Watchdog (2s timeout)

All correctly configured in implementation.

---

## Serial Protocol Encryption

### Algorithm (from erosoutsider docs)
```c
// Transmit encryption
key = 0x00;
for each byte:
    encrypted = plaintext ^ key
    transmit(encrypted)
    key = (key + 0x55) & 0xFF

// Receive decryption
key = 0x00;
for each byte:
    plaintext = received ^ key
    key = (key + 0x55) & 0xFF
```

**Status:** Implemented in `serial.c` ✅

---

## Program Blocks / Bytecode

### Known Information
- Located at flash address `$2000–$21c8`
- Uses bytecode interpreter (CallTable function 30)
- 24 predefined modes + 7 user modes

### Documented Information
✅ Bytecode opcode set documented in `MK312BT_INSTRUCTION_SET.md`
✅ Instruction format fully specified (END/COPY/SET/ADD/RAND/STORE opcodes)
✅ Execution model: inline execution from PROGMEM via `mode_dispatcher.c`

All 25 modes (Waves through Split, plus User1-7) use inline bytecode execution in `mode_dispatcher.c`. Mode programs write channel-block fields (select bytes, min/max, rate, step, action codes) which `param_engine_tick()` then processes each ~244 Hz master tick to drive parameter modulation.

**Parameter Engine Select Byte (`0x04` = ADV_PARAM source):**
- When `freq_select = 0x04`, the frequency value tracks `adv_frequency` (Advanced F setting) each tick
- When `width_select = 0x04`, the width value tracks `adv_width` (Advanced W setting) each tick
- Modes Audio 1, Audio 2, Phase 1-3 use `freq_select = 0x04` so the F setting applies in real time
- This matches the original MK-312BT behavior where changing the F advanced setting affects Audio/Phase modes immediately

---

## Recommendations for Improvement

### 1. High Priority - Hardware Testing
- Test on hardware with oscilloscope to verify biphasic pulse waveforms
- Verify Timer1/Timer2 ISR timing accuracy
- Confirm dead-time is sufficient to prevent FET shoot-through
- Compare pulse frequency/width with real MK-312BT output

### 2. ~~Medium Priority - Serial Protocol Enhancements~~ DONE
- ~~Virtual memory addressing `$4000-$43FF` region for full compatibility~~ Implemented in `serial_mem.c`
- ~~Command execution at address `$4070`~~ Implemented in `serial_mem.c`
- ~~Proper register read/write handlers mapping to MK312BTState~~ Implemented in `serial_mem.c`

### 3. Low Priority - Bytecode Extraction
To get **exact** program blocks:
- Use `avrdisas` tool from buttshock repository
- Extract bytecode from flash region `$2000-$21c8`
- Decode instruction sequences
- Replace representative programs with exact ones

---

## Conclusion

The implementation is **functionally accurate** and well-structured. The core systems (parameter engine, mode dispatcher, timer-driven pulse generation, DAC intensity control) are complete.

**Implemented:**
- Hardware configuration and pin assignments
- Timer-driven biphasic pulse generation (Timer1/Timer2 ISR state machines)
- DAC intensity control via SPI (main loop)
- Serial protocol encryption
- EEPROM layout and persistence
- Menu system (LCD UI) with mode selection and parameter adjustment
- Parameter engine with advanced settings and MA knob support
- Mode structure (all 25 modes)
- Virtual memory addressing for serial protocol (serial_mem.c)
- Box command execution via `$4070` writes (mode select, LCD ops)
- EEPROM read/write via serial protocol (`$8000-$81FF`)
- Device identification: box model (`$00FC`=0x0C) and firmware version (`$00FD-$00FF`)

**Remaining work:**
- Exact bytecode programs (using representative versions for user modes)

**Next steps:**
1. Test on hardware - verify biphasic waveform generation on oscilloscope
2. Compare pulse timing with real MK-312BT output
3. Test serial protocol compatibility with Buttplug.io / buttshock-py
4. Fine-tune mode behavior parameters against known reference settings
