/*
 * MK-312BT Bytecode Modules
 *
 * These modules configure the parameter engine registers for each mode.
 * They are verified against ErosLink Interactive Controls screenshots
 * from the original ET-312 hardware running firmware v1.6.
 *
 * When modifying, cross-reference with ErosLink to verify correctness.
 *
 * Each module is a small bytecode program stored in PROGMEM that configures
 * param engine registers (intensity, frequency, width, gate, ramp, etc.).
 *
 * Modules run ONCE when a mode is selected, or when a module chain triggers.
 * The param engine then runs autonomously using the configured registers.
 *
 * 36 modules total, referenced by mode dispatch table in mode_dispatcher.c.
 *
 * SET opcode: 0x80+ => addr = 0x80 + (opcode & 0x3F), bit6 => +0x100 for ch B
 * MEMOP: 0x40-0x4F => op=(opcode&0x0C)>>2: 0=STORE,1=LOAD,2=DIV2,3=RAND
 * MATHOP: 0x50-0x5F => op=(opcode&0x0C)>>2: 0=ADD,1=AND,2=OR,3=XOR
 * COPY: 0x20-0x3F => multi-byte set
 * END: 0x00
 */

#include "mode_programs.h"
#include <avr/pgmspace.h>

/* Module 0 - Turn Off Gates */
const uint8_t module_00[] PROGMEM = {
    0x90, 0x06,     // SET gate_value = 0x06 (pos pulses, gate off)
    0x00, 0x00
};

/* Module 1 - Turn On Gates */
const uint8_t module_01[] PROGMEM = {
    0x90, 0x07,     // SET gate_value = 0x07 (pos pulses, gate on)
    0x00, 0x00
};

/* Module 2 - Intense B: enable 2s on/off gating on channel B
 * From ErosLink: both channels active, freq=MA, width=adv, level=100% constant
 * Defaults already provide freq_select=0x08(MA), width_select=0x04(adv), gate=ON
 * Only need to enable gate timer for the 2s on/off cycle */
const uint8_t module_02[] PROGMEM = {
    0xDA, 0x02,     // SET ch_b gate_select = 0x02 (30Hz timer -> ~2s on/off)
    0x00, 0x00
};

/* Module 3 - Stroke A: fixed freq, MA controls intensity cycle rate
 * Level U-D with depth controlling min, fixed freq=0xF9, fixed width=0xFF
 * ErosLink: freq=97.5%, width=100%, Depth checked, Rate MA checked */
const uint8_t module_03[] PROGMEM = {
    0x86, 0x01,     // SET ma_range_high = 1  (intensity rate when MA=max -> fastest)
    0x87, 0x50,     // SET ma_range_low  = 80 (intensity rate when MA=0  -> slowest)
    0xA6, 0x4D,     // SET intensity_min = 77 (overridden by min_src=~ADV in select)
    0xA7, 0xFF,     // SET intensity_max = 255
    0xA9, 0x01,     // SET intensity_step = 1
    0xAC, 0x55,     // SET intensity_select = 0x55 (244Hz, min_src=~ADV/depth, rate_src=MA)
    0xAA, 0xFE,     // SET intensity_action_min = REV_TOGGLE
    0xAB, 0xFE,     // SET intensity_action_max = REV_TOGGLE
    0xB5, 0x00,     // SET freq_select = 0x00 (no update, fixed)
    0xAE, 0xF9,     // SET freq_value = 249 (ErosLink: 97.5%)
    0xBE, 0x00,     // SET width_select = 0x00 (no update, fixed)
    0xB7, 0xFF,     // SET width_value = 255 (ErosLink: 100%)
    0x00, 0x00
};

/* Module 4 - Stroke B: fixed freq=249, MA controls intensity cycle rate
 * Level U-D with depth controlling min, fixed freq=0xF9, fixed width=0xC9
 * ErosLink: freq=97.5%, width=78.8%, intensity_min=80.4% */
const uint8_t module_04[] PROGMEM = {
    0xE6, 0xCD,     // SET ch_b intensity_min = 205 (ErosLink: 80.4%)
    0xE7, 0xFF,     // SET ch_b intensity_max = 255
    0xE9, 0x01,     // SET ch_b intensity_step = 1
    0xEC, 0x41,     // SET ch_b intensity_select = 0x41 (244Hz, rate_src=MA)
    0xEA, 0xFE,     // SET ch_b intensity_action_min = REV_TOGGLE
    0xEB, 0xFE,     // SET ch_b intensity_action_max = REV_TOGGLE
    0xF5, 0x00,     // SET ch_b freq_select = 0x00 (fixed)
    0xEE, 0xF9,     // SET ch_b freq_value = 249 (ErosLink: 97.5%)
    0xFE, 0x00,     // SET ch_b width_select = 0x00 (fixed)
    0xF7, 0xC9,     // SET ch_b width_value = 201 (ErosLink: 78.8%)
    0x00, 0x00
};

/* Module 5 - Climb A: freq sweep down from 255 to 8, step 1 -> chains to 6
 * ErosLink: freq_min=3.1% (8), freq_max=100% (255) */
const uint8_t module_05[] PROGMEM = {
    0x86, 0x01,     // SET ma_range_high = 1   (rate when MA=max -> fastest sweep)
    0x87, 0x64,     // SET ma_range_low  = 100 (rate when MA=0   -> slowest sweep)
    0xAF, 0x08,     // SET freq_min = 8 (ErosLink: 3.1%)
    0xB5, 0x41,     // SET freq_select = 0x41 (244Hz, rate_src=MA)
    0xB3, 0x06,     // SET freq_action_min = module 6
    0xB0, 0xFF,     // SET freq_max = 255
    0xAE, 0xFF,     // SET freq_value = 255 (start at max)
    0xB2, 0x01,     // SET freq_step = 1
    0x00, 0x00
};

/* Module 6 - Climb A step 2 -> chains to 7 */
const uint8_t module_06[] PROGMEM = {
    0xB2, 0x02,     // SET freq_step = 2
    0xAE, 0xFF,     // SET freq_value = 255
    0xB3, 0x07,     // SET freq_action_min = module 7
    0x00, 0x00
};

/* Module 7 - Climb A step 3 -> chains back to 5 */
const uint8_t module_07[] PROGMEM = {
    0xB2, 0x04,     // SET freq_step = 4
    0xAE, 0xFF,     // SET freq_value = 255
    0xB3, 0x05,     // SET freq_action_min = module 5
    0x00, 0x00
};

/* Module 8 - Climb B: freq sweep down from 255 to 8, step 1 -> chains to 9
 * ErosLink: freq_min=3.1% (8), freq_max=100% (255) */
const uint8_t module_08[] PROGMEM = {
    0x85, 0x02,     // SET apply_channel = B only
    0x86, 0x01,     // SET ma_range_high = 1
    0x87, 0x64,     // SET ma_range_low  = 100
    0xAF, 0x08,     // SET freq_min = 8 (ErosLink: 3.1%)
    0xB5, 0x41,     // SET freq_select = 0x41 (244Hz, rate_src=MA)
    0xB3, 0x09,     // SET freq_action_min = module 9
    0xB0, 0xFF,     // SET freq_max = 255
    0xAE, 0xFF,     // SET freq_value = 255 (start at max)
    0xB2, 0x01,     // SET freq_step = 1
    0x00, 0x00
};

/* Module 9 - Climb B step 2 -> chains to 10 */
const uint8_t module_09[] PROGMEM = {
    0x85, 0x02,     // SET apply_channel = B only
    0xB2, 0x02,     // SET freq_step = 2
    0xAE, 0xFF,     // SET freq_value = 255
    0xB3, 0x0A,     // SET freq_action_min = module 10
    0x00, 0x00
};

/* Module 10 - Climb B step 3 -> chains back to 8 */
const uint8_t module_10[] PROGMEM = {
    0x85, 0x02,     // SET apply_channel = B only
    0xB2, 0x05,     // SET freq_step = 5
    0xAE, 0xFF,     // SET freq_value = 255
    0xB3, 0x08,     // SET freq_action_min = module 8
    0x00, 0x00
};

/* Module 11 - Waves A: freq U-D 28-15 Hz, width U-D 0-179 us
 * Matches original firmware values from ErosLink Interactive Controls
 * Both cycle rates controlled by MA (rate_src=MA in select byte) */
const uint8_t module_11[] PROGMEM = {
    0x86, 0x01,     // SET ma_range_high = 1   (rate when MA=max -> fastest)
    0x87, 0x28,     // SET ma_range_low  = 40  (rate when MA=0   -> slowest)
    0xB7, 0x00,     // SET width_value = 0     (start at zero)
    0xB8, 0x00,     // SET width_min = 0       (sweep down to silence)
    0xB9, 0xB3,     // SET width_max = 179
    0xBC, 0xFF,     // SET width_action_min = REVERSE
    0xBD, 0xFF,     // SET width_action_max = REVERSE
    0xBE, 0x41,     // SET width_select = 0x41 (244Hz, rate_src=MA)
    0xBB, 0x03,     // SET width_step = 3
    0xAE, 0x8B,     // SET freq_value = 139    (start at ~28 Hz)
    0xAF, 0x8B,     // SET freq_min = 139      (28 Hz)
    0xB0, 0xFF,     // SET freq_max = 255      (15 Hz)
    0xB3, 0xFF,     // SET freq_action_min = REVERSE
    0xB4, 0xFF,     // SET freq_action_max = REVERSE
    0xB2, 0x01,     // SET freq_step = 1
    0xB5, 0x41,     // SET freq_select = 0x41 (244Hz, rate_src=MA)
    0x00, 0x00
};

/* Module 12 - Waves B: freq U-D 19-15 Hz, width U-D 0-179 us
 * Matches original firmware: narrower freq range than Ch A */
const uint8_t module_12[] PROGMEM = {
    0xF7, 0x00,     // SET ch_b width_value = 0
    0xF8, 0x00,     // SET ch_b width_min = 0
    0xF9, 0xB3,     // SET ch_b width_max = 179
    0xFC, 0xFF,     // SET ch_b width_action_min = REVERSE
    0xFD, 0xFF,     // SET ch_b width_action_max = REVERSE
    0xFE, 0x41,     // SET ch_b width_select = 0x41 (244Hz, rate_src=MA)
    0xFB, 0x03,     // SET ch_b width_step = 3
    0xEE, 0xCD,     // SET ch_b freq_value = 205 (start at ~19 Hz)
    0xEF, 0xCD,     // SET ch_b freq_min = 205  (19 Hz)
    0xF0, 0xFF,     // SET ch_b freq_max = 255  (15 Hz)
    0xF3, 0xFF,     // SET ch_b freq_action_min = REVERSE
    0xF4, 0xFF,     // SET ch_b freq_action_max = REVERSE
    0xF2, 0x01,     // SET ch_b freq_step = 1
    0xF5, 0x41,     // SET ch_b freq_select = 0x41 (244Hz, rate_src=MA)
    0x00, 0x00
};

/* Module 13 - Combo A: freq sweep using mode_init range (168-255), gate from MA
 * ErosLink: freq_min=65.9%(168), freq_max=100%(255) - matches mode_init defaults
 * Width: 30Hz timer with adv_width as min, pace as rate */
const uint8_t module_13[] PROGMEM = {
    0x86, 0x00,     // SET ma_range_high = 0   (gate on/off when MA=max -> fastest)
    0x87, 0x40,     // SET ma_range_low  = 64  (gate on/off when MA=0   -> slowest)
    0x9A, 0x4A,     // SET gate_select = 0x4A (30Hz, on=MA, off=MA)
    0xB5, 0x02,     // SET freq_select = 0x02 (30Hz timer, sweep)
    0xBE, 0x26,     // SET width_select = 0x26 (30Hz, adv_width as min, pace as rate)
    0x00, 0x00
};

/* Module 14 - Intense A: enable 2s on/off gating on channel A
 * From ErosLink: both channels active, freq=MA, width=adv, level=100% constant
 * Defaults already provide freq_select=0x08(MA), width_select=0x04(adv), gate=ON
 * Only need to enable gate timer for the 2s on/off cycle */
const uint8_t module_14[] PROGMEM = {
    0x9A, 0x02,     // SET gate_select = 0x02 (30Hz timer -> ~2s on/off)
    0x00, 0x00
};

/* Module 15 - Rhythm 1: freq 37-250 Hz via MA, gate on/off via MA
 * Intensity U-U 75.7-100% over ~65s, width toggles 70/180 at 1s intervals
 * ErosLink: intensity_min=75.7%(0xC1), both channels identical */
const uint8_t module_15[] PROGMEM = {
    0x86, 0x10,     // SET ma_range_high = 16  (freq=250Hz, gate=fast when MA=max)
    0x87, 0x69,     // SET ma_range_low  = 105 (freq=37Hz, gate=slow when MA=0)
    0x9A, 0x49,     // SET gate_select = 0x49 (244Hz, on=MA, off=MA)
    0x95, 0x1F,     // SET next_module_timer_max = 31 (~1 sec at 30Hz)
    0x96, 0x02,     // SET next_module_select = timer 30Hz
    0x97, 0x10,     // SET next_module_number = 16
    0xB7, 0x46,     // SET width_value = 70
    0xBE, 0x00,     // SET width_select = 0x00 (driven by module chain)
    0xA5, 0xC1,     // SET intensity_value = 193 (75.7%)
    0xA6, 0xC1,     // SET intensity_min = 193 (75.7%)
    0xA7, 0xFF,     // SET intensity_max = 255 (100%)
    0xA8, 0xFD,     // SET intensity_rate = 253 (~1 step/sec at 244Hz)
    0xA9, 0x01,     // SET intensity_step = 1
    0xAB, 0xFD,     // SET intensity_action_max = LOOP
    0xAC, 0x01,     // SET intensity_select = 0x01 (244Hz timer)
    0x00, 0x00
};

/* Module 16 - Rhythm 2 */
const uint8_t module_16[] PROGMEM = {
    0xB7, 0xB4,       // SET width_value = 0xB4 (180)
    0x97, 0x11,       // SET next_module_number = 17
    0x50, 0xA5, 0x01, // MATHOP ADD intensity_value += 1
    0x00              // END
};

/* Module 17 - Rhythm 3 */
const uint8_t module_17[] PROGMEM = {
    0xB7, 0x46,       // SET width_value = 0x46 (70)
    0x97, 0x10,       // SET next_module_number = 16
    0x00              // END
};

/* Module 18 - Toggle 1: ch_A on, ch_B off; only ch_A drives module timer */
const uint8_t module_18[] PROGMEM = {
    0x85, 0x03,     // SET apply_channel = both (for shared settings)
    0xB5, 0x04,     // SET freq_select = 0x04 (ADV_PARAM static, both channels)
    0xBE, 0x04,     // SET width_select = 0x04 (ADV_PARAM static, both channels)
    0x85, 0x01,     // SET apply_channel = A only (module timer only on ch_A)
    0x86, 0x3C,     // SET ma_range_high = 60  (timer_max when MA=max -> ~250ms)
    0x87, 0xF0,     // SET ma_range_low  = 240 (timer_max when MA=0   -> ~1s)
    0x96, 0x41,     // SET next_module_select = 0x41 (244Hz, rate_src=MA) ch_A only
    0x97, 0x13,     // SET next_module_number = 19 (ch_A only)
    0x90, 0x07,     // SET gate_value = 0x07 (ch_A gate on)
    0xD0, 0x06,     // SET ch_B gate_value = 0x06 (ch_B gate off)
    0x85, 0x03,     // SET apply_channel = both (restore)
    0x00, 0x00
};

/* Module 19 - Toggle 2: ch_A off, ch_B on; loop back to 18 */
const uint8_t module_19[] PROGMEM = {
    0x85, 0x01,     // SET apply_channel = A only
    0x90, 0x06,     // SET ch_A gate_value = 0x06 (gate off)
    0x97, 0x12,     // SET ch_A next_module_number = 18 (loop back)
    0x85, 0x03,     // SET apply_channel = both (restore)
    0xD0, 0x07,     // SET ch_B gate_value = 0x07 (gate on)
    0x00, 0x00
};

/* Module 20 - Phase 1/2 ch_A: freq from adv, width U-D 0-179 at 244Hz
 * ErosLink: freq=Freq(adv), width sweeps 0-179 (mode_init range), step=1 */
const uint8_t module_20[] PROGMEM = {
    0xB5, 0x04,     // SET freq_select = 0x04 (adv_frequency)
    0xB7, 0x00,     // SET width_value = 0 (start at min, ch_a)
    0xBB, 0x01,     // SET width_step = 1
    0xBE, 0x01,     // SET width_select = 0x01 (244Hz timer)
    0x00, 0x00
};

/* Module 21 - Phase 1/2 ch_B: width U-D 0-179 starting at max (~180 deg offset)
 * ErosLink: width sweeps 0-179 (mode_init range), starts at opposite end from ch_A */
const uint8_t module_21[] PROGMEM = {
    0xF7, 0xB3,     // SET ch_b width_value = 179 (start at max, ~180 deg offset)
    0xFB, 0x01,     // SET ch_b width_step = 1
    0xFE, 0x01,     // SET ch_b width_select = 0x01 (244Hz timer)
    0x00, 0x00
};

/* Module 22 - Phase 3: intensity U-D, freq from adv, width from adv
 * Ch A: intensity 1Hz cycle, min=0x9B from mode_init (no depth)
 * Ch B: intensity 1Hz cycle, min from depth (adv), Depth checkbox
 * ErosLink: ch_a min=60.9%(0x9B), ch_b min=Depth controlled */
const uint8_t module_22[] PROGMEM = {
    0xB5, 0x04,     // SET freq_select = 0x04 (adv_frequency)
    0xBE, 0x04,     // SET width_select = 0x04 (adv_width)
    0xA9, 0x01,     // SET intensity_step = 1
    0xA8, 0x01,     // SET intensity_rate = 1
    0xAC, 0x03,     // SET intensity_select = 0x03 (1Hz, min_src=OWN)
    0xE9, 0x01,     // SET ch_b intensity_step = 1
    0xE8, 0x01,     // SET ch_b intensity_rate = 1
    0xEC, 0x07,     // SET ch_b intensity_select = 0x07 (1Hz, min_src=ADV/depth)
    0x00, 0x00
};

/* Module 23 - Audio 1/2 */
const uint8_t module_23[] PROGMEM = {
    0xB5, 0x04,     // SET freq_select = 0x04
    0xBE, 0x00,     // SET width_select = 0x00
    0x00, 0x00
};

/* Module 24 - Orgasm 1: width starts at 0, min starts at 0
 * Width ramps up to 200, then module 25 chains */
const uint8_t module_24[] PROGMEM = {
    0xAC, 0x00,     // SET intensity_select = 0x00
    0xB7, 0x00,     // SET width_value = 0 (start at minimum)
    0xBB, 0x04,     // SET width_step = 4
    0xBA, 0x01,     // SET width_rate = 1
    0xB8, 0x00,     // SET width_min = 0 (minimum starts at 0)
    0x85, 0x01,     // SET apply_channel = A only
    0xBE, 0x01,     // SET width_select = 0x01 (timer 244Hz)
    0xBD, 0x19,     // SET width_action_max = module 25
    0xFE, 0x00,     // SET ch_b width_select = 0x00
    0x00, 0x00
};

/* Module 25 - Orgasm 2 */
const uint8_t module_25[] PROGMEM = {
    0x85, 0x01,       // SET apply_channel = A only
    0xBB, 0xFF,       // SET width_step = 255
    0xBC, 0x1A,       // SET width_action_min = module 26
    0xFE, 0x01,       // SET ch_b width_select = 0x01
    0xFD, 0xFF,       // SET ch_b width_action_max = REVERSE
    0x85, 0x03,       // SET apply_channel = both
    0x50, 0xB8, 0x02, // MATHOP ADD width_min += 2
    0x00              // END
};

/* Module 26 - Orgasm 3 */
const uint8_t module_26[] PROGMEM = {
    0x85, 0x01,       // SET apply_channel = A only
    0xBE, 0x00,       // SET width_select = 0x00
    0xFC, 0x1B,       // SET ch_b width_action_min = module 27
    0x00              // END
};

/* Module 27 - Orgasm 4 */
const uint8_t module_27[] PROGMEM = {
    0x85, 0x01,     // SET apply_channel = A only
    0xBE, 0x01,     // SET width_select = 0x01
    0xFE, 0x00,     // SET ch_b width_select = 0x00
    0xBB, 0x01,     // SET width_step = 1
    0xFB, 0x01,     // SET ch_b width_step = 1
    0x00, 0x00
};

/* Module 28 - Torment 1 */
const uint8_t module_28[] PROGMEM = {
    0x85, 0x03,     // SET apply_channel = both
    0xAC, 0x00,     // SET intensity_select = 0x00
    0xA5, 0xB0,     // SET intensity_value = 0xB0
    0x90, 0x06,     // SET gate_value = 0x06 (gate off)
    0x8D, 0x05,     // SET random_min = 5
    0x8E, 0x18,     // SET random_max = 24
    0x4D, 0x95,     // MEMOP RAND [0x195] -> next_module_timer_max (ch B)
    0xD6, 0x03,     // SET ch_b next_module_select = 3
    0xAB, 0x1C,     // SET intensity_action_max = module 28
    0x8D, 0xE0,     // SET random_min = 0xE0
    0x8E, 0xFF,     // SET random_max = 0xFF
    0x4C, 0xA7,     // MEMOP RAND [0x0A7] -> intensity_max
    0x8D, 0x06,     // SET random_min = 6
    0x8E, 0x3F,     // SET random_max = 0x3F
    0x4C, 0xA8,     // MEMOP RAND [0x0A8] -> intensity_rate
    0x8D, 0x1D,     // SET random_min = 0x1D (29)
    0x8E, 0x1F,     // SET random_max = 0x1F (31)
    0x4D, 0x97,     // MEMOP RAND [0x197] -> ch B next_module_number
    0xAB, 0xFF,     // SET intensity_action_max = REVERSE
    0x00, 0x00
};

/* Module 29 - Torment 2 */
const uint8_t module_29[] PROGMEM = {
    0x85, 0x03,     // SET apply_channel = both
    0xAC, 0x01,     // SET intensity_select = 0x01 (timer 244Hz)
    0x90, 0x07,     // SET gate_value = 0x07 (gate on)
    0xAB, 0x1C,     // SET intensity_action_max = module 28
    0x00, 0x00
};

/* Module 30 - Torment 3 */
const uint8_t module_30[] PROGMEM = {
    0x85, 0x02,     // SET apply_channel = B only
    0xAC, 0x01,     // SET intensity_select = 0x01
    0x90, 0x07,     // SET gate_value = 0x07 (gate on)
    0xAB, 0x1C,     // SET intensity_action_max = module 28
    0x00, 0x00
};

/* Module 31 - Torment 4 */
const uint8_t module_31[] PROGMEM = {
    0x85, 0x01,     // SET apply_channel = A only
    0xAC, 0x01,     // SET intensity_select = 0x01
    0x90, 0x07,     // SET gate_value = 0x07 (gate on)
    0xAB, 0x1C,     // SET intensity_action_max = module 28
    0x00, 0x00
};

/* Module 32 - Random 2
 * Ch A: level 70-100%, freq 37-250 Hz, width 70-200 us
 * Ch B: level 60-100%, freq 37-250 Hz, width 50-200 us */
const uint8_t module_32[] PROGMEM = {
    0xA6, 0xB3,     // SET ch_a intensity_min = 179 (~70%)
    0xAF, 0x10,     // SET ch_a freq_min = 16 (250 Hz)
    0xB0, 0x69,     // SET ch_a freq_max = 105 (~37 Hz)
    0xB8, 0x46,     // SET ch_a width_min = 70
    0xE6, 0x99,     // SET ch_b intensity_min = 153 (~60%)
    0xEF, 0x10,     // SET ch_b freq_min = 16 (250 Hz)
    0xF0, 0x69,     // SET ch_b freq_max = 105 (~37 Hz)
    0x8D, 0x01,     // SET random_min = 1
    0x8E, 0x04,     // SET random_max = 4
    0x4D, 0xB2,     // MEMOP RAND [0x1B2] ch_b freq_step
    0x4C, 0xA8,     // MEMOP RAND [0x0A8] intensity_rate
    0x4D, 0xA8,     // MEMOP RAND [0x1A8] ch_b intensity_rate
    0x4C, 0xB1,     // MEMOP RAND [0x0B1] freq_rate
    0x4D, 0xB1,     // MEMOP RAND [0x1B1] ch_b freq_rate
    0x4C, 0xBA,     // MEMOP RAND [0x0BA] width_rate
    0x4D, 0xBA,     // MEMOP RAND [0x1BA] ch_b width_rate
    0xBE, 0x01,     // SET width_select = 0x01 (244Hz)
    0xB5, 0x02,     // SET freq_select = 0x02 (30Hz)
    0xAC, 0x02,     // SET intensity_select = 0x02 (30Hz)
    0xD6, 0x03,     // SET ch_b next_module_select = 3
    0xD7, 0x20,     // SET ch_b next_module_number = 32 (self-loop)
    0x8D, 0x05,     // SET random_min = 5
    0x8E, 0x1F,     // SET random_max = 31
    0x4D, 0x95,     // MEMOP RAND [0x195] ch_b next_module_timer_max
    0x00, 0x00
};

/* Module 33 - Combo B: uses mode_init freq range (168-255), step=2 for both
 * ErosLink: freq_min=65.9%(168), freq_max=100%(255) - matches mode_init defaults */
const uint8_t module_33[] PROGMEM = {
    0xF2, 0x02,     // SET ch_b freq_step = 2
    0xFB, 0x02,     // SET ch_b width_step = 2
    0x00, 0x00
};

/* Module 34 - Audio 3 */
const uint8_t module_34[] PROGMEM = {
    0xB5, 0x00,     // SET freq_select = 0x00
    0xBE, 0x00,     // SET width_select = 0x00
    0xAE, 0x0A,     // SET freq_value = 10
    0x00, 0x00
};

/* Module 35 - Phase 2B (intensity select for phase 2) */
const uint8_t module_35[] PROGMEM = {
    0xAC, 0x25,     // SET intensity_select = 0x25
    0x00, 0x00
};

const uint8_t* const module_table[] PROGMEM = {
    module_00, module_01, module_02, module_03, module_04,
    module_05, module_06, module_07, module_08, module_09,
    module_10, module_11, module_12, module_13, module_14,
    module_15, module_16, module_17, module_18, module_19,
    module_20, module_21, module_22, module_23, module_24,
    module_25, module_26, module_27, module_28, module_29,
    module_30, module_31, module_32, module_33, module_34,
    module_35,
};

