/*
 * adc.c - Analog-to-Digital Converter Driver
 *
 * Provides synchronous ADC reads for all analog inputs using Arduino's
 * analogRead() which handles MUX selection and conversion automatically.
 *
 * Pin mapping (ATmega16 PA port = Arduino analog pins):
 *   PA4 (A4): Level pot A
 *   PA5 (A5): Level pot B
 *   PA1 (A1): Multi-Adjust (MA) knob
 *   PA7 (A7): Audio input A (right line-in)
 *   PA6 (A6): Audio input B (left line-in / mic)
 *   PA3 (A3): Battery voltage (12V through divider)
 */

#include "adc.h"
#include "MK312BT_Constants.h"
#include "avr_registers.h"


void adc_init(void) {
    /* ADC hardware is configured in initializeHardware().
     * No additional init required when using analogRead(). */
}


uint16_t adc_read_level_a(void) {
    return fastAnalogRead(ADC_CHANNEL_LEVEL_A_PIN);
}

uint16_t adc_read_level_b(void) {
    return fastAnalogRead(ADC_CHANNEL_LEVEL_B_PIN);
}

uint16_t adc_read_audio_a(void) {
    return fastAnalogRead(ADC_AUDIO_A_PIN);
}

uint16_t adc_read_audio_b(void) {
    return fastAnalogRead(ADC_AUDIO_B_PIN);
}

uint16_t adc_read_battery(void) {
    return fastAnalogRead(ADC_BATTERY_PIN);
}

uint16_t fastAnalogRead(uint8_t pin) {
    // Convert Arduino pin number (e.g., A0) to channel (0-7)
    uint8_t channel = pin - PA0;
    if (channel > 7) return 0; // invalid

    // Set reference to AVCC and select channel
    ADMUX = ADC_VREF_AVCC | (channel & 0x07);

    // Start conversion
    ADCSRA |= (1 << ADSC);

    // Wait for completion
    while (ADCSRA & (1 << ADSC));

    // Read result (low then high)
    uint8_t low = ADCL;
    uint8_t high = ADCH;
    return (high << 8) | low;
}
