/*
 * param_engine.c - Timer-Driven Parameter Modulation Engine
 *
 * (Original header omitted for brevity, but keep your existing header.)
 * 
 * ADDED: master_timer (16-bit) counting at 1.91 Hz (every 128 ticks)
 * for Random 1 mode.
 */

#include "param_engine.h"
#include "channel_mem.h"
#include "config.h"
#include "prng.h"
#include <stddef.h>

static uint8_t tick_counter;
static uint8_t pending_module_a;
static uint8_t pending_module_b;

static uint16_t master_timer = 0;          // 1.91 Hz timer (every 128 ticks)

#define DIR_UP   0
#define DIR_DOWN 1

#define DIR_BIT_RAMP       0x01
#define DIR_BIT_INTENSITY  0x02
#define DIR_BIT_FREQ       0x04
#define DIR_BIT_WIDTH      0x08

static uint8_t dir_flags_a;
static uint8_t dir_flags_b;

static uint8_t gate_phase_a = 0;
static uint8_t gate_timer_a = 0;

static uint8_t gate_phase_b = 0;
static uint8_t gate_timer_b = 0;

typedef struct {
    uint8_t value;
    uint8_t min;
    uint8_t max;
    uint8_t rate;
    uint8_t step;
    uint8_t action_min;
    uint8_t action_max;
    uint8_t select;
    uint8_t timer;
} ParamGroup;

static uint8_t map_ma(uint8_t ma_raw, uint8_t ma_high, uint8_t ma_low) {
    if (ma_high >= ma_low) {
        return ma_low + (uint8_t)(((uint16_t)ma_raw * (ma_high - ma_low)) / 255);
    }
    return ma_low - (uint8_t)(((uint16_t)ma_raw * (ma_low - ma_high)) / 255);
}

static uint8_t resolve_source(uint8_t index, uint8_t own_val,
                               uint8_t adv_val, uint8_t ma_scaled,
                               uint8_t other_val) {
    uint8_t base = index & 0x03;
    uint8_t val;
    switch (base) {
        case 0: val = own_val;   break;
        case 1: val = adv_val;   break;
        case 2: val = ma_scaled; break;
        case 3: val = other_val; break;
        default: val = own_val;  break;
    }
    if (index & 0x04) val = ~val;
    return val;
}

static uint8_t timer_fires(uint8_t timer_sel) {
    switch (timer_sel) {
        case SEL_TIMER_244HZ: return 1;
        case SEL_TIMER_30HZ:  return (tick_counter & 0x07) == 0;
        case SEL_TIMER_1HZ:   return tick_counter == 0;
        default:              return 0;
    }
}

static uint8_t infer_direction(ParamGroup *g) {
    uint8_t lo = g->min;
    uint8_t hi = g->max;
    if (lo > hi) { uint8_t t = lo; lo = hi; hi = t; }

    if (hi == lo) return DIR_UP;
    uint8_t dist_to_max = (g->value <= hi) ? (hi - g->value) : 0;
    uint8_t dist_to_min = (g->value >= lo) ? (g->value - lo) : 0;

    if (g->value >= hi) return DIR_DOWN;
    if (g->value <= lo) return DIR_UP;
    if (dist_to_max <= dist_to_min) return DIR_UP;
    return DIR_DOWN;
}

static uint8_t do_action(uint8_t action, ParamGroup *g, ChannelBlock *ch,
                          uint8_t *dir) {
    switch (action) {
        case ACTION_REV_TOGGLE:
            ch->gate_value ^= GATE_ALT_POL;
            /* fall through */
        case ACTION_REVERSE:
            *dir = (*dir == DIR_UP) ? DIR_DOWN : DIR_UP;
            return 0xFF;
        case ACTION_LOOP:
            if (*dir == DIR_UP) {
                g->value = g->min;
            } else {
                g->value = g->max;
            }
            return 0xFF;
        case ACTION_STOP:
            g->select &= ~SEL_TIMER_MASK;
            return 0xFF;
        default:
            if (ACTION_IS_MODULE(action)) return action;
            return 0xFF;
    }
}

static uint8_t step_group(ParamGroup *g, ChannelBlock *ch, uint8_t ma_scaled,
                           uint8_t adv_min, uint8_t adv_rate,
                           uint8_t other_val, uint8_t *dir) {
    uint8_t sel = g->select;
    uint8_t timer_sel = sel & SEL_TIMER_MASK;

    if (timer_sel == SEL_TIMER_NONE) {
        uint8_t src_bits = (sel >> 2) & 0x07;
        if (src_bits != 0) {
            g->value = resolve_source(src_bits, g->value, adv_min, ma_scaled, other_val);
        }
        return 0xFF;
    }

    if (!timer_fires(timer_sel)) return 0xFF;

    uint8_t rate_idx = (sel >> 5) & 0x07;
    uint8_t effective_rate = resolve_source(rate_idx, g->rate, adv_rate, ma_scaled, other_val);
    if (effective_rate == 0) effective_rate = 1;

    g->timer++;
    if (g->timer < effective_rate) return 0xFF;
    g->timer = 0;

    uint8_t min_idx = (sel >> 2) & 0x07;
    if (min_idx != 0) {
        g->min = resolve_source(min_idx, g->min, adv_min, ma_scaled, other_val);
    }

    uint8_t stp = g->step;
    if (stp == 0) return 0xFF;

    if (*dir == DIR_UP) {
        uint16_t next = (uint16_t)g->value + stp;
        if (next >= (uint16_t)g->max) {
            g->value = g->max;
            return do_action(g->action_max, g, ch, dir);
        }
        g->value = (uint8_t)next;
    } else {
        int16_t next = (int16_t)g->value - stp;
        if (next <= (int16_t)g->min) {
            g->value = g->min;
            return do_action(g->action_min, g, ch, dir);
        }
        g->value = (uint8_t)next;
    }
    return 0xFF;
}

static uint8_t step_channel_group(ChannelBlock *ch, uint8_t offset,
                                   uint8_t ma_scaled, uint8_t adv_min,
                                   uint8_t adv_rate, uint8_t other_val,
                                   uint8_t *dir) {
    ParamGroup *g = (ParamGroup *)(&((uint8_t *)ch)[offset]);
    return step_group(g, ch, ma_scaled, adv_min, adv_rate, other_val, dir);
}

#define OFF_RAMP      (offsetof(ChannelBlock, ramp_value))
#define OFF_INTENSITY (offsetof(ChannelBlock, intensity_value))
#define OFF_FREQ      (offsetof(ChannelBlock, freq_value))
#define OFF_WIDTH     (offsetof(ChannelBlock, width_value))

static uint8_t* get_dir_flags(ChannelBlock *ch) {
    return (ch == &channel_a) ? &dir_flags_a : &dir_flags_b;
}

static void update_gate_timer(ChannelBlock *ch, uint8_t *gt, uint8_t *gp) {
    uint8_t sel = ch->gate_select;
    uint8_t timer_sel = sel & SEL_TIMER_MASK;

    if (timer_sel == SEL_TIMER_NONE) return;
    if (!timer_fires(timer_sel)) return;

    system_config_t *cfg = config_get();
    uint8_t ma_scaled = map_ma(cfg->multi_adjust, ch->ma_range_high, ch->ma_range_low);

    uint8_t ontime = ch->gate_ontime;
    if (sel & GATE_ON_FROM_MA)          ontime = ma_scaled;
    else if (sel & GATE_ON_FROM_EFFECT) ontime = cfg->adv_effect;
    if (ontime == 0) ontime = 1;

    uint8_t offtime = ch->gate_offtime;
    if (sel & GATE_OFF_FROM_MA)         offtime = ma_scaled;
    else if (sel & GATE_OFF_FROM_TEMPO) offtime = cfg->adv_tempo;
    if (offtime == 0) offtime = 1;

    (*gt)++;

    if (*gp == 0) {
        if (*gt >= ontime) {
            *gt = 0;
            *gp = 1;
            ch->gate_value &= ~GATE_ON_BIT;
        }
    } else {
        if (*gt >= offtime) {
            *gt = 0;
            *gp = 0;
            ch->gate_value |= GATE_ON_BIT;
            ch->gate_transitions++;
        }
    }
}

static void step_channel(ChannelBlock *ch, ChannelBlock *other,
                          uint8_t ma_raw, uint8_t *trigger) {
    system_config_t *cfg = config_get();
    uint8_t ma_scaled = map_ma(ma_raw, ch->ma_range_high, ch->ma_range_low);
    uint8_t *flags = get_dir_flags(ch);
    uint8_t m;
    uint8_t dir;

    dir = (*flags & DIR_BIT_RAMP) ? DIR_DOWN : DIR_UP;
    step_channel_group(ch, OFF_RAMP, ma_scaled, cfg->adv_ramp_level,
                       cfg->adv_ramp_time, other->ramp_value, &dir);
    if (dir == DIR_DOWN) *flags |= DIR_BIT_RAMP; else *flags &= ~DIR_BIT_RAMP;

    dir = (*flags & DIR_BIT_INTENSITY) ? DIR_DOWN : DIR_UP;
    m = step_channel_group(ch, OFF_INTENSITY, ma_scaled, cfg->adv_depth,
                            cfg->adv_tempo, other->intensity_value, &dir);
    if (dir == DIR_DOWN) *flags |= DIR_BIT_INTENSITY; else *flags &= ~DIR_BIT_INTENSITY;
    if (m != 0xFF && *trigger == 0xFF) *trigger = m;

    dir = (*flags & DIR_BIT_FREQ) ? DIR_DOWN : DIR_UP;
    m = step_channel_group(ch, OFF_FREQ, ma_scaled, cfg->adv_frequency,
                            cfg->adv_effect, other->freq_value, &dir);
    if (dir == DIR_DOWN) *flags |= DIR_BIT_FREQ; else *flags &= ~DIR_BIT_FREQ;
    if (m != 0xFF && *trigger == 0xFF) *trigger = m;

    dir = (*flags & DIR_BIT_WIDTH) ? DIR_DOWN : DIR_UP;
    m = step_channel_group(ch, OFF_WIDTH, ma_scaled, cfg->adv_width,
                            cfg->adv_pace, other->width_value, &dir);
    if (dir == DIR_DOWN) *flags |= DIR_BIT_WIDTH; else *flags &= ~DIR_BIT_WIDTH;
    if (m != 0xFF && *trigger == 0xFF) *trigger = m;
}

static void step_next_module_timer(ChannelBlock *ch, uint8_t ma_scaled,
                                    uint8_t adv_val, uint8_t other_max,
                                    uint8_t *trigger) {
    uint8_t sel = ch->next_module_select;
    uint8_t timer_sel = sel & SEL_TIMER_MASK;
    if (timer_sel == SEL_TIMER_NONE) return;
    if (!timer_fires(timer_sel)) return;

    uint8_t rate_idx = (sel >> 5) & 0x07;
    uint8_t effective_max = resolve_source(rate_idx, ch->next_module_timer_max,
                                            adv_val, ma_scaled, other_max);
    if (effective_max == 0) effective_max = 1;

    ch->next_module_timer_cur++;
    if (ch->next_module_timer_cur >= effective_max) {
        ch->next_module_timer_cur = 0;
        if (*trigger == 0xFF) {
            *trigger = ch->next_module_number;
        }
    }
}

static uint8_t compute_dir_flags(ChannelBlock *ch) {
    uint8_t flags = 0;
    ParamGroup *g;

    g = (ParamGroup *)(&((uint8_t *)ch)[OFF_RAMP]);
    if (infer_direction(g) == DIR_DOWN) flags |= DIR_BIT_RAMP;

    g = (ParamGroup *)(&((uint8_t *)ch)[OFF_INTENSITY]);
    if (infer_direction(g) == DIR_DOWN) flags |= DIR_BIT_INTENSITY;

    g = (ParamGroup *)(&((uint8_t *)ch)[OFF_FREQ]);
    if (infer_direction(g) == DIR_DOWN) flags |= DIR_BIT_FREQ;

    g = (ParamGroup *)(&((uint8_t *)ch)[OFF_WIDTH]);
    if (infer_direction(g) == DIR_DOWN) flags |= DIR_BIT_WIDTH;

    return flags;
}

void param_engine_init(void) {
    tick_counter = 0;
    master_timer = 0;                        // Initialize master timer
    pending_module_a = 0xFF;
    pending_module_b = 0xFF;
    dir_flags_a = 0;
    dir_flags_b = 0;
    gate_phase_a = 0;
    gate_timer_a = 0;
    gate_phase_b = 0;
    gate_timer_b = 0;
}

void param_engine_init_directions(void) {
    dir_flags_a = compute_dir_flags(&channel_a);
    dir_flags_b = compute_dir_flags(&channel_b);
    gate_phase_a = (channel_a.gate_value & GATE_ON_BIT) ? 0 : 1;
    gate_timer_a = 0;
    gate_phase_b = (channel_b.gate_value & GATE_ON_BIT) ? 0 : 1;
    gate_timer_b = 0;
}

void param_engine_tick(void) {
    tick_counter++;

    // Update 1.91 Hz master timer (every 128 ticks)
    static uint8_t master_sub = 0;
    master_sub++;
    if (master_sub >= 128) {
        master_sub = 0;
        master_timer++;
    }

    update_gate_timer(&channel_a, &gate_timer_a, &gate_phase_a);
    update_gate_timer(&channel_b, &gate_timer_b, &gate_phase_b);

    pending_module_a = 0xFF;
    pending_module_b = 0xFF;

    system_config_t *cfg = config_get();
    uint8_t ma_raw = cfg->multi_adjust;
    uint8_t ma_a = map_ma(ma_raw, channel_a.ma_range_high, channel_a.ma_range_low);
    uint8_t ma_b = map_ma(ma_raw, channel_b.ma_range_high, channel_b.ma_range_low);

    step_channel(&channel_a, &channel_b, ma_raw, &pending_module_a);
    step_next_module_timer(&channel_a, ma_a, cfg->adv_tempo,
                            channel_b.next_module_timer_max, &pending_module_a);

    step_channel(&channel_b, &channel_a, ma_raw, &pending_module_b);
    step_next_module_timer(&channel_b, ma_b, cfg->adv_tempo,
                            channel_a.next_module_timer_max, &pending_module_b);
}

uint8_t param_engine_get_tick(void) {
    return tick_counter;
}

uint8_t param_engine_check_module_trigger(ChannelBlock *ch) {
    if (ch == &channel_a) {
        uint8_t m = pending_module_a;
        pending_module_a = 0xFF;
        return m;
    }
    uint8_t m = pending_module_b;
    pending_module_b = 0xFF;
    return m;
}

// New function to read the master timer
uint16_t param_engine_get_master_timer(void) {
    return master_timer;
}