/*
 * LTC1661 Dual 10-bit DAC Driver
 *
 * Controls output intensity (power supply voltage) for both channels.
 * Higher DAC value = lower output intensity (inverted).
 * DAC 1023 = off, DAC 0 = maximum power.
 * Communication via SPI with chip select on PD4.
 */
#ifndef DAC_H
#define DAC_H

#include <stdint.h>
#include "MK312BT_Constants.h"

#ifdef __cplusplus
extern "C" {
#endif

void dac_init(void);                          // Init SPI and wake DAC
void dac_write_channel_a(uint16_t value);     // Load + update Ch A
void dac_write_channel_b(uint16_t value);     // Load + update Ch B
void dac_load_a(uint16_t value);              // Load Ch A (no update)
void dac_load_b(uint16_t value);              // Load Ch B (no update)
void dac_update(void);                        // Update both from loaded values
void dac_update_both_channels(uint16_t value_a, uint16_t value_b);  // Atomic both

#ifdef __cplusplus
}
#endif

#endif
