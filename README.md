# MK-312BT Firmware - Accurate C Reimplementation

This project contains an accurate C reimplementation of the MK-312BT firmware for the ATmega16 microcontroller, based on disassembly analysis of the original binary.

**Note:** This project is designed for Arduino IDE compilation with MightyCore board support.

## Project Structure

### Arduino Files
```
MK312BT.ino                   - Main Arduino sketch (setup, main loop, hardware init)
Button.h                    - Button debouncing library
MK312BT_Memory.h              - Memory structures and state (MK312BTState)
MK312BT_Constants.h           - System constants and pin definitions
MK312BT_Modes.h               - Mode enumerations (MK312BTMode enum)
MK312BT_Utils.h               - Utility functions
```

### Pulse Generation & Output
```
pulse_gen.c/h               - Timer-driven biphasic pulse generator
interrupts.c                - ISR implementations (Timer1/Timer2 H-bridge state machines)
dac.c/h                     - LTC1661 DAC control via SPI (intensity)
avr_registers.h             - AVR hardware register definitions
```

### Mode Engine & Parameter System
```
mode_programs.c/h           - Mode bytecode programs (all 25 modes)
mode_dispatcher.c/h         - Mode selection and inline bytecode execution
channel_mem.c/h             - ChannelBlock structs (modulation state for both channels)
param_engine.c/h            - Parameter modulation engine (select byte decode, MA/ADV_PARAM)
prng.c/h                    - Pseudo-random number generator
```

### Hardware Abstraction
```
adc.c/h                     - ADC multiplexing
lcd.c/h                     - LCD implementation (4-bit parallel)
serial.c/h                  - Serial protocol (READ/WRITE/LINK commands)
utils.c/h                   - Utilities
```

### Configuration & Menu System
```
config.c/h                  - System configuration management
eeprom.c/h                  - EEPROM persistence with checksums
menu.c/h                    - Menu system (mode selection, parameter adjustment)
audio_processor.c/h         - Audio input processing and envelope following
```

## Features Implemented

### Hardware Peripherals
- **Timer1**: CTC mode, /8 prescaler (1 MHz). Channel A H-bridge pulse generation via CompA ISR
- **Timer2**: CTC mode, /8 prescaler (1 MHz). Channel B H-bridge pulse generation via Comp ISR
- **USART**: Serial communication with XOR encryption (19200 baud)
- **ADC**: Multiplexed analog inputs for potentiometers, audio, and battery monitoring
- **SPI**: LTC1661 dual DAC control (10-bit resolution, intensity/power)
- **External Interrupts**: INT0 and INT1 for audio digital inputs
- **LCD**: 4-bit parallel interface (16x2 character display)
- **EEPROM**: Configuration persistence (512 bytes)

### Core Functionality Based on Disassembly

#### Bytecode Engine (Inline Execution)
- Executes bytecode programs directly from PROGMEM
- Memory-optimized: Uses 13-byte compact memory structure instead of full 1024-byte address space
- Opcodes: COPY, ADD, SET, STORE, RAND, END (covers all 25 mode programs)
- Per-channel execution state

#### Mode Dispatcher (Function_0x604)
- Maps mode numbers (0x76-0x8E) to program blocks
- Handles split mode operation (independent A/B channels)
- Manages mode switching and initialization

#### Program Blocks
- 36 bytecode modules stored in PROGMEM (`mode_programs.c`)
- Modules run once on mode selection to configure parameter engine registers
- Module chaining: action codes at parameter boundaries trigger the next module automatically
- User-customizable modes (User1-User7) stored in EEPROM user program slots

#### Parameter Engine & Advanced Settings ✅
- **Channel Memory Blocks**: `ChannelBlock` struct (64 bytes × 2 channels) mirrors MK-312BT SRAM layout; holds all modulation state for ramp, intensity, frequency, and width groups
- **Select Byte Encoding**: Each parameter group has a `select` byte that encodes both timer rate and value source:
  - `0x00` = Static (value unchanged)
  - `0x04` = ADV_PARAM (value tracks the corresponding advanced setting each tick)
  - `0x08` = MA_KNOB (value tracks MA knob, scaled to ma_range_min..ma_range_max)
  - Timer bits `0x01/0x02/0x03` = sweep at 244 Hz / 30 Hz / 1 Hz
- **Parameter Ramping**: Timer-driven sweeps implement U-D (up-down bounce), U-U (sawtooth loop), and Stop patterns via action codes at min/max boundaries
- **Multi-Adjust (MA) Knob**: Mode-specific functions (sweep rate, direct value, phase control) encoded in select byte rate/min source fields
- **Advanced Settings**: 8 adjustable parameters stored in `system_config_t`:
  - Pace: Controls width pulsing rate (Combo mode, written to rate field by mode program)
  - Width: Sets pulse width via ADV_PARAM source for Climb, Intense, Toggle, Torment, Phase 3
  - Effect: Reserved for user routines
  - Frequency: Sets carrier frequency via ADV_PARAM for Audio 1, Audio 2, Phase 1-3 — updates live each tick
  - Tempo: Controls pulse rate for Phase 2 (written to rate field by mode program)
  - Depth: Controls level variation (Stroke, Phase 2)
  - Ramp Level/Time: Auto-ramp configuration
- **Real-Time ADV_PARAM Updates**: Modes using `select = 0x04` (ADV_PARAM) reflect advanced setting changes immediately — no mode restart required
- **EEPROM Storage**: All advanced settings persist across power cycles

#### Serial Protocol (MK312BT-Accurate) ✅
- **Handshake**: Send 0x00, expect 0x07
- **Key Exchange**: Command 0x2F establishes encryption key
- **Read Command**: 0x3C reads single byte from memory address
- **Write Command**: 0x0D + length nibble writes 1-16 bytes
- **Reset Command**: 0x08 resets device
- **Encryption**: XOR with `box_key ^ host_key ^ 0x55`
- **Checksum**: Simple 8-bit sum (mod 256)
- **Memory Mapping**: 0x4000-0x43FF (RAM), 0x8000-0x81FF (EEPROM), 0x0000-0x00FF (Flash)
- **Compatible with**: Buttplug.io, buttshock-py, and other MK312BT control software

#### CallTable System
- 40 function pointers at 0x1c70-0x1cc0
- Menu operations, display functions, mode switching
- CallTable_30 is the bytecode interpreter

#### Interrupt-Driven Pulse Generation
- Timer1 CompA: 5-phase state machine generates biphasic pulses on Channel A H-bridge (PB2/PB3)
- Timer2 Comp: 5-phase state machine generates biphasic pulses on Channel B H-bridge (PB0/PB1)
- USART RX: Drain receive buffer (prevents overrun)
- SPI STC: Drain SPI buffer (prevents overrun)
- DAC intensity updates are performed from the main loop, not from ISRs

## Startup Sequence

On power-up, the firmware performs the following initialization:

1. **Hardware Initialization**: Configure all peripherals (timers, USART, ADC, SPI, interrupts)
2. **LCD Initialization**: Initialize 4-bit LCD interface, backlight on
3. **Serial Initialization**: Configure USART and encryption state
4. **Load Configuration**: Read saved settings from EEPROM
5. **DAC Test** (`dacTest()`): Verify SPI communication and LTC1661 DAC output
   - If DAC test fails: displays "DAC FAIL - HALT / Check hardware" and halts
6. **FET Calibration** (`fetCalibrate()`): Verify H-bridge FET pairs are functional
   - Applies test pulses and measures output current for each FET
   - If calibration fails: displays "FET FAIL - HALT / Check hardware" and halts
7. **Initialization Progress Bar**: Displays "Initializine..." with a 16-step progress bar
8. **Startup Screen**: Displays:
   ```
           MK-312BT
   Selftest OK MK02
   ```
   (held for 1.5 seconds, then waits for any button press)
9. **Menu Startup**: Shows main menu (mode name + navigation hint)
10. **Select Mode**: Loads last saved mode from EEPROM
11. **Enable Interrupts**: `sei()` starts ISR-driven pulse generation
12. **Enable Watchdog**: Activates 2-second watchdog timer
13. **First Update**: Calls `mode_dispatcher_update()` to initialize output state

If DAC test fails:
```
DAC FAIL - HALT
Check hardware
```
(system halts — watchdog is not fed, triggering reset after 2 seconds if watchdog was enabled)

If FET calibration fails:
```
FET FAIL - HALT
Check hardware
```
(system halts)

Then transitions to main menu display after button press.

## Building the Firmware

### Prerequisites
- Arduino IDE 1.8.x or later
- ATmega16 board support (MightyCore)

### Installation
1. Install MightyCore board support:
   - Go to **File → Preferences**
   - Add to "Additional Board Manager URLs": `https://mcudude.github.io/MightyCore/package_MCUdude_MightyCore_index.json`
   - Go to **Tools → Board → Boards Manager**
   - Search for "MightyCore" and install

### Compilation & Upload
1. Open `MK312BT.ino` in Arduino IDE
2. Select **Tools → Board → MightyCore → ATmega16**
3. Select **Tools → Clock → 8 MHz external**
4. Select **Tools → Programmer** (e.g., USBasp, Arduino as ISP)
5. Click **Verify** to compile
6. Click **Upload** to flash

## Architecture from Disassembly Analysis

This reimplementation is based on detailed analysis of the original MK-312BT firmware disassembly (312-16-decrypted-combined.bin). Key architectural components identified:

### Interrupt Vector Table (IVT) at 0x0000-0x0050
- Reset vector at 0x0000
- IRQ0/IRQ1 for audio digital inputs
- Timer1 CompA/CompB for output timing
- Timer0 Overflow for system timing
- SPI transfer complete
- USART RX complete
- ADC conversion complete

### Main Loop at 0x0054-0x059E
- ADC reading and processing
- Button input handling
- Mode state machine
- Output channel updates
- Serial communication processing
- Uses registers r4-r14 for channel state

### Mode Selection System at 0x604-0x78A
- Function_0x604: Main mode dispatcher
- Maps mode numbers (0x76-0x8E) to program blocks
- Handles split mode, audio modes, random modes
- Initializes bytecode execution state

### CallTable at 0x1c70-0x1cc0
- 40 function pointers for operations
- Menu navigation functions
- Display update functions
- Mode switching functions
- **CallTable_30**: Bytecode interpreter (most critical)

### Program Blocks at 0x2000-0x21c8
- Binary pattern data for each mode
- Interpreted as bytecode by CallTable_30
- Defines waveforms, timing, and behavior

### String Tables at 0x1cc8-0x1f90
- LCD menu strings
- Mode names
- Error messages

### Serial Protocol at 0x1858-0x199C
- Encrypted master/slave communication
- Memory read/write commands
- Link establishment

### Bootloader at 0x3e7a-0x3ff8
- Firmware update capability
- CRC verification
- Self-programming (SPM instructions)

## Output Architecture

The MK-312BT output stage has two independent subsystems:

### 1. Pulse Generation (Timer-Driven H-Bridge)
Each channel has a dedicated hardware timer running a 5-phase biphasic pulse state machine:

- **Timer1** (16-bit) drives Channel A via PB2 (Gate+) and PB3 (Gate-)
- **Timer2** (8-bit) drives Channel B via PB0 (Gate+) and PB1 (Gate-)
- Both timers run in CTC mode at 1 MHz (F_CPU/8 prescaler)
- ISR phases: Positive -> DeadTime -> Negative -> DeadTime -> Gap -> repeat
- Dead time (4 us) prevents H-bridge shoot-through
- The main loop sets pulse parameters; timers run autonomously

### 2. Intensity Control (DAC via Main Loop)
The LTC1661 dual 10-bit DAC controls the power supply voltage that scales pulse amplitude:

- Main loop reads Level A/B potentiometers via ADC
- Calculates DAC value based on power base level and modulation factor
- Writes to DAC via SPI from the main loop (not from ISRs)
- DAC chip select on PD4 (active low)
- SPI clock divider /16
- Higher DAC value = lower output intensity (inverted: 1023 = off, 0 = max)

### Output Generation Flow
1. Mode/parameter engine updates gate, frequency, and width values
2. Main loop converts frequency index to period (us) and width index to pulse width (us)
3. Main loop calls `pulse_set_width_a/b()`, `pulse_set_frequency_a/b()`, `pulse_set_gate_a/b()`
4. Timer ISRs autonomously generate biphasic pulse trains at the configured rate
5. Main loop reads level pots via ADC and writes intensity to DAC via SPI
6. DAC output voltage controls the transformer drive level (overall power)

## Bytecode System Details

### Memory Optimization
To fit within ATmega16's 1024 bytes of RAM, the firmware uses a compact memory structure:
- **Original design**: 1024-byte full address space (wouldn't fit)
- **Optimized design**: 13-byte compact structure containing only actively used addresses
- **Memory saved**: 1011 bytes (from 145% RAM usage to 46%)

See `MEMORY_OPTIMIZATION.md` for detailed explanation.

### Opcodes
The inline bytecode executor implements MK-312BT instruction set patterns:

| Opcode Pattern | Name | Description |
|---------------|------|-------------|
| 0x00 | END | End of program block |
| 0x20-0x3F | COPY | Copy 1-7 bytes to memory address |
| 0x40-0x43 | STORE | Copy value to bank register |
| 0x4C-0x4F | RAND | Generate random value |
| 0x50-0x5F | ADD | Add value to memory address |
| 0x80-0xFF | SET | Set single byte at address |

### Program Execution
- Programs execute directly from PROGMEM (no RAM copy)
- Lightweight inline execution in `mode_dispatcher_update()`
- Compact 13-byte device memory structure
- All 25 mode programs supported

### Example Program (Waves Mode)
```c
0x28, 0xA5, 0x78, 0x04,     // COPY 2 bytes to 0xA5: intensity=120, freq=4
0x50, 0x90, 0x02,           // ADD 2 to gate A (increment)
0x00                        // END (loop)
```

See `MK312BT_INSTRUCTION_SET.md` for complete opcode reference.

## Technical Details

### Microcontroller Configuration
- **MCU**: ATmega16
- **Clock**: 8 MHz external crystal
- **USART**: 19200 baud, 8N1
- **Timer1**: CTC mode, /8 prescaler (1 MHz tick), Channel A pulse generation
- **Timer2**: CTC mode, /8 prescaler (1 MHz tick), Channel B pulse generation
- **ADC**: AVCC reference, /128 prescaler
- **SPI**: Master mode, clock/16 (DAC communication)
- **Watchdog**: 2-second timeout

### Memory Layout
- **Flash**: 16 KB (program memory)
- **SRAM**: 1 KB (0x0060-0x045F)
- **EEPROM**: 512 bytes
- **Bootloader**: 0x3E00-0x3FFF (if implemented)

### Communication Protocol
The firmware implements a serial protocol with:
- XOR-based cipher key encryption (`session_key = box_key ^ host_key ^ 0x55`)
- Command types: Read (0x3C), Write (0x0D + length nibble), Key Exchange (0x2F), Reset (0x08)
- 8-bit checksum (sum of all bytes mod 256)
- Memory regions: Flash 0x0000-0x00FF (read-only), RAM 0x4000-0x43FF, EEPROM 0x8000-0x81FF

### Interrupt Priorities (AVR hardware order)
1. INT0/INT1 - External audio inputs
2. TIMER2_COMP - Channel B H-bridge pulse generation
3. TIMER1_COMPA - Channel A H-bridge pulse generation
4. USART_RXC - Serial receive buffer drain
5. SPI_STC - SPI buffer drain

### Pin Mapping
The Arduino version uses the following pin configuration:

**PORTA (ADC Inputs):**
- PA7: Line In R (low-pass filtered)
- PA6: Line In L / Mic/Remote (low-pass filtered)
- PA5: Channel Level B
- PA4: Channel Level A
- PA3: 12V measurement
- PA2: V+ measurement
- PA1: Multi-Adj VR3G$1
- PA0: Output current measurement

**PORTB (Output Gates & SPI):**
- PB7: SCK (SPI clock to DAC)
- PB6: MISO (ISP)
- PB5: MOSI (SPI data to DAC)
- PB4: Not connected
- PB3: Output A Gate-
- PB2: Output A Gate+
- PB1: Output B Gate-
- PB0: Output B Gate+

**PORTC (LCD & Buttons - Multiplexed):**
- PC7: LCD_DB7 / Menu button (multiplexed)
- PC6: LCD_DB6 / Up button (multiplexed)
- PC5: LCD_DB5 / OK button (multiplexed)
- PC4: LCD_DB4 / Down button (multiplexed)
- PC3: LCD_RS
- PC2: LCD_E
- PC1: LCD_RW
- PC0: Multiplex switch (LOW=LCD mode, HIGH=Button mode)

**PORTD (Serial, LEDs, DAC CS):**
- PD7: LCD backlight cathode
- PD6: Output LED1
- PD5: Output LED2
- PD4: DAC CS/LD (LTC1661 chip select)
- PD3: Multi-Adj Line In R
- PD2: Multi-Adj Line In L / Mic/Remote
- PD1: TX (serial via MAX232)
- PD0: RX (serial via MAX232)

#### Button/LCD Multiplexing (PORTC)
The MK-312BT uses a clever pin-sharing design where PC7-PC4 are multiplexed between LCD data and button inputs:

**LCD Mode (PC0 = LOW):**
- PC7-PC4 configured as outputs
- Used for sending 4-bit LCD data

**Button Mode (PC0 = HIGH):**
- PC7-PC4 configured as inputs with pullup
- PC7: Menu button
- PC6: Up button
- PC5: OK button
- PC4: Down button

This multiplexing saves 4 pins on the microcontroller.

## Menu System

The firmware includes a comprehensive menu system accessible via four buttons:

### Button Layout
- **UP** (Pin 2): Navigate up, increase values
- **DOWN** (Pin 3): Navigate down, decrease values
- **MENU** (Pin 4): Enter advanced menu, return to main menu
- **OK** (Pin 5): Select/confirm, enter edit mode

### Main Menu
The main display shows:
- Current mode name
- Channel A intensity (highlighted if selected)
- Channel B intensity (highlighted if selected)

Quick access from main menu:
- **UP + OK**: Enter intensity adjustment
- **DOWN + OK**: Enter frequency adjustment
- **UP + DOWN**: Enter width adjustment
- **MENU**: Enter advanced menu
- **OK**: Enter mode selection

### Mode Selection
Navigate through all available modes:
- **UP/DOWN**: Browse modes
- **OK**: Select mode and return to main menu
- **MENU**: Cancel and return to main menu

### Parameter Adjustment
When adjusting intensity, frequency, or width:
- **UP**: Increase value
- **DOWN**: Decrease value
- **OK/MENU**: Save and return to main menu

### Advanced Menu
Access system settings:
1. **Toggle Channel**: Switch between Channel A and B for parameter editing
2. **Power Level**: Set Low/Normal/High power output
3. **Set Favorite**: Save current mode as favorite
4. **Reset Config**: Reset all settings to defaults

Navigation:
- **UP/DOWN**: Browse options
- **OK**: Select option
- **MENU**: Return to main menu

## Modes Available

The firmware supports the following operational modes:

1. **Waves** - Smooth sine wave modulation
2. **Stroke** - Rhythmic pulsing pattern
3. **Climb** - Gradually increasing intensity
4. **Combo** - Combined pattern mode
5. **Intense** - High-frequency stimulation
6. **Rhythm** - Musical beat patterns
7. **Audio1/2/3** - Audio-responsive modes
8. **Random1/2** - Randomized pattern selection
9. **Toggle** - On/off switching
10. **Orgasm** - Progressive intensity build
11. **Torment** - Intense variable pattern

## Implementation Notes

### Accuracy
This reimplementation was created by analyzing the disassembled binary (312-16-decrypted-combined.bin) and translating key structures to C:
- Interrupt vector table matches original
- Memory layout at 0x0060+ preserved
- Mode dispatcher logic (Function_0x604) accurately reproduced
- CallTable system with 40 function pointers
- Bytecode interpreter (CallTable_30) implements original opcodes
- Program blocks contain representative patterns

### Completion Status

**Complete - Core Functionality:**
- Timer-driven biphasic pulse generation with 5-phase ISR state machines
- Independent Timer1 (Ch A) and Timer2 (Ch B) H-bridge control
- Dead-time insertion prevents FET shoot-through
- DAC intensity control via SPI (LTC1661) from main loop
- Inline bytecode execution - optimized for ATmega16's 1KB RAM
- Mode dispatcher and mode selection system (all 25 modes)
- Parameter engine with U-D, U-U, Toggle ramping behaviors
- Serial protocol READ/WRITE/KEY EXCHANGE commands
- EEPROM configuration save/load with checksums
- Audio processing with envelope follower
- Unified configuration management system
- Menu system with mode selection and parameter adjustment
- LCD display and button input
- ADC multiplexing for all channels

**Optimizations from Original:**
- Compact 13-byte memory structure instead of 1024-byte address space
- Inline bytecode execution instead of separate interpreter module
- Functionally equivalent but memory-optimized for ATmega16

See `MEMORY_OPTIMIZATION.md` for RAM optimization details.

### Extensibility
To add or modify modes:
1. Create new program block in `program_blocks.c`
2. Add entry to `program_block_table[]`
3. Map mode number in mode_dispatcher.c
4. Define mode constant in `mode_dispatcher.h`

### Testing
Without actual MK-312BT hardware, testing options:
- AVR simulator (simavr)
- Logic analyzer on SPI/USART pins
- Oscilloscope on DAC outputs
- Serial protocol testing via USART

## Safety

This firmware controls electrostimulation hardware. Always follow proper safety protocols when testing or using this code with actual MK-312BT hardware:

- Never use above the waist
- Never use if you have a pacemaker or heart condition
- Always start with low intensity settings
- Never use on broken skin
- Use only approved electrodes designed for this purpose
- Disconnect immediately if you feel any discomfort
