/*
 * audio_processor.c - Audio Input Envelope Follower
 *
 * Processes audio input from the line-in jacks to modulate channel intensity.
 * Used by Audio1/Audio2/Audio3 modes for sound-reactive output.
 *
 * Signal processing per ET-312 reference:
 *   - Read ADC (0-1023), half-wave rectified at the hardware level
 *   - Divide by two
 *   - If LSB was 1, subtract 0x53
 *   - Clamp to 0-255, write to intensity_value
 */

#include "audio_processor.h"
#include "MK312BT_Constants.h"
#include "channel_mem.h"
#include "config.h"
#include "adc.h"
#include <stdint.h>

void audio_init(void) {
}

void audio_process_channel_a(void) {
    uint16_t adc_val = adc_read_audio_a();   // 0-1023, half-wave rectified by hardware
    uint8_t lsb = adc_val & 1;
    uint16_t half = adc_val >> 1;            // divide by two
    if (lsb) {
        if (half >= 0x53) half -= 0x53;
        else half = 0;
    }
    if (half > 255) half = 255;
    channel_a.intensity_value = (uint8_t)half;
}

void audio_process_channel_b(void) {
    uint16_t adc_val = adc_read_audio_b();
    uint8_t lsb = adc_val & 1;
    uint16_t half = adc_val >> 1;
    if (lsb) {
        if (half >= 0x53) half -= 0x53;
        else half = 0;
    }
    if (half > 255) half = 255;
    channel_b.intensity_value = (uint8_t)half;
}