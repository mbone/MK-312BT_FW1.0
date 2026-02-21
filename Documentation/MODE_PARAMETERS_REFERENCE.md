# MK-312BT Mode Parameters Reference

Comprehensive reference for all built-in mode parameters, advanced settings, and Multi-Adjust (MA) knob behavior.

---

## Advanced Settings Impact Matrix

Advanced settings (accessible through the menu system):
- **P**: Pace
- **W**: Width
- **E**: Effect
- **F**: Frequency
- **T**: Tempo
- **D**: Depth

### Which Settings Affect Which Modes

| Mode | P | W | E | F | T | D |
|------|---|---|---|---|---|---|
| Waves | | | | | | |
| Stroke | | | | | | ✓ |
| Climb | | ✓ | | | | |
| Combo | ✓ | ✓ | | | | |
| Intense | | ✓ | | | | |
| Rhythm | | | | | | |
| Audio 1 | | | | ✓ | | |
| Audio 2 | | | | ✓ | | |
| Audio 3 | | | | | | |
| Split | (depends on split routines) |
| Random 1 | (depends on random routine) |
| Random 2 | | | | | | |
| Toggle | | ✓ | | ✓ | | |
| Orgasm | | | | | | |
| Torment | | ✓ | | | | |
| Phase 1 | | | | ✓ | | |
| Phase 2 | | | | ✓ | ✓ | ✓ |
| Phase 3 | | ✓ | | ✓ | | |

### Special Notes on Advanced Settings

**Depth (D):**
- Only affects Stroke and Phase 2
- **Opposite behavior** between the two:
  - **Stroke**: Depth 100 = maximum pulsing on Channel A (slow), Depth 10 = no level variation (feels like Intense)
  - **Phase 2**: Depth 100 = no level variation, Depth 10 = maximum level variation

**Tempo (T):**
- Only affects Phase 2
- Controls pulse rate
- Tempo 01 = fastest pulses
- Tempo 100 = extremely slow (approximately one pulse every 80 seconds when Depth = 10)

**Pace (P):**
- Only affects Combo
- Determines rate of pulses in width
- Pace 01 = fastest pulses
- Pace 100 = extremely slow (may not be noticeable)

**Effect (E):**
- Does not affect any built-in routines
- Reserved for user-supplied routines via ErosLink

**Famous Settings (Electrokin's Recommended):**
- Depth Adjust: 100
- Frequency Adjust: 107
- Width Adjust: 70

---

## Multi-Adjust (MA) Knob Assignments

The Multi-Adjust knob serves different functions depending on the active mode:

| Mode | Level | Frequency | Width | On-Off |
|------|-------|-----------|-------|--------|
| Waves | | Cycle Rate | Cycle Rate | |
| Stroke | Cycle Rate | Cycle Rate | Cycle Rate | |
| Climb | | Cycle Rate | | |
| Combo | | | | Cycle Rate |
| Intense | | Value | | |
| Rhythm | | Value | | Cycle Rate |
| Audio 1/2/3 | | Value | | |
| Split | (depends on split routines) |
| Random 1 | No Effect |
| Random 2 | No Effect |
| Toggle | | | | Cycle Rate |
| Orgasm | | Value | | |
| Torment | | Value | | |
| Phase 1/2/3 | Controls phase relationship between A and B channels |

**Legend:**
- **Cycle Rate**: MA controls how fast parameter cycles through its range
- **Value**: MA directly sets the actual frequency value

---

## Detailed Mode Parameters

### Signal Parameters

**Level (0-100%):** Controls intensity relative to user's Level A/B knob setting
**Frequency (15-330 Hz):** Pulse frequency of stimulation
**Width (70-250 μs):** Duration of each pulse (steps of 5μs from 70-100, steps of 10μs from 100-250)
**On-Off Time:** Duration signal generator is on/off (only used by Combo, Rhythm, Toggle)

**Value Notation:**
- **U-D**: Value ramps smoothly up and down between minimum and maximum
- **U-U**: Value ramps up to maximum, then restarts at minimum

---

## Mode-by-Mode Breakdown

### Waves
**Channel A:**
- Level: 100% (constant)
- Frequency: U-D, 30-250 Hz, cycle rate = MA
- Width: U-D, 70-200 μs, cycle rate = MA

**Channel B:**
- Level: 100% (constant)
- Frequency: U-D, 60-250 Hz, cycle rate = MA
- Width: U-D, 70-200 μs, cycle rate = MA

**Behavior:** Smooth sine wave modulation, channels not synchronized

---

### Stroke
**Channel A:**
- Level: U-D, (100 - depth) to 100%, cycle rate = MA
  - Minimum level: 20% when depth = 100
  - Depth 100: 30-100% variation
  - Depth 10: constant 100% (no variation)
- Frequency: freq_value=249 (fixed), ~15.7 Hz pulse rate
- Width: 255 μs (fixed)

**Channel B:**
- Level: U-D, 80-100%, cycle rate = MA
- Frequency: freq_value=249 (fixed), ~15.7 Hz pulse rate
- Width: 201 μs (fixed)

**Special:** Smaller level variations cycle faster

---

### Climb
**Channel A:**
- Level: 100% (constant)
- Frequency: U-U, 15-250 Hz, cycle rate = MA
- Width: From advanced width setting (constant)

**Channel B:**
- Level: 100% (constant)
- Frequency: U-U, 15-250 Hz, cycle rate = MA
- Width: From advanced width setting (constant)

**Special:** Channels A and B frequencies are always synchronized

---

### Combo
**Channel A & B:**
- Level: 100% (constant)
- Frequency: U-D, 37-250 Hz
- Width: Complex behavior based on advanced width setting:
  - If adv. width < 200: toggles from 200 to adv. width
  - If adv. width > 200: ramps from adv. width to 200 and back
  - Toggle/ramp rate controlled by Pace setting (01 = fast, 100 = slow)
- On-Off: Controlled by MA (same on and off times)

---

### Intense
**Channel A & B:**
- Level: 100% (constant)
- Frequency: Set directly by MA (15-250 Hz)
- Width: From advanced width setting (constant)
- On-Off: 2 seconds on, 2 seconds off

---

### Rhythm
**Channel A & B:**
- Level: U-U, 75-100%, 65 second repeat cycle
- Frequency: Set directly by MA (37-250 Hz)
- Width: 1 second min-max toggle (70 μs for 1 sec, 180 μs for 1 sec)
- On-Off: Controlled by MA (same on and off times)

---

### Audio 1, 2, 3
**Channel A & B:**
- Level: From audio input
- Frequency: From advanced frequency setting
- Width: From audio input processing
- On-Off: N/A

**Special:** Audio-responsive modes that modulate based on audio input

---

### Split
**Special:** Each channel can run a different mode from: Waves, Stroke, Climb, Combo, Intense, Rhythm, Audio 1, Audio 2, Audio 3

**Note:** Cannot choose the same mode for both channels (e.g., if Channel A is Waves, Channel B cannot be Waves)

---

### Random 1
**From Manual:** "This mode randomly selects from the first 6 modes (Waves, Stroke, Climb, Combo, Intense, Rhythm) and runs each for a random length with MA set to a random value. This provides extremely wide variety of stimulation. MA has no effect. Auto-Ramp does not work."

**Special:** For Climb and Intense within Random 1, width is taken from advanced width setting. If set higher than factory default (130), these modes will feel stronger than others.

---

### Random 2
**Channel A:**
- Level: U-D, 70-100%, variable rate
- Frequency: U-D, 37-250 Hz, variable rate
- Width: U-D, 70-200 μs, variable rate

**Channel B:**
- Level: U-D, 60-100%, variable rate
- Frequency: U-D, 37-250 Hz, variable rate
- Width: U-D, 50-200 μs, variable rate

---

### Toggle
**Channel A & B:**
- Level: 100% (constant)
- Frequency: From advanced frequency setting
- Width: From advanced width setting
- On-Off: Controlled by MA (channels alternate being ON)

**Special:** Channels A and B alternate

---

### Orgasm
**Channel A & B:**
- Level: 100% (constant)
- Frequency: Set directly by MA (15-250 Hz)
- Width: Complex progressive behavior:
  - Minimum starts at 0, stays for 140 seconds
  - Minimum increases to 200 over 100 seconds
  - U-D cycles from minimum to 200
  - Cycle time decreases from 3 seconds to several per second
  - Final cycle: width reaches 250 μs for ~0.5 sec (muscle contraction)

**Special:** Channels A and B are coordinated:
- Both start at minimum
- A increases to 200, then decreases
- B starts increasing when A is halfway down
- When A reaches minimum, waits for B to complete cycle

---

### Torment
**From Manual:** "Designed more for BDSM use than pleasure. Delivers brief random stimulation at random times and varying intensities. MA adjusts pulse frequency. Set Levels slightly higher than normal (vs Toggle or Intense)."

**Channel A & B:**
- Level: Random, variable
- Frequency: Set directly by MA
- Width: From advanced width setting

---

### Phase 1
**Channel A:**
- Level: 100% (constant)
- Frequency: From advanced frequency setting
- Width: U-D, 150 to (150 + MA_offset) μs, very fast cycle
  - MA adjusts maximum: 185 μs when MA full CCW, 150 μs when MA full CW
  - **Note:** Opposite of expected behavior

**Channel B:**
- Level: Follows audio or varies
- Frequency: From advanced frequency setting
- Width: U-D, 150 to (150 + MA_offset) μs, very fast cycle

**Special:** MA controls phase relationship between channels

---

### Phase 2
**Channel A & B:**
- Level: U-D, depth to 100%, cycle rate = tempo
  - Depth 100 = no variation, Depth 10 = maximum variation (opposite of Stroke!)
- Frequency: From advanced frequency setting
- Width: U-D, 150 to (150 + MA_offset) μs, very fast cycle
  - MA adjusts maximum: 185 μs when MA full CCW, 150 μs when MA full CW

**Special:**
- MA controls phase relationship between channels
- Tempo controls pulse rate: 01 = fastest, 100 = glacially slow (~80 sec/pulse at Depth 10)

---

### Phase 3
**Channel A:**
- Level: U-D, 61-100%, 1 per second
- Frequency: From advanced frequency setting (15-250 Hz)
- Width: From advanced width setting

**Channel B:**
- Level: U-D, 62-100%, 1 per second
- Frequency: From advanced frequency setting (15-250 Hz)
- Width: From advanced width setting

**Special:** MA controls phase relationship between channels

---

## Implementation Notes

### Channel Synchronization
Most modes do **not** keep channels synchronized even if settings are identical. This creates natural randomness. Modes with synchronized channels are noted specifically (e.g., Climb frequency, Orgasm coordination).

### Width Steps
- 70-100 μs: steps of 5 μs
- 100-250 μs: steps of 10 μs

### MA Knob Positions
- Full counter-clockwise (CCW): "7:00pm position" - typically slowest or longest values
- Full clockwise (CW): "5:00pm position" - typically fastest or shortest values

### Advanced Width Setting Default
Factory default for width adjustment is **130 μs**. This affects modes like Climb, Intense, Toggle, Torment, and Phase 3.

---

## References

- Original research by Juggle5
- MK-312BT Manual
- ErosLink software analysis
- Community forum discussions (Electrokin's famous settings)
