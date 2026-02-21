# Documentation Index

Quick reference guide to all documentation files in this project.

---

## Start Here

**[README.md](README.md)** - Main project documentation
- Overview of the MK-312BT firmware implementation
- Hardware setup and pin mapping
- Build instructions for Arduino IDE
- Output architecture (pulse generation + DAC intensity)
- Complete feature list and mode descriptions

---

## Architecture & Implementation

**[ARCHITECTURE.md](ARCHITECTURE.md)** - System architecture overview
- Timer-driven biphasic pulse generation (Timer1/Timer2 ISR state machines)
- Output generation: H-bridge pulse gen + DAC intensity control
- Data flow and execution model

**[MEMORY_OPTIMIZATION.md](MEMORY_OPTIMIZATION.md)** - Critical RAM optimization
- How RAM usage was reduced from 145% to 46%
- Compact memory structure (13 bytes vs 1024 bytes)
- Inline bytecode execution strategy
- **Read this if you're wondering why the code fits in 1KB RAM**

**[MENU_SYSTEM.md](MENU_SYSTEM.md)** - Menu implementation
- Button handling and navigation
- Mode selection and parameter adjustment screens
- EEPROM persistence for settings
- Advanced settings menu (Pace, Width, Effect, Frequency, Tempo, Depth)

---

## Technical Reference

**[MK312BT_INSTRUCTION_SET.md](MK312BT_INSTRUCTION_SET.md)** - Bytecode instruction reference
- Complete opcode documentation
- Instruction encoding patterns
- Usage examples (used by User1-7 and Split modes)

**[MODE_PARAMETERS_REFERENCE.md](MODE_PARAMETERS_REFERENCE.md)** - Mode parameters and behavior
- Complete parameter tables for all 18 built-in modes
- Advanced settings impact matrix (Pace, Width, Effect, Frequency, Tempo, Depth)
- Multi-Adjust knob assignments per mode
- Detailed behavior notes and special cases
- **Essential for implementing accurate mode behavior**

**[ADVANCED_SETTINGS_IMPLEMENTATION.md](ADVANCED_SETTINGS_IMPLEMENTATION.md)** - Implementation guide
- Architecture and component hierarchy
- Advanced settings storage and persistence
- Parameter engine ramping behaviors (U-D, U-U, Toggle)
- Mode-specific implementations
- MA knob function mappings
- Testing recommendations and known limitations

**[MEMORY_MAP_REFERENCE.md](MEMORY_MAP_REFERENCE.md)** - Memory address mapping
- RAM layout (compact structure addresses)
- EEPROM configuration storage
- Register assignments

**[SERIAL_PROTOCOL.md](SERIAL_PROTOCOL.md)** - Serial communication protocol
- Command format (READ, WRITE, KEY EXCHANGE)
- XOR encryption algorithm
- Checksum calculation
- Compatible with existing MK-312BT control software

---

## Historical & Analysis

**[FIRMWARE_ANALYSIS.md](FIRMWARE_ANALYSIS.md)** - Original firmware analysis
- Disassembly findings from 312-M006.bin
- Behavior patterns identified
- Implementation approach: parameter engine + bytecode + timer-driven pulse gen

**[REVERSE_ENGINEERING_FINDINGS.md](REVERSE_ENGINEERING_FINDINGS.md)** - Reverse engineering notes
- How the original firmware was analyzed
- Key discoveries and insights
- Challenges encountered

---

## Quick Navigation by Task

### "I want to build the firmware"
-> Start with **README.md** (Building the Firmware section)

### "I want to understand the pulse generation"
-> Read **ARCHITECTURE.md** (Output Generation section)

### "I want to understand the RAM optimization"
-> Read **MEMORY_OPTIMIZATION.md**

### "I want to add a new mode"
-> Check **MODE_PARAMETERS_REFERENCE.md** for behavior details, then **mode_behavior.c** and **param_engine.c**

### "I want to understand how modes behave"
-> Read **MODE_PARAMETERS_REFERENCE.md**

### "I want to control it via serial"
-> Read **SERIAL_PROTOCOL.md**

### "I want to modify the menu"
-> Read **MENU_SYSTEM.md**

### "I want to understand the architecture"
-> Read **ARCHITECTURE.md** and **MEMORY_MAP_REFERENCE.md**

---

## Reference Files (Not Used in Build)

The `reference/` folder contains historical files that are not compiled or used in the current build:
- **`bytecode.c/h`** - Original full bytecode interpreter (replaced by inline execution for memory optimization)
- **`program_blocks_data.h`** - Raw firmware data extracted from 312-M006.bin (140 bytes per mode)
- **`extract_module_bytecode.py`** - Python tool for extracting bytecode from firmware
- **`extract_programs.py`** - Python tool for extracting program blocks

These files are preserved for reference, research, and historical purposes.

---

## Documentation Files Removed

The following documentation files were removed as obsolete:
- `NEXT_STEPS.md` - Old roadmap (tasks completed)
- `IMPLEMENTATION_STATUS.md` - Outdated status
- `MODE_SYSTEM_FIX.md` - Old fix notes
- `CHANNEL_INDEPENDENCE_FIX.md` - Superseded implementation
- `BYTECODE_CORRECTION.md` - Old bytecode notes
- `BYTECODE_MODE_SYSTEM.md` - Old architecture notes
- `FIRMWARE_EXTRACTION_SUMMARY.md` - Extraction process notes

All information from these files has been consolidated into the remaining documentation.
