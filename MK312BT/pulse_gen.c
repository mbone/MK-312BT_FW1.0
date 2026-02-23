/*
 * MK-312BT Biphasic Pulse Generator - Timer Setup & Control
 *
 * Configures Timer1 (16-bit) for Channel A and Timer2 (8-bit) for Channel B.
 * Both run in CTC mode with /8 prescaler giving 1 MHz tick rate (1 us resolution).
 * ISRs in interrupts.c handle the actual pulse generation state machine.
 */

#include "pulse_gen.h"
#include "avr_registers.h"
#include "MK312BT_Constants.h"

volatile ChannelPulseState pulse_ch_a;
volatile ChannelPulseState pulse_ch_b;

void pulse_gen_init(void) {
    pulse_ch_a.gate = PULSE_OFF;
    pulse_ch_a.width_ticks = 100;
    pulse_ch_a.period_ticks = 5000;
    pulse_ch_a.phase = PH_GAP;
    pulse_ch_a.gap_remaining = 0;
    pulse_ch_a.pending_width = 100;
    pulse_ch_a.pending_period = 5000;
    pulse_ch_a.params_dirty = 0;

    pulse_ch_b.gate = PULSE_OFF;
    pulse_ch_b.width_ticks = 100;
    pulse_ch_b.period_ticks = 5000;
    pulse_ch_b.phase = PH_GAP;
    pulse_ch_b.gap_remaining = 0;
    pulse_ch_b.pending_width = 100;
    pulse_ch_b.pending_period = 5000;
    pulse_ch_b.params_dirty = 0;

    /* Ensure all H-bridge pins start LOW (both channels off) */
    PORTB &= ~HBRIDGE_FETS_MASK;

    cli();

    /* Timer1 - Channel A pulse generation (16-bit)
     * CTC mode (WGM12), /8 prescaler (CS11) = 1 MHz tick
     * OCR1A sets the duration of each pulse phase */
    TCNT1H = 0;
    TCNT1L = 0;
    TCCR1A = 0;                              // No PWM output pins
    TCCR1B = (1 << WGM12) | (1 << CS11);    // CTC mode, prescaler /8
    OCR1AH = 0;
    OCR1AL = 250;                            // Initial: 250 us until first ISR
    TIMSK |= (1 << OCIE1A);                  // Enable CompA interrupt

    /* Timer2 - Channel B pulse generation (8-bit)
     * CTC mode (WGM21), /8 prescaler (CS21) = 1 MHz tick
     * Only 8-bit OCR2, so long gaps use multi-step countdown */
    TCNT2 = 0;
    TCCR2 = (1 << WGM21) | (1 << CS21);     // CTC mode, prescaler /8
    OCR2 = 250;                              // Initial: 250 us until first ISR
    TIMSK |= (1 << OCIE2);                   // Enable Comp2 interrupt

    sei();
}

void pulse_set_width_a(uint8_t width_us) {
    if (width_us < 70) width_us = 70;
    uint8_t sreg = SREG;
    cli();
    pulse_ch_a.pending_width = width_us;
    pulse_ch_a.params_dirty = 1;
    SREG = sreg;
}

void pulse_set_width_b(uint8_t width_us) {
    if (width_us < 70) width_us = 70;
    uint8_t sreg = SREG;
    cli();
    pulse_ch_b.pending_width = width_us;
    pulse_ch_b.params_dirty = 1;
    SREG = sreg;
}

/* Turn gate on/off. When turning off, immediately drive H-bridge pins LOW
 * to prevent any residual output between ISR cycles. Both the gate flag
 * write and the pin-clear are done atomically so the ISR cannot fire
 * between them and see an inconsistent state. */
void pulse_set_gate_a(uint8_t on) {
    uint8_t sreg = SREG;
    cli();
    pulse_ch_a.gate = on;
    if (!on) {
        PORTB &= ~((1 << HBRIDGE_CH_A_POS) | (1 << HBRIDGE_CH_A_NEG));
    }
    SREG = sreg;
}

void pulse_set_gate_b(uint8_t on) {
    uint8_t sreg = SREG;
    cli();
    pulse_ch_b.gate = on;
    if (!on) {
        PORTB &= ~((1 << HBRIDGE_CH_B_POS) | (1 << HBRIDGE_CH_B_NEG));
    }
    SREG = sreg;
}

void pulse_set_frequency_a(uint16_t period_us) {
    if (period_us < 500) period_us = 500;
    uint8_t sreg = SREG;
    cli();
    pulse_ch_a.pending_period = period_us;
    pulse_ch_a.params_dirty = 1;
    SREG = sreg;
}

void pulse_set_frequency_b(uint16_t period_us) {
    if (period_us < 500) period_us = 500;
    uint8_t sreg = SREG;
    cli();
    pulse_ch_b.pending_period = period_us;
    pulse_ch_b.params_dirty = 1;
    SREG = sreg;
}
