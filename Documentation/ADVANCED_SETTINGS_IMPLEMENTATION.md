# Advanced Settings Implementation

This document describes the implementation of the advanced settings and parameter engine system based on the MK-312BT mode parameters reference documentation.

---

## Overview

The advanced settings system implements the sophisticated parameter control behaviors documented in `MODE_PARAMETERS_REFERENCE.md`. This includes:

1. **Advanced settings** that affect specific modes
2. **Multi-Adjust (MA) knob** mode-specific functions
3. **Parameter modulation engine** for dynamic parameter sweeping
4. **Per-channel channel-memory blocks** that encode all modulation state

---

## Architecture

### Component Hierarchy

```
config.c/h
    └── Stores advanced settings in system_config_t
    └── Persists to EEPROM via eeprom.c

channel_mem.c/h
    └── ChannelBlock struct mirrors MK-312BT SRAM layout (0x80-0xBF per channel)
    └── Holds all modulation parameters: ramp, intensity, freq, width groups
    └── Each group: value, min, max, rate, step, action_min, action_max, select, timer
    └── Select byte encodes timer rate AND value source (MA, ADV_PARAM, static, etc.)

param_engine.c/h
    └── param_engine_tick(): Called once per ~244 Hz master tick
    └── Reads select bytes to determine timer rate and value source
    └── Static sources (select timer bits = 0):
        0x00 = STATIC (no update, value unchanged)
        0x04 = ADV_PARAM (value = advanced setting for this group, updated each tick)
        0x08 = MA_KNOB (value follows MA knob, scaled to ma_range_min..ma_range_max)
    └── Timer-driven sweeps (select timer bits != 0):
        Timer 1 = ~244 Hz, Timer 2 = ~30 Hz, Timer 3 = ~1 Hz
        value increments/decrements by step each rate ticks
        At min/max boundary: Reverse, Loop, Stop, or trigger a module
    └── ADV_PARAM mapping per group:
        ramp group  (0x9C) -> adv_depth
        intensity   (0xA5) -> adv_effect
        frequency   (0xAE) -> adv_frequency
        width       (0xB7) -> adv_width

mode_dispatcher.c
    └── Routes built-in modes to inline bytecode execution via mode_programs.c
    └── Routes user modes to bytecode execution
    └── Calls param_engine_tick() each main-loop iteration

pulse_gen.c/h + interrupts.c
    └── Timer1 ISR: Channel A biphasic pulse generation (PB2/PB3)
    └── Timer2 ISR: Channel B biphasic pulse generation (PB0/PB1)
    └── Main loop sets width, frequency (period), and gate on/off
    └── ISRs run autonomously with 5-phase state machine

MK312BT.ino (main loop)
    └── Reads channel_a/channel_b freq_value, width_value, intensity_value, gate_value
    └── Converts to timer parameters and DAC values
    └── Calls pulse_set_width/frequency/gate functions
    └── Reads level pots via ADC, writes DAC for intensity
```

---

## Files Added/Modified

### Core Files

**channel_mem.h / channel_mem.c**
- Defines `ChannelBlock` struct: 64 bytes per channel, offset from 0x80
- Contains all modulation groups: ramp (0x9C), intensity (0xA5), freq (0xAE), width (0xB7)
- Defines select byte constants: `SEL_TIMER_MASK`, `SEL_TIMER_244HZ/30HZ/1HZ`
- Defines action codes: `ACTION_REVERSE`, `ACTION_REV_TOGGLE`, `ACTION_LOOP`, `ACTION_STOP`
- Defines gate flags: `GATE_ON_BIT`, `GATE_POL_*`, `GATE_ALT_POL`, `GATE_AUDIO_FREQ/INT`

**param_engine.h / param_engine.c**
- `param_engine_init()`: Reset tick counter and pending module state
- `param_engine_tick()`: Step all four parameter groups for both channels each master tick
- `param_engine_check_module_trigger()`: Check if a boundary action triggered a module switch
- `param_engine_get_tick()`: Return current tick counter (0-255 wrapping)
- Implements select byte decode: static sources (ADV_PARAM, MA_KNOB) and timer sweeps
- ADV_PARAM (`select & 0x1C == 0x04`): value is set from the relevant advanced config field each tick
- MA_KNOB (`select & 0x1C == 0x08`): value is scaled MA reading within ma_range_min..ma_range_max

**mode_programs.c/h**
- Bytecode programs for all 25 modes stored in PROGMEM
- Programs write select bytes and other channel-memory fields to configure modulation
- Built-in mode programs set `freq_select = 0x04` for ADV_PARAM frequency source in Audio/Phase modes

**ADVANCED_SETTINGS_IMPLEMENTATION.md** (this file)
- Implementation documentation

### Modified Files

**config.h / config.c**
- Added 8 advanced setting fields to `system_config_t`:
  - `adv_ramp_level` (0-255)
  - `adv_ramp_time` (0-255)
  - `adv_depth` (0-255, default 50) → feeds ramp group when select = 0x04
  - `adv_tempo` (0-255, default 50) → Phase 2 cycle rate (encoded in rate field by mode program)
  - `adv_frequency` (0-255, default 107) → feeds freq group when select = 0x04
  - `adv_effect` (0-255, default 128) → feeds intensity group when select = 0x04
  - `adv_width` (0-255, default 130) → feeds width group when select = 0x04
  - `adv_pace` (0-255, default 50) → Combo width cycle rate (encoded in rate field by mode program)
- Updated `config_load_from_eeprom()` to load advanced settings
- Updated `config_set_defaults()` to set sensible defaults

**eeprom.h** (already had the fields)
- Structure already included advanced settings storage at 0x800D-0x8014
- No changes needed

---

## Advanced Settings Reference

### Settings and Their Effects

| Setting | Range | Default | Affects Modes | Description |
|---------|-------|---------|---------------|-------------|
| **Depth** | 0-255 | 50 | Stroke, Phase 2 | Controls level variation (opposite behavior between modes) |
| **Tempo** | 0-255 | 50 | Phase 2 | Controls pulse rate (01=fastest, 100=slowest) |
| **Pace** | 0-255 | 50 | Combo | Controls width pulsing rate |
| **Width** | 0-255 | 130 | Climb, Combo, Intense, Toggle, Torment, Phase 3 | Sets pulse width value directly via ADV_PARAM source |
| **Frequency** | 0-255 | 107 | Audio 1, Audio 2, Phase 1-3 | Sets carrier frequency directly via ADV_PARAM source |
| **Effect** | 0-255 | 128 | None (built-in) | Reserved for user routines |
| **Ramp Level** | 0-255 | 128 | Auto-ramp | Sets target level for auto-ramp |
| **Ramp Time** | 0-255 | 128 | Auto-ramp | Sets ramp-up duration |

### How ADV_PARAM Source Works

When a parameter group's `select` byte has timer bits = 0 and source bits = `0x04` (ADV_PARAM), the parameter engine calls `config_get()` on **every tick** and sets `value` from the corresponding advanced setting:

- `freq_select = 0x04` → `freq_value = cfg->adv_frequency` every tick
- `width_select = 0x04` → `width_value = cfg->adv_width` every tick
- `ramp select = 0x04` → `ramp_value = cfg->adv_depth` every tick
- `intensity_select = 0x04` → `intensity_value = cfg->adv_effect` every tick

This means adjusting an advanced setting via the menu takes effect **immediately** in all modes that use ADV_PARAM for that group — no mode restart required.

### Special Behaviors

**Depth Setting:**
- **Stroke**: Depth 100 = maximum pulsing on Channel A (slow), Depth 10 = no level variation (feels like Intense)
- **Phase 2**: Depth 100 = no level variation, Depth 10 = maximum level variation (OPPOSITE of Stroke!)

**Tempo Setting (Phase 2 only):**
- Controls cycle rate of level ramping (written into the ramp rate field by the Phase 2 mode program)
- Tempo 01 = fastest pulses
- Tempo 100 = extremely slow (~80 seconds per pulse at Depth 10)

**Width Setting:**
- Factory default: 130 (used by Climb, Intense, Toggle, Torment, Phase 3 when not overridden)
- Modes with `width_select = 0x04` track this value in real time
- Combo has special behavior: toggles or ramps based on value vs 200

**Frequency Setting:**
- Default 107 (Electrokin's famous recommendation)
- Modes with `freq_select = 0x04` (Audio 1, Audio 2, Phase 1-3) track this value in real time
- Adjusting F from the menu takes effect immediately without restarting the mode

---

## Multi-Adjust (MA) Knob Functions

The MA knob serves different functions depending on the active mode. The mode program sets `ma_range_min` and `ma_range_max` in the channel block, and sets appropriate `select` bytes with `SEL_MIN_SRC_MA` or `SEL_RATE_SRC_MA` to indicate MA-driven behavior:

| Mode | Level | Frequency | Width | Notes |
|------|-------|-----------|-------|-------|
| Waves | | Cycle Rate | Cycle Rate | MA controls sweep speed |
| Stroke | Cycle Rate | Cycle Rate | Cycle Rate | MA controls all sweep speeds |
| Climb | | Cycle Rate | | MA controls freq sweep speed |
| Combo | | | | MA controls on-off rate |
| Intense | | Value (MA_KNOB) | | MA directly sets frequency |
| Rhythm | | Value (MA_KNOB) | | MA directly sets frequency and on-off rate |
| Audio 1/2/3 | | Value (MA_KNOB) | | MA directly sets carrier frequency |
| Random 1 | No Effect | No Effect | No Effect | |
| Random 2 | No Effect | No Effect | No Effect | |
| Toggle | | | | MA controls toggle rate |
| Orgasm | | Value (MA_KNOB) | | MA directly sets frequency |
| Torment | | Value (MA_KNOB) | | MA directly sets frequency |
| Phase 1/2/3 | Controls phase relationship between A and B channels | | | |

**Select Byte Encoding for MA:**
- `SEL_RATE_SRC_MA` (bits 7:5 = 2): MA knob value inverts the sweep rate — higher MA = slower sweep
- `SEL_MIN_SRC_MA` (bits 4:2 = 2): MA knob value sets the min bound of the sweep each tick
- `SEL_STATIC_MA = 0x08` (timer bits = 0, source = MA_KNOB): MA value directly sets the parameter value each tick, scaled to `ma_range_min..ma_range_max`

---

## Parameter Group Structure

Each of the four parameter groups (ramp/0x9C, intensity/0xA5, freq/0xAE, width/0xB7) contains 9 fields:

| Offset | Field | Description |
|--------|-------|-------------|
| +0 | value | Current parameter value |
| +1 | min | Sweep minimum (lower bound) |
| +2 | max | Sweep maximum (upper bound) |
| +3 | rate | Ticks per step (larger = slower) |
| +4 | step | Value change per rate-tick |
| +5 | action_min | Action when value hits min |
| +6 | action_max | Action when value hits max |
| +7 | select | Timer rate + value source encoding |
| +8 | timer | Tick countdown accumulator |

**Action codes at boundaries:**
- `0xFF` = Reverse direction (swap min/max and actions)
- `0xFE` = Reverse + toggle gate polarity (GATE_ALT_POL)
- `0xFD` = Loop (wrap to other end)
- `0xFC` = Stop (set `select = SEL_TIMER_NONE`, freeze value)
- `0x00-0xDB` = Trigger that module number (jump to next mode program)

---

## Integration Notes

### Built-in vs User Modes

**Built-in Modes (MODE_NUM_WAVES through MODE_NUM_PHASE3):**
- Mode programs in PROGMEM write channel-block fields via bytecode to configure modulation
- `param_engine_tick()` reads those fields and applies MA/ADV_PARAM logic each tick
- Support advanced settings via ADV_PARAM source (select = 0x04) for real-time adjustment

**User Modes (User1 through User7, Split):**
- Also use bytecode execution from mode_programs.c
- Can be customized by modifying bytecode programs
- Can use the same ADV_PARAM and MA_KNOB select sources if their programs set them

### Initialization Sequence

1. `channel_mem_init()` - Zero-initialize channel blocks
2. `param_engine_init()` - Reset tick counter
3. `config_load_from_eeprom()` - Load settings including advanced settings
4. `pulse_gen_init()` - Initialize Timer1/Timer2, start ISRs, gates OFF
5. `mode_dispatcher_select_mode(mode)` - User selects mode
   - Mode program bytecode runs and writes initial channel-block fields
6. Main loop calls `mode_dispatcher_update()` each frame:
   - Inline bytecode execution updates channel-block fields
   - `param_engine_tick()` applies MA/ADV_PARAM logic, advances timer sweeps
   - `param_engine_check_module_trigger()` checks for module-switch boundary events
7. Main loop reads `channel_a/b.freq_value`, `.width_value`, `.gate_value`
   - Converts freq_value to Hz and calls `pulse_set_frequency_a/b()`
   - Converts width_value to μs and calls `pulse_set_width_a/b()`
   - Gate flags → `pulse_set_gate_a/b(PULSE_ON/OFF)`
8. Timer ISRs autonomously generate biphasic pulses at configured rate

### Memory Usage

The parameter engine adds minimal RAM overhead:
- All parameter state lives in `channel_a` and `channel_b` (two `ChannelBlock` structs, 64 bytes each = 128 bytes total)
- `param_engine.c` uses only 3 static bytes: `tick_counter`, `pending_module_a`, `pending_module_b`
- Advanced settings already in `system_config_t` structure (no extra RAM)

Total additional RAM for param engine logic: **3 bytes**

---

## Testing Recommendations

### Test Cases

1. **Advanced Settings Storage**
   - Set each advanced setting to non-default value
   - Save to EEPROM
   - Power cycle device
   - Verify settings persist

2. **ADV_PARAM Real-Time Response**
   - Enter Audio 1, Audio 2, or any Phase mode
   - Adjust Frequency (F) from the advanced settings menu while mode is running
   - Verify carrier frequency changes immediately without restarting the mode

3. **MA Knob Functions**
   - **Waves**: Adjust MA, verify cycle rate changes
   - **Intense**: Adjust MA, verify frequency changes immediately
   - **Combo**: Adjust MA, verify on-off rate changes

4. **Boundary Actions**
   - Verify U-D sweep reverses at min and max correctly
   - Verify U-U (loop) wrap-around produces sawtooth pattern
   - Verify ACTION_STOP freezes the parameter

5. **Mode Switching**
   - Switch between modes rapidly
   - Verify no parameter carryover between channel blocks
   - Verify each mode initializes its channel block correctly

---

## Known Limitations

1. **Phase Control**: MA phase control for Phase 1-3 modes controls phase relationship in hardware (polarity and timing) — the exact original algorithm for the inter-channel phase offset has not been fully verified against the original firmware.

2. **Orgasm Progressive Behavior**: The 140-second + 100-second progressive width sequence requires a long-running state machine in the mode program. The representative program in `mode_programs.c` may not be cycle-accurate to the original.

3. **Torment Random Behavior**: Random burst timing uses the PRNG (`prng.c`). Burst distribution may differ from the original firmware.

4. **Audio Modes**: `adv_frequency` is correctly applied to freq_value via ADV_PARAM (select=0x04), but the audio envelope follower in `audio_processor.c` drives intensity and width — those paths are independent of the parameter engine.

5. **Random1 Mode**: The "randomly selects from first 6 modes" behavior selects a random module from the mode program table; exact timing and weighting may differ from the original.

---

## Future Enhancements

1. Verify Phase mode MA phase-control algorithm against real MK-312BT hardware output
2. Implement Orgasm mode's cycle-accurate 240-second progressive sequence
3. Implement Torment mode's exact random burst pattern from original firmware
4. Implement auto-ramp functionality using ramp_level and ramp_time settings
5. Implement Random1 mode's accurate weighted mode-selection behavior

---

## References

- `MODE_PARAMETERS_REFERENCE.md` - Complete mode parameter tables
- `MK312BT_INSTRUCTION_SET.md` - Bytecode instruction reference
- `MEMORY_MAP_REFERENCE.md` - Memory address mapping
- `channel_mem.h` - ChannelBlock struct and select byte constants
- `param_engine.c` - Complete parameter engine implementation with select byte decode comments
- Original MK-312BT firmware analysis by Juggle5
- Electrokin's famous settings: Depth=100, Frequency=107, Width=70

---

*Last updated: 2026-02-19 — added ADV_PARAM (select=0x04) support to param_engine*
