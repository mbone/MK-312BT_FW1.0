#include "channel_mem.h"
#include <avr/pgmspace.h>
#include <string.h>

ChannelBlock channel_a;
ChannelBlock channel_b;

static const uint8_t channel_defaults[CHAN_BLOCK_SIZE] PROGMEM = {
    0x00,       // +00  0x80  unused
    0x00,       // +01  0x81  unused
    0x02,       // +02  0x82  retry_count
    0x00,       // +03  0x83  output_control_flags
    0x00,       // +04  0x84  cond_module
    0x03,       // +05  0x85  apply_channel = both
    0x01,       // +06  0x86  ma_range_Min = 1    (value when MA at min: slowest rate)  
    0xFF,       // +07  0x87  ma_range_Max  = 255 (value when MA at max: fastest rate) 
    0x00,       // +08  0x88  routine_timer_lo
    0x00,       // +09  0x89  routine_timer_mid
    0x00,       // +0A  0x8A  routine_timer_hi
    0x00,       // +0B  0x8B  routine_timer_slower
    0x00,       // +0C  0x8C  bank = 0
    0x00,       // +0D  0x8D  random_min = 0
    0x08,       // +0E  0x8E  random_max = 8
    0x00,       // +0F  0x8F  audio_trigger_module
    0x07,       // +10  0x90  gate_value = 0x07 (biphasic, gate ON)
    0x00,       // +11  0x91  gate_want_a
    0x00,       // +12  0x92  gate_want_b
    0x00,       // +13  0x93  unused
    0x00,       // +14  0x94  next_module_timer_cur
    0xFF,       // +15  0x95  next_module_timer_max = 255
    0x00,       // +16  0x96  next_module_select
    0x00,       // +17  0x97  next_module_number
    0x3E,       // +18  0x98  gate_ontime = 62
    0x3E,       // +19  0x99  gate_offtime = 62
    0x00,       // +1A  0x9A  gate_select = 0 (no timer)
    0x00,       // +1B  0x9B  gate_transitions
    0x9C,       // +1C  0x9C  ramp_value = 156
    0x9C,       // +1D  0x9D  ramp_min = 156
    0xFF,       // +1E  0x9E  ramp_max = 255
    0x07,       // +1F  0x9F  ramp_rate = 7
    0x01,       // +20  0xA0  ramp_step = 1
    0xFC,       // +21  0xA1  ramp_action_min = STOP
    0xFC,       // +22  0xA2  ramp_action_max = STOP
    0x01,       // +23  0xA3  ramp_select = timer 244Hz
    0x00,       // +24  0xA4  ramp_timer
    0xFF,       // +25  0xA5  intensity_value = 255
    0xCD,       // +26  0xA6  intensity_min = 205 (power-on default per firmware)
    0xFF,       // +27  0xA7  intensity_max = 255
    0x01,       // +28  0xA8  intensity_rate = 1 (power-on default per firmware)
    0x01,       // +29  0xA9  intensity_step = 1
    0xFF,       // +2A  0xAA  intensity_action_min = REVERSE
    0xFF,       // +2B  0xAB  intensity_action_max = REVERSE
    0x00,       // +2C  0xAC  intensity_select = 0 (no timer)
    0x00,       // +2D  0xAD  intensity_timer
    0x16,       // +2E  0xAE  freq_value = 22
    0x09,       // +2F  0xAF  freq_min = 9
    0x64,       // +30  0xB0  freq_max = 100 (power-on default per firmware)
    0x01,       // +31  0xB1  freq_rate = 1 (power-on default per firmware)
    0x01,       // +32  0xB2  freq_step = 1
    0xFF,       // +33  0xB3  freq_action_min = REVERSE
    0xFF,       // +34  0xB4  freq_action_max = REVERSE
    0x08,       // +35  0xB5  freq_select = 0x08 (no timer, MA knob)
    0x00,       // +36  0xB6  freq_timer
    0x82,       // +37  0xB7  width_value = 130
    0x32,       // +38  0xB8  width_min = 50 (power-on default per firmware)
    0xC8,       // +39  0xB9  width_max = 200 (power-on default per firmware)
    0x01,       // +3A  0xBA  width_rate = 1 (power-on default per firmware)
    0x01,       // +3B  0xBB  width_step = 1
    0xFF,       // +3C  0xBC  width_action_min = REVERSE
    0xFF,       // +3D  0xBD  width_action_max = REVERSE
    0x04,       // +3E  0xBE  width_select = 0x04 (no timer, advanced default)
    0x00,       // +3F  0xBF  width_timer
};

void channel_load_defaults(ChannelBlock *ch) {
    memcpy_P(ch, channel_defaults, CHAN_BLOCK_SIZE);
}

void channel_mem_init(void) {
    channel_load_defaults(&channel_a);
    channel_load_defaults(&channel_b);
}

static uint8_t scratch_byte;

uint8_t* channel_get_reg_ptr(uint16_t addr) {
    uint8_t offset;
    ChannelBlock *ch;

    if (addr >= CHAN_BASE_B && addr < CHAN_BASE_B + CHAN_BLOCK_SIZE) {
        ch = &channel_b;
        offset = addr - CHAN_BASE_B;
    } else if (addr >= CHAN_BASE_A && addr < CHAN_BASE_A + CHAN_BLOCK_SIZE) {
        ch = &channel_a;
        offset = addr - CHAN_BASE_A;
    } else {
        scratch_byte = 0;
        return &scratch_byte;
    }

    return ((uint8_t*)ch) + offset;
}
