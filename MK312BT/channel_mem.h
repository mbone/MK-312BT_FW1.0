#ifndef CHANNEL_MEM_H
#define CHANNEL_MEM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t unused_80;
    uint8_t unused_81;
    uint8_t retry_count;            // 0x82
    uint8_t output_control_flags;   // 0x83
    uint8_t cond_module;            // 0x84
    uint8_t apply_channel;          // 0x85: 1=A, 2=B, 3=both
    uint8_t ma_range_high;          // 0x86  MA_RANGE_HIGH: output value when MA knob is at maximum
    uint8_t ma_range_low;           // 0x87  MA_RANGE_LOW:  output value when MA knob is at minimum
    uint8_t routine_timer_lo;       // 0x88
    uint8_t routine_timer_mid;      // 0x89
    uint8_t routine_timer_hi;       // 0x8A
    uint8_t routine_timer_slower;   // 0x8B
    uint8_t bank;                   // 0x8C: temp byte store
    uint8_t random_min;             // 0x8D
    uint8_t random_max;             // 0x8E
    uint8_t audio_trigger_module;   // 0x8F
    uint8_t gate_value;             // 0x90
    uint8_t gate_want_a;            // 0x91
    uint8_t gate_want_b;            // 0x92
    uint8_t unused_93;
    uint8_t next_module_timer_cur;  // 0x94
    uint8_t next_module_timer_max;  // 0x95
    uint8_t next_module_select;     // 0x96
    uint8_t next_module_number;     // 0x97
    uint8_t gate_ontime;            // 0x98
    uint8_t gate_offtime;           // 0x99
    uint8_t gate_select;            // 0x9A
    uint8_t gate_transitions;       // 0x9B
    uint8_t ramp_value;             // 0x9C
    uint8_t ramp_min;               // 0x9D
    uint8_t ramp_max;               // 0x9E
    uint8_t ramp_rate;              // 0x9F
    uint8_t ramp_step;              // 0xA0
    uint8_t ramp_action_min;        // 0xA1
    uint8_t ramp_action_max;        // 0xA2
    uint8_t ramp_select;            // 0xA3
    uint8_t ramp_timer;             // 0xA4
    uint8_t intensity_value;        // 0xA5
    uint8_t intensity_min;          // 0xA6
    uint8_t intensity_max;          // 0xA7
    uint8_t intensity_rate;         // 0xA8
    uint8_t intensity_step;         // 0xA9
    uint8_t intensity_action_min;   // 0xAA
    uint8_t intensity_action_max;   // 0xAB
    uint8_t intensity_select;       // 0xAC
    uint8_t intensity_timer;        // 0xAD
    uint8_t freq_value;             // 0xAE
    uint8_t freq_min;               // 0xAF
    uint8_t freq_max;               // 0xB0
    uint8_t freq_rate;              // 0xB1
    uint8_t freq_step;              // 0xB2
    uint8_t freq_action_min;        // 0xB3
    uint8_t freq_action_max;        // 0xB4
    uint8_t freq_select;            // 0xB5
    uint8_t freq_timer;             // 0xB6
    uint8_t width_value;            // 0xB7
    uint8_t width_min;              // 0xB8
    uint8_t width_max;              // 0xB9
    uint8_t width_rate;             // 0xBA
    uint8_t width_step;             // 0xBB
    uint8_t width_action_min;       // 0xBC
    uint8_t width_action_max;       // 0xBD
    uint8_t width_select;           // 0xBE
    uint8_t width_timer;            // 0xBF
} ChannelBlock;

#define CHAN_BASE_A  0x080
#define CHAN_BASE_B  0x180
#define CHAN_BLOCK_SIZE 64

#define ACTION_REVERSE    0xFF
#define ACTION_REV_TOGGLE 0xFE
#define ACTION_LOOP       0xFD
#define ACTION_STOP       0xFC
#define ACTION_IS_MODULE(a) ((a) < 0xFC)

#define SEL_TIMER_MASK    0x03
#define SEL_TIMER_NONE    0x00
#define SEL_TIMER_244HZ   0x01
#define SEL_TIMER_30HZ    0x02
#define SEL_TIMER_1HZ     0x03

#define GATE_ON_BIT       0x01
#define GATE_POL_MASK     0x06
#define GATE_POL_NONE     0x00
#define GATE_POL_NEG      0x02
#define GATE_POL_POS      0x04
#define GATE_POL_BIPHASIC 0x06
#define GATE_ALT_POL      0x08
#define GATE_INV_POL      0x10
#define GATE_OFF_FROM_TEMPO  0x04
#define GATE_OFF_FROM_MA     0x08
#define GATE_AUDIO_FREQ      0x20
#define GATE_ON_FROM_EFFECT  0x20
#define GATE_ON_FROM_MA      0x40
#define GATE_AUDIO_INT       0x40

extern ChannelBlock channel_a;
extern ChannelBlock channel_b;

void channel_mem_init(void);
void channel_load_defaults(ChannelBlock *ch);

uint8_t* channel_get_reg_ptr(uint16_t addr);

#ifdef __cplusplus
}
#endif

#endif
