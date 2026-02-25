/*
 * MK-312BT Mode Dispatcher
 *
 * (Full file with corrected 0x60 opcode – loads scaled MA value)
 */

#include "mode_dispatcher.h"
#include "mode_programs.h"
#include "user_programs.h"
#include "channel_mem.h"
#include "param_engine.h"
#include "config.h"
#include "MK312BT_Memory.h"
#include "MK312BT_Modes.h"
#include "eeprom.h"
#include "prng.h"
#include "pulse_gen.h"
#include "dac.h"
#include "MK312BT_Constants.h"
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>

static uint8_t current_mode;
static uint8_t split_mode_a;
static uint8_t split_mode_b;
static uint8_t dispatcher_paused;

#define DEFERRED_NONE       0
#define DEFERRED_SET_MODE   1
#define DEFERRED_PAUSE      2
#define DEFERRED_NEXT       3
#define DEFERRED_PREV       4
#define DEFERRED_RELOAD     5
#define DEFERRED_START_RAMP 6

static volatile uint8_t deferred_cmd = DEFERRED_NONE;
static volatile uint8_t deferred_mode = 0;

// Helper: map raw MA (0-255) to scaled value using channel's range registers
static uint8_t map_ma(uint8_t ma_raw, uint8_t ma_high, uint8_t ma_low) {
    if (ma_high >= ma_low) {
        return ma_low + (uint8_t)(((uint16_t)ma_raw * (ma_high - ma_low)) >> 8);
    }
    return ma_low - (uint8_t)(((uint16_t)ma_raw * (ma_low - ma_high)) >> 8);
}

static uint8_t get_ma_knob_value(void) {
    return config_get()->multi_adjust;
}

static void execute_module(uint8_t module_index) {
    if (module_index >= MODULE_COUNT) return;

    const uint8_t* program = (const uint8_t*)pgm_read_word(&module_table[module_index]);
    const uint8_t* pc = program;

    while (1) {
        uint8_t opcode = pgm_read_byte(pc);

        if ((opcode & 0xE0) == 0x00) {
            break;
        }

        if ((opcode & 0xE0) == 0x20) {
            uint8_t len = 1 + ((opcode & 0x1C) >> 2);
            uint16_t addr = ((opcode & 0x03) << 8) | pgm_read_byte(pc + 1);
            for (uint8_t i = 0; i < len; i++) {
                *channel_get_reg_ptr(addr + i) = pgm_read_byte(pc + 2 + i);
            }
            pc += 2 + len;
            continue;
        }

        if ((opcode & 0xF0) == 0x40) {
            uint16_t addr = ((opcode & 0x03) << 8) | pgm_read_byte(pc + 1);
            uint8_t op = (opcode & 0x0C) >> 2;

            uint8_t apply_a = 1, apply_b = 0;
            if (addr >= 0x80 && addr < 0xC0) {
                uint8_t ac = channel_a.apply_channel;
                apply_a = (ac & 0x01) ? 1 : 0;
                apply_b = (ac & 0x02) ? 1 : 0;
            } else if (addr >= 0x180 && addr < 0x1C0) {
                apply_a = 0;
                apply_b = 1;
            }

            switch (op) {
                case 0: { // STORE: copy from bank to addr
                    uint16_t bank_addr_a = 0x08C;
                    uint16_t bank_addr_b = 0x18C;
                    if (apply_a) {
                        *channel_get_reg_ptr(addr) = *channel_get_reg_ptr(bank_addr_a);
                    }
                    if (apply_b) {
                        *channel_get_reg_ptr(addr + 0x100) = *channel_get_reg_ptr(bank_addr_b);
                    }
                    break;
                }
                case 1: { // LOAD: copy from addr to bank
                    uint16_t bank_addr_a = 0x08C;
                    uint16_t bank_addr_b = 0x18C;
                    if (apply_a) {
                        *channel_get_reg_ptr(bank_addr_a) = *channel_get_reg_ptr(addr);
                    }
                    if (apply_b) {
                        *channel_get_reg_ptr(bank_addr_b) = *channel_get_reg_ptr(addr + 0x100);
                    }
                    break;
                }
                case 2: // DIV2
                    if (apply_a) {
                        *channel_get_reg_ptr(addr) >>= 1;
                    }
                    if (apply_b) {
                        *channel_get_reg_ptr(addr + 0x100) >>= 1;
                    }
                    break;
                case 3: { // RAND
                    ChannelBlock *src_ch = (addr & 0x100) ? &channel_b : &channel_a;
                    uint8_t min_val = src_ch->random_min;
                    uint8_t max_val = src_ch->random_max;
                    uint8_t rand_val;
                    if (max_val > min_val) {
                        uint16_t range = (uint16_t)max_val - (uint16_t)min_val + 1;
                        rand_val = min_val + (uint8_t)(prng_next() % range);
                    } else {
                        rand_val = min_val;
                    }
                    if (apply_a) {
                        *channel_get_reg_ptr(addr) = rand_val;
                    }
                    if (apply_b) {
                        *channel_get_reg_ptr(addr + 0x100) = rand_val;
                    }
                    break;
                }
            }
            pc += 2;
            continue;
        }

        if ((opcode & 0xF0) == 0x50) {
            uint16_t addr = ((opcode & 0x03) << 8) | pgm_read_byte(pc + 1);
            uint8_t value = pgm_read_byte(pc + 2);
            uint8_t op = (opcode & 0x0C) >> 2;

            uint8_t apply_a = 1, apply_b = 0;
            if (addr >= 0x80 && addr < 0xC0) {
                uint8_t ac = channel_a.apply_channel;
                apply_a = (ac & 0x01) ? 1 : 0;
                apply_b = (ac & 0x02) ? 1 : 0;
            } else if (addr >= 0x180 && addr < 0x1C0) {
                apply_a = 0;
                apply_b = 1;
            }

            if (apply_a) {
                uint8_t* ptr = channel_get_reg_ptr(addr);
                switch (op) {
                    case 0: *ptr = *ptr + value; break;
                    case 1: *ptr = *ptr & value; break;
                    case 2: *ptr = *ptr | value; break;
                    case 3: *ptr = *ptr ^ value; break;
                }
            }
            if (apply_b && addr >= 0x80 && addr < 0xC0) {
                uint8_t* ptr = channel_get_reg_ptr(addr + 0x100);
                switch (op) {
                    case 0: *ptr = *ptr + value; break;
                    case 1: *ptr = *ptr & value; break;
                    case 2: *ptr = *ptr | value; break;
                    case 3: *ptr = *ptr ^ value; break;
                }
            }
            pc += 3;
            continue;
        }

        if (opcode == 0x60) { // LOAD_MA – copy scaled MA knob value to bank registers
            uint8_t ma_raw = get_ma_knob_value();
            uint8_t ac = channel_a.apply_channel;
            if (ac & 0x01) {
                uint8_t scaled = map_ma(ma_raw, channel_a.ma_range_high, channel_a.ma_range_low);
                *channel_get_reg_ptr(0x08C) = scaled;
            }
            if (ac & 0x02) {
                uint8_t scaled = map_ma(ma_raw, channel_b.ma_range_high, channel_b.ma_range_low);
                *channel_get_reg_ptr(0x18C) = scaled;
            }
            pc += 1;
            continue;
        }

        if (opcode & 0x80) {
            uint8_t offset = opcode & 0x3F;
            uint8_t value = pgm_read_byte(pc + 1);
            if (opcode & 0x40) {
                *channel_get_reg_ptr(0x180 + offset) = value;
            } else {
                uint8_t ac = channel_a.apply_channel;
                if (ac & 0x01) {
                    *channel_get_reg_ptr(0x80 + offset) = value;
                }
                if (ac & 0x02) {
                    *channel_get_reg_ptr(0x180 + offset) = value;
                }
            }
            pc += 2;
            continue;
        }

        if (opcode & 0x10) {
            pc += 2;
            continue;
        }

        pc++;
    }
}

/* Lookup table: two module indices per built-in mode (0xFF = none). */
static const uint8_t mode_modules[17][2] PROGMEM = {
    {11, 12},   /* MODE_WAVES   */
    { 3,  4},   /* MODE_STROKE  */
    { 5,  8},   /* MODE_CLIMB   */
    {13, 33},   /* MODE_COMBO   */
    {14,  2},   /* MODE_INTENSE */
    {15, 0xFF}, /* MODE_RHYTHM  */
    {23, 0xFF}, /* MODE_AUDIO1  */
    {23, 0xFF}, /* MODE_AUDIO2  */
    {34, 0xFF}, /* MODE_AUDIO3  */
    {0xFF,0xFF},/* MODE_RANDOM1 */
    {32, 0xFF}, /* MODE_RANDOM2 */
    {18, 0xFF}, /* MODE_TOGGLE  */
    {24, 0xFF}, /* MODE_ORGASM  */
    {28, 0xFF}, /* MODE_TORMENT */
    {20, 21},   /* MODE_PHASE1  */
    {20, 21},   /* MODE_PHASE2  */
    {22, 0xFF}, /* MODE_PHASE3  */
};

static void setup_mode_modules(uint8_t mode) {
    if (mode >= MODE_USER1 && mode < MODE_SPLIT) {
        user_prog_execute(mode - MODE_USER1);
        return;
    }

    if (mode >= MODE_SPLIT) {
        channel_a.gate_value = 0x07;
        channel_b.gate_value = 0x07;
        return;
    }

    uint8_t m0 = pgm_read_byte(&mode_modules[mode][0]);
    uint8_t m1 = pgm_read_byte(&mode_modules[mode][1]);
    if (m0 != 0xFF) execute_module(m0);
    if (m1 != 0xFF) execute_module(m1);

    if (mode == MODE_PHASE2) execute_module(35);

    if (mode == MODE_AUDIO1) {
        channel_a.gate_value = 0x47;
        channel_b.gate_value = 0x47;
        channel_a.output_control_flags = 0x40;
    } else if (mode == MODE_AUDIO2) {
        channel_a.gate_value = 0x47;
        channel_b.gate_value = 0x47;
    } else if (mode == MODE_AUDIO3) {
        channel_a.gate_value = 0x67;
        channel_b.gate_value = 0x67;
        channel_a.output_control_flags = 0x04;
    } else if (mode == MODE_PHASE1 || mode == MODE_PHASE2) {
        channel_a.output_control_flags = 0x05;
    }
}

static void apply_mode_init(ChannelBlock *ch) {
    ch->ramp_min  = 0x9C;
    ch->ramp_max  = 0xFF;
    ch->ramp_rate = 0x07;
    ch->ramp_step = 0x01;
    ch->ramp_action_min = 0xFC;
    ch->ramp_action_max = 0xFC;
    ch->ramp_select = 0x01;
    ch->ramp_timer = 0;

    ch->intensity_value = 0xFF;
    ch->intensity_select = 0x00;

    ch->freq_select = 0x08;

    ch->width_select = 0x00;

    
}

static void init_mode_modules(uint8_t mode) {
    channel_load_defaults(&channel_a);
    channel_load_defaults(&channel_b);
    apply_mode_init(&channel_a);
    apply_mode_init(&channel_b);
    channel_a.apply_channel = 0x03;
    execute_module(1);
    setup_mode_modules(mode);
    channel_a.apply_channel = 0x03;
}

static void init_split_mode(void) {
    ChannelBlock saved_a, saved_b;

    channel_load_defaults(&channel_a);
    channel_load_defaults(&channel_b);
    apply_mode_init(&channel_a);
    apply_mode_init(&channel_b);
    channel_a.apply_channel = 0x01;
    execute_module(1);
    setup_mode_modules(split_mode_a);
    memcpy(&saved_a, &channel_a, sizeof(ChannelBlock));

    channel_load_defaults(&channel_a);
    channel_load_defaults(&channel_b);
    apply_mode_init(&channel_a);
    apply_mode_init(&channel_b);
    channel_a.apply_channel = 0x02;
    execute_module(1);
    setup_mode_modules(split_mode_b);
    memcpy(&saved_b, &channel_b, sizeof(ChannelBlock));

    memcpy(&channel_a, &saved_a, sizeof(ChannelBlock));
    memcpy(&channel_b, &saved_b, sizeof(ChannelBlock));
    channel_a.apply_channel = 0x03;
}

/* Random 1 state */
static uint16_t r1_target = 0;

static void random1_init(void) {
    r1_target = 0;
}

static void random1_tick(void) {
    uint16_t master = param_engine_get_master_timer();

    if (r1_target == 0) {
        uint8_t offset = 0x14 + (prng_next() % (0x78 - 0x14 + 1));
        r1_target = master + offset;
        return;
    }

    if (master >= r1_target) {
        r1_target = 0;
        static const uint8_t random1_modes[] PROGMEM = {
            MODE_WAVES, MODE_STROKE, MODE_CLIMB,
            MODE_COMBO, MODE_INTENSE, MODE_RHYTHM
        };
        uint8_t new_mode = pgm_read_byte(&random1_modes[prng_next() % 6]);

        uint8_t rand184 = 0x8C + (prng_next() % (0xB8 - 0x8C + 1));
        channel_b.cond_module = rand184;

        mode_dispatcher_select_mode(new_mode);
    }
}

static void update_output_flags(void) {
    g_mk312bt_state.output_control_flags = channel_a.output_control_flags;
}

void mode_dispatcher_pause(void) {
    dispatcher_paused = 1;
}

void mode_dispatcher_resume(void) {
    dispatcher_paused = 0;
}

void mode_dispatcher_init(void) {
    current_mode = MODE_WAVES;
    split_mode_a = MODE_WAVES;
    split_mode_b = MODE_WAVES;
    dispatcher_paused = 0;
    eeprom_load_split_modes(&split_mode_a, &split_mode_b);
    user_programs_init();
    channel_mem_init();
    param_engine_init();
    random1_init();
}

void mode_dispatcher_set_split_modes(uint8_t mode_a, uint8_t mode_b) {
    if (mode_a >= MODE_SPLIT) mode_a = MODE_WAVES;
    if (mode_b >= MODE_SPLIT) mode_b = MODE_WAVES;
    split_mode_a = mode_a;
    split_mode_b = mode_b;
    eeprom_save_split_modes(mode_a, mode_b);
}

void mode_dispatcher_select_mode(uint8_t mode_number) {
    if (mode_number >= MODE_COUNT) mode_number = 0;

    dac_update_both_channels(DAC_MAX_VALUE, DAC_MAX_VALUE);
    _delay_ms(2);
    pulse_set_gate_a(PULSE_OFF);
    pulse_set_gate_b(PULSE_OFF);

    current_mode = mode_number;
    param_engine_init();

    if (mode_number == MODE_RANDOM1) {
        channel_mem_init();
        random1_init();
    } else if (mode_number == MODE_SPLIT) {
        init_split_mode();
    } else {
        init_mode_modules(mode_number);
    }

    param_engine_init_directions();

    update_output_flags();
}

void mode_dispatcher_update(void) {
    if (dispatcher_paused) return;

    if (current_mode == MODE_RANDOM1) {
        random1_tick();
    }

    param_engine_tick();

    uint8_t mod_a = param_engine_check_module_trigger(&channel_a);
    uint8_t mod_b = param_engine_check_module_trigger(&channel_b);

    if (mod_a != 0xFF && mod_a < MODULE_COUNT) {
        execute_module(mod_a);
        param_engine_init_directions();
    }
    if (mod_b != 0xFF && mod_b < MODULE_COUNT) {
        execute_module(mod_b);
        param_engine_init_directions();
    }

    update_output_flags();
}

uint8_t mode_dispatcher_get_mode(void) {
    return current_mode;
}

uint8_t mode_dispatcher_get_split_mode_a(void) {
    return split_mode_a;
}

uint8_t mode_dispatcher_get_split_mode_b(void) {
    return split_mode_b;
}

void mode_dispatcher_request_mode(uint8_t mode_number) {
    deferred_mode = mode_number;
    deferred_cmd = DEFERRED_SET_MODE;
}

void mode_dispatcher_request_pause(void) {
    deferred_cmd = DEFERRED_PAUSE;
}

void mode_dispatcher_request_next_mode(void) {
    deferred_cmd = DEFERRED_NEXT;
}

void mode_dispatcher_request_prev_mode(void) {
    deferred_cmd = DEFERRED_PREV;
}

void mode_dispatcher_request_reload(void) {
    deferred_cmd = DEFERRED_RELOAD;
}

void mode_dispatcher_request_start_ramp(void) {
    deferred_cmd = DEFERRED_START_RAMP;
}

uint8_t mode_dispatcher_poll_deferred(void) {
    uint8_t cmd = deferred_cmd;
    if (cmd == DEFERRED_NONE) return 0;
    deferred_cmd = DEFERRED_NONE;

    switch (cmd) {
        case DEFERRED_SET_MODE:
            mode_dispatcher_select_mode(deferred_mode);
            return 1;
        case DEFERRED_PAUSE:
            mode_dispatcher_pause();
            return 2;
        case DEFERRED_NEXT:
            if (current_mode < MODE_COUNT - 1) {
                mode_dispatcher_select_mode(current_mode + 1);
            }
            return 1;
        case DEFERRED_PREV:
            if (current_mode > 0) {
                mode_dispatcher_select_mode(current_mode - 1);
            }
            return 1;
        case DEFERRED_RELOAD:
            mode_dispatcher_select_mode(current_mode);
            return 1;
        case DEFERRED_START_RAMP:
            return 3;
    }
    return 1;
}
