/*
 * ET-312 Bytecode Modules (corrected to match programs.org)
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

/* Module 2 - Intense B: set width params for ch B */
const uint8_t module_02[] PROGMEM = {
    0xD8, 0x3F,     // SET ch_b gate_ontime = 0x3F
    0xD9, 0x3F,     // SET ch_b gate_offtime = 0x3F
    0xDA, 0x01,     // SET ch_b gate_select = 0x01
    0x00, 0x00
};

/* Module 3 - Stroke A */
const uint8_t module_03[] PROGMEM = {
    0x86, 0x00,     // SET ma_range_high = 0x00
    0x87, 0x20,     // SET ma_range_low  = 0x20
    0xA9, 0x02,     // SET intensity_step = 0x02
    0xAA, 0xFE,     // SET intensity_action_min = REV_TOGGLE
    0xAB, 0xFE,     // SET intensity_action_max = REV_TOGGLE
    0xAC, 0x55,     // SET intensity_select = 0x55
    0xB5, 0x00,     // SET freq_select = 0x00
    0xB7, 0xFF,     // SET width_value = 0xFF
    0xBE, 0x00,     // SET width_select = 0x00
    0x90, 0x05,     // SET gate_value = 0x05
    0x00, 0x00
};

/* Module 4 - Stroke B */
const uint8_t module_04[] PROGMEM = {
    0xE6, 0xE6,     // SET ch_b intensity_min = 0xE6
    0xE9, 0x01,     // SET ch_b intensity_step = 0x01
    0xEA, 0xFE,     // SET ch_b intensity_action_min = REV_TOGGLE
    0xEB, 0xFE,     // SET ch_b intensity_action_max = REV_TOGGLE
    0xEC, 0x41,     // SET ch_b intensity_select = 0x41
    0xF5, 0x00,     // SET ch_b freq_select = 0x00
    0xF7, 0xD8,     // SET ch_b width_value = 0xD8
    0xFE, 0x00,     // SET ch_b width_select = 0x00
    0xD0, 0x05,     // SET ch_b gate_value = 0x05
    0x00, 0x00
};

/* Module 5 - Climb A: frequency sweep step 1 -> chains to 6 */
const uint8_t module_05[] PROGMEM = {
    0x86, 0x01,     // SET ma_range_high = 1
    0x87, 0x64,     // SET ma_range_low  = 100
    0xB5, 0x41,     // SET freq_select = 0x41
    0xB3, 0x06,     // SET freq_action_min = module 6
    0xB0, 0xFF,     // SET freq_max = 255
    0xAE, 0xFF,     // SET freq_value = 255
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

/* Module 8 - Climb B: frequency sweep step 1 -> chains to 9 */
const uint8_t module_08[] PROGMEM = {
    0xEE, 0xFF,     // SET ch_b freq_value = 255
    0xF0, 0xFF,     // SET ch_b freq_max = 255
    0xF2, 0x01,     // SET ch_b freq_step = 1
    0xF3, 0x09,     // SET ch_b freq_action_min = module 9
    0xF5, 0x41,     // SET ch_b freq_select = 0x41
    0x00, 0x00
};

/* Module 9 - Climb B step 2 -> chains to 10 */
const uint8_t module_09[] PROGMEM = {
    0x85, 0x02,     // SET apply_channel = B only
    0xF2, 0x02,     // SET ch_b freq_step = 2
    0xEE, 0xFF,     // SET ch_b freq_value = 255
    0xF3, 0x0A,     // SET ch_b freq_action_min = module 10
    0x00, 0x00
};

/* Module 10 - Climb B step 3 -> chains back to 8 */
const uint8_t module_10[] PROGMEM = {
    0x85, 0x02,     // SET apply_channel = B only
    0xF2, 0x05,     // SET ch_b freq_step = 5
    0xEE, 0xFF,     // SET ch_b freq_value = 255
    0xF3, 0x08,     // SET ch_b freq_action_min = module 8
    0x00, 0x00
};

/* Module 11 - Waves A */
const uint8_t module_11[] PROGMEM = {
    0x86, 0x01,     // SET ma_range_high = 1
    0x87, 0x40,     // SET ma_range_low  = 64
    0xBE, 0x41,     // SET width_select = 0x41 (timer 244Hz)
    0xBB, 0x02,     // SET width_step = 2
    0xB5, 0x41,     // SET freq_select = 0x41 (timer 244Hz)
    0xB0, 0x80,     // SET freq_max = 128
    0x00, 0x00
};

/* Module 12 - Waves B */
const uint8_t module_12[] PROGMEM = {
    0xFE, 0x41,     // SET ch_b width_select = 0x41
    0xFB, 0x03,     // SET ch_b width_step = 3
    0xF5, 0x41,     // SET ch_b freq_select = 0x41
    0xF0, 0x40,     // SET ch_b freq_max = 64
    0x00, 0x00
};

/* Module 13 - Combo A */
const uint8_t module_13[] PROGMEM = {
    0x86, 0x00,     // SET ma_range_high = 0
    0x87, 0x40,     // SET ma_range_low  = 64
    0x9A, 0x4A,     // SET gate_select = 0x4A
    0xB5, 0x02,     // SET freq_select = 0x02 (timer 30Hz)
    0xBE, 0x26,     // SET width_select = 0x26
    0x00, 0x00
};

/* Module 14 - Intense A */
const uint8_t module_14[] PROGMEM = {
    0x86, 0x09,     // SET ma_range_high = 9
    0x00, 0x00
};

/* Module 15 - Rhythm 1 */
const uint8_t module_15[] PROGMEM = {
    0x95, 0x1F,     // SET next_module_timer_max = 31
    0x95, 0x1F,     // (duplicate)
    0x9A, 0x49,     // SET gate_select = 0x49
    0x96, 0x02,     // SET next_module_select = timer 2
    0xA5, 0xE0,     // SET intensity_value = 0xE0
    0x97, 0x10,     // SET next_module_number = 16
    0x86, 0x01,     // SET ma_range_high = 1
    0x87, 0x17,     // SET ma_range_low  = 23
    0xB7, 0x46,     // SET width_value = 0x46
    0xAB, 0xFD,     // SET intensity_action_max = LOOP
    0xBE, 0x00,     // SET width_select = 0
    0xAB, 0xFD,     // (duplicate)
    0xA9, 0x00,     // SET intensity_step = 0
    0xAC, 0x01,     // SET intensity_select = 0x01 (timer 244Hz)
    0xA6, 0xE0,     // SET intensity_min = 0xE0
    0x00, 0x00
};

/* Module 16 - Rhythm 2 */
const uint8_t module_16[] PROGMEM = {
    0x97, 0x11,        // SET next_module_number = 17
    0x5C, 0xA5, 0x01,  // MATHOP XOR intensity_value ^= 1
    0x50, 0xA5, 0x01,  // MATHOP ADD intensity_value += 1
    0xB7, 0xB4,        // SET width_value = 0xB4
    0x00, 0x00
};

/* Module 17 - Rhythm 3 */
const uint8_t module_17[] PROGMEM = {
    0xB7, 0x46,     // SET width_value = 0x46
    0x97, 0x10,     // SET next_module_number = 16
    0x00, 0x00
};

/* Module 18 - Toggle 1 */
const uint8_t module_18[] PROGMEM = {
    0x86, 0x00,        // SET ma_range_high = 0
    0x87, 0x7F,        // SET ma_range_low  = 127
    0x96, 0x02,        // SET next_module_select = timer 2
    0x60,              // LOAD_MA into bank (0x8C/0x18C)
    0x40, 0x95,        // MEMOP STORE: copy bank to 0x95 (next_module_timer_max)
    0x97, 0x13,        // SET next_module_number = 19
    0xB5, 0x04,        // SET freq_select = 0x04
    0xBF, 0x04,        // SET width_timer = 4
    0x90, 0x07,        // SET gate_value = 0x07 (A on)
    0xD0, 0x06,        // SET ch_b gate_value = 0x06 (B off)
    0x00, 0x00
};

/* Module 19 - Toggle 2 */
const uint8_t module_19[] PROGMEM = {
    0x85, 0x01,        // SET apply_channel = A only
    0x90, 0x06,        // SET gate_value = 0x06 (A off)
    0x85, 0x03,        // SET apply_channel = both
    0x60,              // LOAD_MA into bank
    0x40, 0x95,        // MEMOP STORE: copy bank to 0x95
    0x97, 0x12,        // SET next_module_number = 18
    0xD0, 0x07,        // SET ch_b gate_value = 0x07 (B on)
    0x00, 0x00
};

/* Module 20 - Phase 1A */
const uint8_t module_20[] PROGMEM = {
    0x86, 0x01,     // SET ma_range_high = 1
    0x87, 0x20,     // SET ma_range_low  = 32
    0xB5, 0x04,     // SET freq_select = 0x04
    0xBE, 0x00,     // SET width_select = 0
    0xB7, 0x7D,     // SET width_value = 125
    0x00, 0x00
};

/* Module 21 - Phase 2A (targets channel B) */
const uint8_t module_21[] PROGMEM = {
    0xF7, 0x79,     // SET ch_b width_value = 121
    0x00, 0x00
};

/* Module 22 - Phase 3 */
const uint8_t module_22[] PROGMEM = {
    0x83, 0x08,     // SET output_control_flags = 0x08
    0xD0, 0xA0,     // SET ch_b gate_value = 0xA0
    0xAC, 0x01,     // SET intensity_select = 0x01
    0x86, 0xCD,     // SET ma_range_high = 0xCD
    0x87, 0xD4,     // SET ma_range_low  = 0xD4
    0xB5, 0x04,     // SET freq_select = 0x04
    0xEC, 0x09,     // SET ch_b intensity_select = 0x09
    0x00, 0x00
};

/* Module 23 - Audio 1/2 */
const uint8_t module_23[] PROGMEM = {
    0xB5, 0x04,     // SET freq_select = 0x04
    0xBE, 0x00,     // SET width_select = 0
    0x00, 0x00
};

/* Module 24 - Orgasm 1 */
const uint8_t module_24[] PROGMEM = {
    0xAC, 0x00,     // SET intensity_select = 0
    0xB7, 0x32,     // SET width_value = 50
    0xBB, 0x04,     // SET width_step = 4
    0xBA, 0x01,     // SET width_rate = 1
    0xB8, 0x32,     // SET width_min = 50
    0x85, 0x01,     // SET apply_channel = A only
    0xBE, 0x01,     // SET width_select = 0x01 (timer 244Hz)
    0xBD, 0x19,     // SET width_action_max = module 25
    0xFE, 0x00,     // SET ch_b width_select = 0
    0x00, 0x00
};

/* Module 25 - Orgasm 2 */
const uint8_t module_25[] PROGMEM = {
    0x85, 0x01,        // SET apply_channel = A only
    0xBB, 0xFF,        // SET width_step = 255
    0xBC, 0x1A,        // SET width_action_min = module 26
    0xFE, 0x01,        // SET ch_b width_select = 0x01
    0xFD, 0xFF,        // SET ch_b width_action_max = REVERSE
    0x85, 0x03,        // SET apply_channel = both
    0x50, 0xB8, 0x02,  // MATHOP ADD width_min += 2
    0x5C, 0xB8, 0x02,  // MATHOP XOR width_min ^= 2
    0x00, 0x00
};

/* Module 26 - Orgasm 3 */
const uint8_t module_26[] PROGMEM = {
    0x85, 0x01,     // SET apply_channel = A only
    0xBE, 0x00,     // SET width_select = 0
    0xFC, 0x1B,     // SET ch_b width_action_min = module 27
    0x00, 0x00
};

/* Module 27 - Orgasm 4 */
const uint8_t module_27[] PROGMEM = {
    0x85, 0x01,     // SET apply_channel = A only
    0xBE, 0x01,     // SET width_select = 0x01
    0xFE, 0x00,     // SET ch_b width_select = 0
    0xBB, 0x01,     // SET width_step = 1
    0xFB, 0x01,     // SET ch_b width_step = 1
    0x00, 0x00
};

/* Module 28 - Torment 1 */
const uint8_t module_28[] PROGMEM = {
    0x85, 0x03,     // SET apply_channel = both
    0xAC, 0x00,     // SET intensity_select = 0
    0xA5, 0xB0,     // SET intensity_value = 0xB0
    0x90, 0x06,     // SET gate_value = 0x06
    0x8D, 0x05,     // SET random_min = 5
    0x8E, 0x18,     // SET random_max = 24
    0x4D, 0x95,     // MEMOP RAND [0x195] -> ch_b next_module_timer_max
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
    0x4D, 0x97,     // MEMOP RAND [0x197] -> ch_b next_module_number
    0xAB, 0xFF,     // SET intensity_action_max = REVERSE
    0x00, 0x00
};

/* Module 29 - Torment 2 */
const uint8_t module_29[] PROGMEM = {
    0x85, 0x03,     // SET apply_channel = both
    0xAC, 0x01,     // SET intensity_select = 0x01
    0x90, 0x07,     // SET gate_value = 0x07
    0xAB, 0x1C,     // SET intensity_action_max = module 28
    0x00, 0x00
};

/* Module 30 - Torment 3 */
const uint8_t module_30[] PROGMEM = {
    0x85, 0x02,     // SET apply_channel = B only
    0xEC, 0x01,     // SET ch_b intensity_select = 0x01
    0xD0, 0x07,     // SET ch_b gate_value = 0x07
    0xEB, 0x1C,     // SET ch_b intensity_action_max = module 28
    0x00, 0x00
};

/* Module 31 - Torment 4 */
const uint8_t module_31[] PROGMEM = {
    0x85, 0x01,     // SET apply_channel = A only
    0xAC, 0x01,     // SET intensity_select = 0x01
    0x90, 0x07,     // SET gate_value = 0x07
    0xAB, 0x1C,     // SET intensity_action_max = module 28
    0x00, 0x00
};

/* Module 32 - Random 2 */
const uint8_t module_32[] PROGMEM = {
    0x8D, 0x01,     // SET random_min = 1
    0x8E, 0x04,     // SET random_max = 4
    0x4D, 0xB2,     // MEMOP RAND [0x1B2] ch_b freq_step
    0x4C, 0xA8,     // MEMOP RAND [0x0A8] intensity_rate
    0x4D, 0xA8,     // MEMOP RAND [0x1A8] ch_b intensity_rate
    0x4C, 0xB1,     // MEMOP RAND [0x0B1] freq_rate
    0x4D, 0xB1,     // MEMOP RAND [0x1B1] ch_b freq_rate
    0x4C, 0xBA,     // MEMOP RAND [0x0BA] width_rate
    0x4D, 0xBA,     // MEMOP RAND [0x1BA] ch_b width_rate
    0xBE, 0x01,     // SET width_select = 0x01
    0xB5, 0x02,     // SET freq_select = 0x02
    0xAC, 0x02,     // SET intensity_select = 0x02
    0xD6, 0x03,     // SET ch_b next_module_select = 3
    0xD7, 0x20,     // SET ch_b next_module_number = 32
    0x8D, 0x05,     // SET random_min = 5
    0x8E, 0x1F,     // SET random_max = 31
    0x4D, 0x95,     // MEMOP RAND [0x195] ch_b next_module_timer_max
    0x00, 0x00
};

/* Module 33 - Combo B */
const uint8_t module_33[] PROGMEM = {
    0xF2, 0x02,     // SET ch_b freq_step = 2
    0xFB, 0x02,     // SET ch_b width_step = 2
    0x00, 0x00
};

/* Module 34 - Audio 3 */
const uint8_t module_34[] PROGMEM = {
    0xB5, 0x00,     // SET freq_select = 0
    0xBE, 0x00,     // SET width_select = 0
    0xAE, 0x0A,     // SET freq_value = 10
    0x00, 0x00
};

/* Module 35 - Phase 2B */
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

const uint8_t module_sizes[] PROGMEM = {
    sizeof(module_00), sizeof(module_01), sizeof(module_02), sizeof(module_03),
    sizeof(module_04), sizeof(module_05), sizeof(module_06), sizeof(module_07),
    sizeof(module_08), sizeof(module_09), sizeof(module_10), sizeof(module_11),
    sizeof(module_12), sizeof(module_13), sizeof(module_14), sizeof(module_15),
    sizeof(module_16), sizeof(module_17), sizeof(module_18), sizeof(module_19),
    sizeof(module_20), sizeof(module_21), sizeof(module_22), sizeof(module_23),
    sizeof(module_24), sizeof(module_25), sizeof(module_26), sizeof(module_27),
    sizeof(module_28), sizeof(module_29), sizeof(module_30), sizeof(module_31),
    sizeof(module_32), sizeof(module_33), sizeof(module_34), sizeof(module_35),
};