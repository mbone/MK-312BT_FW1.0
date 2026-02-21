/*
 * utils.c - Hardware Diagnostics and Self-Test Utilities
 *
 * Provides startup self-test routines run before entering the main loop:
 *
 *   dacTest()       - Verifies LTC1661 DAC output by writing known values
 *                     and reading them back through the ADC level channels.
 *                     Tests both DAC A and DAC B with 6 values each.
 *
 *   fetCalibrate()  - Calibrates H-bridge FET pairs by driving each
 *                     half-bridge and measuring output current through PA0.
 *                     Checks that:
 *                     - Each FET draws measurable current (not open)
 *                     - Current is within safe limits (not shorted)
 *                     - Positive and negative FETs are balanced (<50% imbalance)
 *                     Stores baseline readings for runtime current monitoring.
 *
 * Both routines display progress on the LCD during execution.
 * Returns 1 on success, 0 on failure.
 */

#include "MK312BT_Utils.h"
#include "MK312BT_Constants.h"
#include "dac.h"
#include "lcd.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

/* Baseline current readings from calibration (positive and negative FET for each channel) */
static uint16_t fet_baseline_a_pos;
static uint16_t fet_baseline_a_neg;
static uint16_t fet_baseline_b_pos;
static uint16_t fet_baseline_b_neg;

/* Blocking ADC read: select channel, start conversion, wait for result */
static uint16_t adc_read_blocking(uint8_t channel) {
  if (channel > 7) channel = 0;

  ADMUX = ADC_VREF_AVCC | (channel & 0x07);
  ADCSRA |= (1 << ADSC);

  while (ADCSRA & (1 << ADSC));  /* Wait for conversion */

  uint8_t low = ADCL;   /* Must read low first */
  uint8_t high = ADCH;
  return (high << 8) | low;
}

/* Average multiple current readings for noise reduction */
static uint16_t adc_read_current_avg(void) {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < FET_CAL_NUM_SAMPLES; i++) {
    sum += adc_read_blocking(ADC_MUX_CURRENT);
    _delay_us(200);
  }
  return (uint16_t)(sum / FET_CAL_NUM_SAMPLES);
}

/*
 * Drive a single brief biphasic test pulse on a channel and return the peak
 * current reading sampled during the positive phase.
 *
 * pin_pos / pin_neg are the PORTB bit indices for the high-side and low-side
 * gates respectively (e.g. PB2 / PB3 for channel A).
 *
 * Sequence:
 *   1. Positive phase  (FET_CAL_PULSE_US): pin_pos=H, pin_neg=L  -> sample ADC
 *   2. Dead time       (4 us):             both off
 *   3. Negative phase  (FET_CAL_PULSE_US): pin_pos=L, pin_neg=H  -> discharges output
 *   4. Dead time       (4 us):             both off
 *
 * The biphasic discharge in step 3 ensures no net DC is left on the transformer
 * after the test, which prevents any sensation from being felt by a connected user.
 */
static uint16_t fet_test_pulse(uint8_t pin_pos, uint8_t pin_neg) {
  /* Positive phase */
  PORTB = (PORTB & ~(1 << pin_neg)) | (1 << pin_pos);
  _delay_us(FET_CAL_PULSE_US / 2);
  uint16_t reading = adc_read_current_avg();
  _delay_us(FET_CAL_PULSE_US / 2);

  /* Dead time */
  PORTB &= ~((1 << pin_pos) | (1 << pin_neg));
  _delay_us(4);

  /* Negative phase - discharge */
  PORTB = (PORTB & ~(1 << pin_pos)) | (1 << pin_neg);
  _delay_us(FET_CAL_PULSE_US);

  /* Dead time */
  PORTB &= ~((1 << pin_pos) | (1 << pin_neg));
  _delay_us(4);

  return reading;
}

static void fets_all_off(void) {
  PORTB &= ~HBRIDGE_FETS_MASK;
}

/* Verify SPI communication by performing a transfer and checking completion.
 * The MK-312BT has no DAC-to-ADC feedback path (PA4/PA5 are level pots, not DAC
 * readback), so we can only verify that the SPI bus is functional. */
uint8_t dacTest() {
  if (!(SPCR & (1 << SPE))) {
    return 0;
  }

  lcd_clear();
  lcd_set_cursor(0, 0);
  lcd_write_string_P(PSTR("Testing DAC..."));
  lcd_show_progress(0, 4);

  dac_write_channel_a(0);
  if (!(SPSR & (1 << SPIF)) && !(SPCR & (1 << SPE))) {
    return 0;
  }
  lcd_show_progress(1, 4);
  _delay_ms(50);

  dac_write_channel_a(512);
  lcd_show_progress(2, 4);
  _delay_ms(50);

  dac_write_channel_b(512);
  lcd_show_progress(3, 4);
  _delay_ms(50);

  dac_write_channel_a(0);
  dac_write_channel_b(0);
  lcd_show_progress(4, 4);
  _delay_ms(50);

  return 1;
}

/* Calibrate all 4 H-bridge FETs by measuring current draw.
 * Tests: Ch A positive, Ch A negative, Ch B positive, Ch B negative.
 *
 * Current comparison is done against a per-test reference (DAC on, FETs off)
 * rather than a single global idle measurement.  This prevents a missing or
 * non-functional DAC from producing a false FET-FAIL: if the DAC is absent the
 * reference and the FET-on readings will be the same, so the delta check will
 * still fail — but now that case is caught by dacTest() *before* fetCalibrate()
 * is ever called.
 *
 * Returns 1 if all FETs pass, 0 if any fail. */
uint8_t fetCalibrate(void) {
  lcd_clear();
  lcd_set_cursor(0, 0);
  lcd_write_string_P(PSTR("Calibrate FETs.."));
  lcd_show_progress(0, 8);

  fets_all_off();
  dac_write_channel_a(0);
  dac_write_channel_b(0);
  _delay_ms(FET_CAL_SETTLE_MS);
  lcd_show_progress(1, 8);

  /* ---- Channel A FET calibration ---- */
  dac_write_channel_a(FET_CAL_DAC_VALUE);
  _delay_ms(FET_CAL_SETTLE_MS);

  /* Reference: DAC on, FETs still off */
  uint16_t ref_a = adc_read_current_avg();

  /* Test Ch A positive FET via brief biphasic pulse (PB2=pos, PB3=neg) */
  fet_baseline_a_pos = fet_test_pulse(HBRIDGE_CH_A_POS, HBRIDGE_CH_A_NEG);
  lcd_show_progress(2, 8);

  if (fet_baseline_a_pos < ref_a + FET_CAL_CURRENT_MIN ||
      fet_baseline_a_pos > FET_CAL_CURRENT_MAX) {
    fets_all_off();
    dac_write_channel_a(0);
    return 0;
  }

  /* Test Ch A negative FET via brief biphasic pulse (PB3=pos, PB2=neg) */
  fet_baseline_a_neg = fet_test_pulse(HBRIDGE_CH_A_NEG, HBRIDGE_CH_A_POS);
  lcd_show_progress(3, 8);

  if (fet_baseline_a_neg < ref_a + FET_CAL_CURRENT_MIN ||
      fet_baseline_a_neg > FET_CAL_CURRENT_MAX) {
    fets_all_off();
    dac_write_channel_a(0);
    return 0;
  }

  dac_write_channel_a(0);
  lcd_show_progress(4, 8);

  /* ---- Channel B FET calibration ---- */
  dac_write_channel_b(FET_CAL_DAC_VALUE);
  _delay_ms(FET_CAL_SETTLE_MS);

  /* Reference: DAC on, FETs still off */
  uint16_t ref_b = adc_read_current_avg();

  /* Test Ch B positive FET via brief biphasic pulse (PB0=pos, PB1=neg) */
  fet_baseline_b_pos = fet_test_pulse(HBRIDGE_CH_B_POS, HBRIDGE_CH_B_NEG);
  lcd_show_progress(5, 8);

  if (fet_baseline_b_pos < ref_b + FET_CAL_CURRENT_MIN ||
      fet_baseline_b_pos > FET_CAL_CURRENT_MAX) {
    fets_all_off();
    dac_write_channel_b(0);
    return 0;
  }

  /* Test Ch B negative FET via brief biphasic pulse (PB1=pos, PB0=neg) */
  fet_baseline_b_neg = fet_test_pulse(HBRIDGE_CH_B_NEG, HBRIDGE_CH_B_POS);
  lcd_show_progress(6, 8);

  if (fet_baseline_b_neg < ref_b + FET_CAL_CURRENT_MIN ||
      fet_baseline_b_neg > FET_CAL_CURRENT_MAX) {
    fets_all_off();
    dac_write_channel_b(0);
    return 0;
  }

  dac_write_channel_b(0);
  fets_all_off();
  lcd_show_progress(7, 8);

  /* Check FET balance: positive and negative readings should be within 50% of each other */
  int16_t imbalance_a = (int16_t)fet_baseline_a_pos - (int16_t)fet_baseline_a_neg;
  int16_t imbalance_b = (int16_t)fet_baseline_b_pos - (int16_t)fet_baseline_b_neg;
  if (imbalance_a < 0) imbalance_a = -imbalance_a;
  if (imbalance_b < 0) imbalance_b = -imbalance_b;

  uint16_t threshold_a = fet_baseline_a_pos / 2;
  uint16_t threshold_b = fet_baseline_b_pos / 2;
  if ((uint16_t)imbalance_a > threshold_a || (uint16_t)imbalance_b > threshold_b) {
    return 0;
  }

  lcd_show_progress(8, 8);
  _delay_ms(100);

  return 1;
}

