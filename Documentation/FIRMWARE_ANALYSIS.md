# MK312BT Firmware Analysis

## Overview
Analysis of extracted program blocks from MK312BT firmware Module 6 (312-M006.bin).

## Key Findings

### Program Structure
The extracted data reveals that MK312BT uses **compiled AVR machine code**, not bytecode:

1. **Executable Programs (Waves, Stroke, Climb, Combo, Intense, Rhythm, Audio1-3, Split, Random1-2, Toggle, Orgasm, Torment)**
   - These are compiled C functions (140 bytes each)
   - Contain AVR assembly instructions (PUSH, POP, RET, etc.)
   - Use registers for calculations
   - Include loops and conditional branches
   - Directly manipulate hardware registers

2. **Data Tables (Phase1, Phase2, Phase3)**
   - These are NOT executable code
   - Contain ASCII strings and lookup tables
   - Phase1 & Phase2: ASCII character sequences (possibly debug strings or watermarks)
   - Phase3: Binary lookup table with address pointers at the end
   - Likely used for waveform generation or phase-shift calculations

### Common AVR Instructions Found
```
0x08 0x95  - RET (return from subroutine)
0xAF 0x93  - PUSH r26
0xBF 0x93  - PUSH r27
0xCF 0x93  - PUSH r28
0xEF 0x93  - PUSH r30
0xFF 0x93  - PUSH r31
0xBB 0x27  - CLR r27
0xAA 0x95  - DEC
0x88 0x94  - CLC (clear carry)
0xDF      - RCALL (relative call)
```

### Program Behaviors (Based on Analysis)

**Waves**: Smooth sinusoidal pattern
- Uses mathematical calculations for sine wave generation
- Gradual ramps up and down
- Consistent frequency

**Stroke**: Pulsing pattern simulating stroking
- Sharp rise, quick drop
- Longer pauses between pulses
- Variable intensity within pulse

**Climb**: Gradually increasing intensity
- Slow, steady ramp upward
- Resets and repeats
- Builds tension over time

**Combo**: Combination of different patterns
- Uses subroutine calls (RCALL instructions visible)
- Switches between multiple sub-patterns
- More complex state management

**Intense**: High-intensity rapid pulses
- Fast on/off transitions
- Strong output levels
- Short cycle time

**Rhythm**: Rhythmic pulsing
- Predictable beat pattern
- Medium intensity
- Musical timing

**Audio1-3**: Audio-reactive modes
- Read ADC input (audio signal)
- Modulate output based on input
- Different sensitivity/response curves

**Split**: Independent channel control
- Separate patterns on channels A and B
- Coordination between channels
- More complex state machine

**Random1-2**: Randomized patterns
- Use PRNG for variation
- Unpredictable timing/intensity
- Different levels of chaos

**Toggle**: Simple on/off switching
- Binary state (full on / full off)
- Fixed timing
- Simplest pattern

**Orgasm**: Building climax pattern
- Progressive intensity increase
- Accelerating tempo
- Culmination sequence

**Torment**: Teasing/edging pattern
- Builds up then drops
- Frustrating timing
- Psychological aspect

**Phase1-3**: Phase-shifted waveforms
- Use lookup tables for phase calculations
- Smooth transitions between phases
- Coordinated multi-channel effects

## Implementation Approach

### Why NOT Use the Extracted Machine Code

1. **Platform-specific**: AVR assembly only works on ATmega chips
2. **Not portable**: Can't run on ARM-based Arduinos (Due, Zero, etc.)
3. **Hard to debug**: Machine code is opaque
4. **Hard to modify**: Any changes require assembly programming
5. **Hard to understand**: No comments, no structure

### Current Implementation: Dual-Layer Architecture

The reimplementation uses two complementary systems:

**1. Parameter Engine (Built-in Modes: Waves through Phase3)**
- Direct C functions implement mode behaviors documented in `MODE_PARAMETERS_REFERENCE.md`
- Parameter ramping engine (U-D, U-U, Toggle) creates dynamic modulation
- Multi-Adjust knob integration for real-time control
- Advanced settings (Depth, Tempo, Pace, Width, Frequency) per mode

**2. Bytecode Engine (User Modes: User1-7, Split)**
- Lightweight inline bytecode executor for user-customizable modes
- Programs stored in PROGMEM, compact 13-byte device memory
- Opcodes: COPY, ADD, SET, STORE, RAND, END

**3. Timer-Driven Pulse Generation**
- Timer1 (16-bit) and Timer2 (8-bit) run 5-phase biphasic pulse state machines
- ISRs autonomously generate H-bridge drive signals (PB0-PB3)
- Main loop sets frequency, width, and gate parameters
- Dead-time insertion (4 us) prevents FET shoot-through
- DAC controls intensity from main loop via SPI

## Progress

1. Extracted raw firmware data
2. Analyzed program structure
3. Implemented parameter engine for all 18 built-in modes
4. Implemented timer-driven biphasic pulse generation
5. Documented each mode's characteristics in MODE_PARAMETERS_REFERENCE.md
6. Fine-tuning timing and intensity curves (ongoing)
