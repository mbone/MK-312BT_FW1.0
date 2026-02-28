/*
 * MK312BT_Memory.h - Minimal Global State and Memory-Mapped Register Macros
 *
 * The live channel state lives in channel_a / channel_b (ChannelBlock).
 * This struct holds only the handful of fields that are NOT part of a
 * channel block: pot lockout, power level, MA knob, housekeeping counter,
 * and the output-control flags that the pulse generator reads.
 *
 * Pointer macros provide address-of access compatible with the serial
 * protocol handler (serial_mem.c).
 */

#ifndef MK312BT_MEMORY_H
#define MK312BT_MEMORY_H

#include <stdint.h>

typedef struct {
  uint8_t output_control_flags;  /* Bitmask: audio/phase/output mode flags */
  uint8_t pot_lockout_flags;     /* Front panel pot lockout control */
  uint8_t power_level;           /* Master power level (0=Low,1=Normal,2=High) */
  uint8_t multi_adjust_offset;   /* Multi-adjust knob Offset value (0-75) */
  uint8_t multi_adjust;          /* Multi-adjust knob value after being scaled (MA Min/Max Scaled) */
  uint8_t timer0_counter;        /* Software tick counter for housekeeping */
} MK312BTState;

extern volatile MK312BTState g_mk312bt_state;

#define OUTPUT_CONTROL_FLAGS  (&g_mk312bt_state.output_control_flags)
#define POT_LOCKOUT_FLAGS     (&g_mk312bt_state.pot_lockout_flags)
#define POWER_LEVEL_CONFIG    (&g_mk312bt_state.power_level)
#define MULTI_ADJUST_OFFSET   (&g_mk312bt_state.multi_adjust_offset)
#define MULTI_ADJUST          (&g_mk312bt_state.multi_adjust)
#define TIMER0_COUNTER        (&g_mk312bt_state.timer0_counter)

#endif
