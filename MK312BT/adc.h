/*
 * adc.h - Analog-to-Digital Converter Driver
 *
 * Round-robin ADC sampling of 6 analog inputs: two level pots,
 * Multi-Adjust knob, two audio inputs, and battery voltage.
 * All results are 10-bit (0-1023), cached for non-blocking reads.
 */

#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include "MK312BT_Constants.h"

#ifdef __cplusplus
extern "C" {
#endif

void adc_init(void);              /* Initialize ADC hardware */

/* Cached result accessors (10-bit, 0-1023) */
uint16_t adc_read_level_a(void);
uint16_t adc_read_level_b(void);
uint16_t adc_read_audio_a(void);
uint16_t adc_read_audio_b(void);
uint16_t adc_read_battery(void);

#ifdef __cplusplus
}
#endif

#endif
