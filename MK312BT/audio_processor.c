/*
 * audio_processor.c - Audio Input Envelope Follower
 *
 * Processes audio input from the line-in jacks to modulate channel intensity.
 * Used by Audio1/Audio2/Audio3 modes for sound-reactive output.
 *
 * Signal processing:
 *   1. Read 10-bit ADC (0-1023), centered at ~512 (AC-coupled)
 *   2. Rectify: take absolute distance from center (0-512)
 *   3. Apply gain: multiply by audio_gain, shift right 7 (divide by 128)
 *   4. Clamp to 0-255 for the intensity modulation register
 *
 * Each channel (A/B) has a separate audio input:
 *   Channel A: PA7 (right line-in, low-pass filtered)
 *   Channel B: PA6 (left line-in / mic, low-pass filtered)
 */

#include "audio_processor.h"
#include "MK312BT_Constants.h"
#include "channel_mem.h"
#include "config.h"
#include "adc.h"
#include <stdint.h>

void audio_init(void) {
}

/* Rectify, scale, and apply audio input from right line-in to channel A */
void audio_process_channel_a(void) {
    uint16_t audio_input = adc_read_audio_a();
    uint16_t signal = audio_input;
    uint8_t gain = config_get()->audio_gain;

    /* Full-wave rectify around the 512 center point (AC-coupled signal) */
    if (signal > ADC_CENTER_POINT) {
        signal -= ADC_CENTER_POINT;
    } else {
        signal = ADC_CENTER_POINT - signal;
    }

    uint32_t scaled_a = ((uint32_t)signal * gain) >> 7;
    if (scaled_a > 255) scaled_a = 255;

    channel_a.intensity_value = (uint8_t)scaled_a;
}

/* Rectify, scale, and apply audio input from left line-in/mic to channel B */
void audio_process_channel_b(void) {
    uint16_t audio_input = adc_read_audio_b();
    uint16_t signal = audio_input;
    uint8_t gain = config_get()->audio_gain;

    if (signal > ADC_CENTER_POINT) {
        signal -= ADC_CENTER_POINT;
    } else {
        signal = ADC_CENTER_POINT - signal;
    }

    uint32_t scaled_b = ((uint32_t)signal * gain) >> 7;
    if (scaled_b > 255) scaled_b = 255;

    channel_b.intensity_value = (uint8_t)scaled_b;
}
