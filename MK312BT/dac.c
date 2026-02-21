/*
 * LTC1661 Dual 10-bit DAC Driver
 *
 * Controls output intensity for both channels via SPI.
 * The DAC output voltage scales the transformer drive level:
 *   - DAC value 1023 = minimum output (off)
 *   - DAC value 0    = maximum output (full power)
 *   (Inverted relationship: higher DAC = lower intensity)
 *
 * SPI protocol: 16-bit transfers, MSB first
 *   Byte 1: [CMD3:CMD0][D9:D6]
 *   Byte 2: [D5:D0][XX]
 *
 * Chip select: PD4 (DAC_CS_LD), active low
 * SPI clock: F_CPU/16 = 500 kHz
 *
 * Command codes (from MK312BT_Constants.h):
 *   DAC_CMD_LOUPA  - Load and update DAC A
 *   DAC_CMD_LOUPB  - Load and update DAC B
 *   DAC_CMD_LOAD_A - Load DAC A (no update)
 *   DAC_CMD_LOAD_B - Load DAC B (no update)
 *   DAC_CMD_UPDATE - Update both DACs simultaneously
 *   DAC_CMD_WAKE   - Wake from sleep mode
 *   DAC_CMD_SLEEP  - Enter sleep mode
 */

#include "dac.h"
#include "avr_registers.h"
#include "MK312BT_Constants.h"
#include <util/delay.h>

static inline void dac_cs_low(void) {
    PORTD &= ~(1 << DAC_CS_LD);
}

static inline void dac_cs_high(void) {
    PORTD |= (1 << DAC_CS_LD);
}

/* Blocking SPI transfer - waits for completion */
static void dac_spi_transfer(uint8_t data) {
    SPDR = data;
    while (!(SPSR & (1 << SPIF)));
}

/* Send a 16-bit command+data word to the LTC1661.
 * Format: [cmd | data_hi][data_lo] with 10-bit value left-shifted by 2 */
static void dac_send_word(uint8_t cmd, uint16_t value) {
    if (value > DAC_MAX_VALUE) value = DAC_MAX_VALUE;
    uint8_t hi = cmd | ((value >> 6) & 0x0F);
    uint8_t lo = (uint8_t)((value & 0x3F) << 2);
    dac_cs_low();
    _delay_us(1);
    dac_spi_transfer(hi);
    dac_spi_transfer(lo);
    _delay_us(1);
    dac_cs_high();
}

/* Initialize SPI and wake up the DAC from sleep */
void dac_init(void) {
    DDRD |= (1 << DAC_CS_LD);               // CS pin as output
    dac_cs_high();                            // Deselect DAC
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);  // SPI master, /16 clock
    SPSR = 0x00;
    dac_send_word(DAC_CMD_WAKE, 0);          // Wake from power-down
}

/* Load and immediately update a single channel.
 * Note: LTC1661 DAC-A output is wired to Channel B transformer on the
 * MK-312BT PCB, and DAC-B output to Channel A transformer. Commands are
 * swapped here so that logical channel A maps to physical output A. */
void dac_write_channel_a(uint16_t value) {
    dac_send_word(DAC_CMD_LOUPB, value);
}

void dac_write_channel_b(uint16_t value) {
    dac_send_word(DAC_CMD_LOUPA, value);
}

/* Load without updating (for simultaneous update of both channels).
 * DAC-A/B are swapped to match PCB wiring (see dac_write_channel_a). */
void dac_load_a(uint16_t value) {
    dac_send_word(DAC_CMD_LOAD_B, value);
}

void dac_load_b(uint16_t value) {
    dac_send_word(DAC_CMD_LOAD_A, value);
}

/* Update both DAC outputs simultaneously from previously loaded values */
void dac_update(void) {
    dac_send_word(DAC_CMD_UPDATE, 0);
}

/* Atomic update of both channels: load A, load B, then update together */
void dac_update_both_channels(uint16_t value_a, uint16_t value_b) {
    dac_load_a(value_a);
    dac_load_b(value_b);
    dac_update();
}

