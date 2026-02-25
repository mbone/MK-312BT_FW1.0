/*
 * MK-312BT Interrupt Service Routines
 *
 * Timer1 CompA ISR: Channel A biphasic pulse generation (PB2/PB3)
 * Timer2 Comp ISR:  Channel B biphasic pulse generation (PB0/PB1)
 * USART RX ISR:     Drain receive buffer to prevent overrun
 * SPI STC ISR:      Drain SPI buffer to prevent overrun
 *
 * Each timer ISR implements a 5-phase state machine:
 *   GAP -> POSITIVE -> DEADTIME1 -> NEGATIVE -> DEADTIME2 -> GAP
 *
 * Dead time (4 us) between polarity transitions prevents H-bridge
 * shoot-through (both FETs conducting simultaneously).
 *
 * Timer1 is 16-bit so full period fits in one OCR1A load.
 * Timer2 is 8-bit so long gaps (>250 us) use gap_remaining counter.
 */

#include <avr/interrupt.h>
#include "pulse_gen.h"
#include "avr_registers.h"
#include "MK312BT_Constants.h"

/* External TX buffer variables from serial.c */
//extern uint8_t tx_buffer[64];
//extern volatile uint8_t tx_head;
//extern volatile uint8_t tx_tail;

static inline void ch_a_all_off(void) {
    PORTB &= ~((1 << HBRIDGE_CH_A_POS) | (1 << HBRIDGE_CH_A_NEG));
}

static inline void ch_a_positive(void) {
    PORTB = (PORTB & ~(1 << HBRIDGE_CH_A_NEG)) | (1 << HBRIDGE_CH_A_POS);
}

static inline void ch_a_negative(void) {
    PORTB = (PORTB & ~(1 << HBRIDGE_CH_A_POS)) | (1 << HBRIDGE_CH_A_NEG);
}

static inline void ch_b_all_off(void) {
    PORTB &= ~((1 << HBRIDGE_CH_B_POS) | (1 << HBRIDGE_CH_B_NEG));
}

static inline void ch_b_positive(void) {
    PORTB = (PORTB & ~(1 << HBRIDGE_CH_B_NEG)) | (1 << HBRIDGE_CH_B_POS);
}

static inline void ch_b_negative(void) {
    PORTB = (PORTB & ~(1 << HBRIDGE_CH_B_POS)) | (1 << HBRIDGE_CH_B_NEG);
}

/* Set 16-bit OCR1A (must write high byte first on ATmega16) */
static inline void set_ocr1a(uint16_t val) {
    OCR1AH = (uint8_t)(val >> 8);
    OCR1AL = (uint8_t)(val & 0xFF);
}

/*
 * Timer1 Compare Match A - Channel A Biphasic Pulse Generator
 *
 * 5-phase state machine. OCR1A is reloaded each transition with the
 * duration of the next phase. Timer1 is 16-bit so full gap fits.
 */
ISR(TIMER1_COMPA_vect) {
    switch (pulse_ch_a.phase) {
        case PH_GAP:
            if (pulse_ch_a.params_dirty) {
                pulse_ch_a.width_ticks = pulse_ch_a.pending_width;
                pulse_ch_a.period_ticks = pulse_ch_a.pending_period;
                pulse_ch_a.params_dirty = 0;
            }
            if (!pulse_ch_a.gate) {
                ch_a_all_off();
                set_ocr1a(250);
                return;
            }
            ch_a_positive();
            set_ocr1a(pulse_ch_a.width_ticks);
            pulse_ch_a.phase = PH_POSITIVE;
            break;

        case PH_POSITIVE:
            ch_a_all_off();               // End positive, start dead time
            set_ocr1a(DEAD_TIME_TICKS);
            pulse_ch_a.phase = PH_DEADTIME1;
            break;

        case PH_DEADTIME1:
            ch_a_negative();              // Start negative half-cycle
            set_ocr1a(pulse_ch_a.width_ticks);
            pulse_ch_a.phase = PH_NEGATIVE;
            break;

        case PH_NEGATIVE:
            ch_a_all_off();               // End negative, start dead time
            set_ocr1a(DEAD_TIME_TICKS);
            pulse_ch_a.phase = PH_DEADTIME2;
            break;

        case PH_DEADTIME2: {
            /* Calculate remaining gap time:
             * gap = period - 2*width - 2*dead_time */
            uint16_t used = (uint16_t)pulse_ch_a.width_ticks * 2 +
                            DEAD_TIME_TICKS * 2;
            uint16_t gap;
            if (pulse_ch_a.period_ticks > used)
                gap = pulse_ch_a.period_ticks - used;
            else
                gap = DEAD_TIME_TICKS;

            set_ocr1a(gap);
            pulse_ch_a.phase = PH_GAP;
            break;
        }
    }
}

/*
 * Timer2 Compare Match - Channel B Biphasic Pulse Generator
 *
 * Same 5-phase state machine as Timer1/Channel A, but Timer2 is only
 * 8-bit (max OCR2 = 255). Long inter-pulse gaps are handled by counting
 * down gap_remaining in 250 us chunks across multiple ISR firings.
 */
ISR(TIMER2_COMP_vect) {
    switch (pulse_ch_b.phase) {
        case PH_GAP:
            if (pulse_ch_b.gap_remaining > 0) {
                uint16_t chunk = pulse_ch_b.gap_remaining;
                if (chunk > 250) chunk = 250;
                OCR2 = (uint8_t)chunk;
                pulse_ch_b.gap_remaining -= chunk;
                return;
            }
            if (pulse_ch_b.params_dirty) {
                pulse_ch_b.width_ticks = pulse_ch_b.pending_width;
                pulse_ch_b.period_ticks = pulse_ch_b.pending_period;
                pulse_ch_b.params_dirty = 0;
            }
            if (!pulse_ch_b.gate) {
                ch_b_all_off();
                OCR2 = 250;
                return;
            }
            ch_b_positive();
            OCR2 = pulse_ch_b.width_ticks;
            pulse_ch_b.phase = PH_POSITIVE;
            break;

        case PH_POSITIVE:
            ch_b_all_off();
            OCR2 = DEAD_TIME_TICKS;
            pulse_ch_b.phase = PH_DEADTIME1;
            break;

        case PH_DEADTIME1:
            ch_b_negative();
            OCR2 = pulse_ch_b.width_ticks;
            pulse_ch_b.phase = PH_NEGATIVE;
            break;

        case PH_NEGATIVE:
            ch_b_all_off();
            OCR2 = DEAD_TIME_TICKS;
            pulse_ch_b.phase = PH_DEADTIME2;
            break;

        case PH_DEADTIME2: {
            /* Calculate gap, split into 250 us chunks if needed */
            uint16_t used = (uint16_t)pulse_ch_b.width_ticks * 2 +
                            DEAD_TIME_TICKS * 2;
            uint16_t gap;
            if (pulse_ch_b.period_ticks > used)
                gap = pulse_ch_b.period_ticks - used;
            else
                gap = DEAD_TIME_TICKS;

            if (gap <= 250) {
                OCR2 = (uint8_t)gap;
                pulse_ch_b.gap_remaining = 0;
            } else {
                OCR2 = 250;
                pulse_ch_b.gap_remaining = gap - 250;
            }
            pulse_ch_b.phase = PH_GAP;
            break;
        }
    }
}
/* ---------- USART UDRE Interrupt (non‑blocking transmit) ---------- */
/*
ISR(USART_UDRE_vect) {
    if (tx_head != tx_tail) {
        UDR = tx_buffer[tx_tail];
        tx_tail = (tx_tail + 1) % 64;
    } else {
        UCSRB &= ~(1 << UDRIE);   // buffer empty, disable interrupt
    }
}*/
 
