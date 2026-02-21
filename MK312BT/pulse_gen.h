/*
 * MK-312BT Biphasic Pulse Generator
 *
 * Generates biphasic (alternating polarity) pulses on two independent channels
 * using Timer1 (Channel A) and Timer2 (Channel B) in CTC mode at 1 MHz.
 *
 * Each channel drives an H-bridge through 5 phases per pulse cycle:
 *   PH_POSITIVE  -> Gate+ HIGH, Gate- LOW  (positive half-cycle)
 *   PH_DEADTIME1 -> Both LOW for 4 us     (prevent FET shoot-through)
 *   PH_NEGATIVE  -> Gate+ LOW, Gate- HIGH  (negative half-cycle)
 *   PH_DEADTIME2 -> Both LOW for 4 us     (prevent FET shoot-through)
 *   PH_GAP       -> Both LOW              (inter-pulse gap)
 *
 * H-bridge pin assignments:
 *   Channel A: PB2 = Gate+, PB3 = Gate-
 *   Channel B: PB0 = Gate+, PB1 = Gate-
 *
 * The main loop sets width, period, and gate on/off.
 * Timer ISRs (in interrupts.c) run the state machine autonomously.
 */
#ifndef PULSE_GEN_H
#define PULSE_GEN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PULSE_OFF   0
#define PULSE_ON    1

/* 5-phase biphasic pulse state machine */
typedef enum {
    PH_POSITIVE,    // Gate+ on, Gate- off
    PH_DEADTIME1,   // Both off (4 us dead time)
    PH_NEGATIVE,    // Gate+ off, Gate- on
    PH_DEADTIME2,   // Both off (4 us dead time)
    PH_GAP          // Both off (remaining period time)
} PulsePhase;

/* Per-channel pulse generator state.
 * All fields are volatile since they're shared with ISRs.
 * The pending_* fields provide double-buffered parameter updates:
 * main loop writes pending values, ISR copies them at start of each cycle. */
typedef struct {
    volatile uint8_t gate;           // PULSE_ON or PULSE_OFF
    volatile uint8_t width_ticks;    // Pulse half-cycle width in us (min 20)
    volatile uint16_t period_ticks;  // Full pulse period in us (min 500)
    volatile PulsePhase phase;       // Current state machine phase
    volatile uint16_t gap_remaining; // Timer2 only: multi-step gap countdown
    volatile uint8_t pending_width;  // Double-buffered width (main loop writes)
    volatile uint16_t pending_period; // Double-buffered period (main loop writes)
    volatile uint8_t params_dirty;   // Set by main loop, cleared by ISR after copy
} ChannelPulseState;

extern volatile ChannelPulseState pulse_ch_a;  // Timer1 CompA ISR state
extern volatile ChannelPulseState pulse_ch_b;  // Timer2 Comp ISR state

/* Initialize Timer1 and Timer2 in CTC mode with /8 prescaler.
 * Starts both timers with gates OFF. Enables CompA and Comp2 interrupts. */
void pulse_gen_init(void);

/* Set pulse half-cycle width in microseconds (min 20 us, max 255 us) */
void pulse_set_width_a(uint8_t width_us);
void pulse_set_width_b(uint8_t width_us);

/* Turn pulse output on/off. When off, H-bridge pins go LOW immediately. */
void pulse_set_gate_a(uint8_t on);
void pulse_set_gate_b(uint8_t on);

/* Set pulse period in microseconds (min 500 us = 2 kHz max frequency) */
void pulse_set_frequency_a(uint16_t period_us);
void pulse_set_frequency_b(uint16_t period_us);

#ifdef __cplusplus
}
#endif

#endif
