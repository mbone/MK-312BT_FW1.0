/*
 * MK312BT_Modes.h - Mode Enumeration
 *
 * Defines the MK312BTMode enum with all 25 operating modes:
 *   Modes 0-16  (Waves through Phase3): Built-in modes using the parameter engine
 *   Modes 17-23 (User1 through User7):  User-programmable modes using bytecode interpreter
 *   Mode 24     (Split):                 Independent channel control via bytecode
 *   MODE_COUNT  (25):                    Total number of modes
 *
 * Built-in modes (0-16) are configured via the PROGMEM table in mode_behavior.c.
 * User modes (17-24) execute bytecode programs from mode_programs.c.
 */

#ifndef MK312BT_MODES_H
#define MK312BT_MODES_H

#include <stdint.h>

typedef enum {
  /* Built-in modes - parameter engine driven */
  MODE_WAVES = 0,   /* Smooth sine wave modulation */
  MODE_STROKE,      /* Sharp stroking pulses */
  MODE_CLIMB,       /* Gradual intensity climb (sawtooth freq) */
  MODE_COMBO,       /* Complex mixed pattern with on/off gating */
  MODE_INTENSE,     /* High intensity rapid bursts */
  MODE_RHYTHM,      /* Rhythmic pulsing with width toggle */
  MODE_AUDIO1,      /* Audio-reactive: levels and width from audio */
  MODE_AUDIO2,      /* Audio-reactive variant */
  MODE_AUDIO3,      /* Audio-reactive high frequency */
  MODE_RANDOM1,     /* Moderate random variation */
  MODE_RANDOM2,     /* Highly chaotic: all params sweep randomly */
  MODE_TOGGLE,      /* On/off toggle with channels alternating */
  MODE_ORGASM,      /* Progressive building to peak */
  MODE_TORMENT,     /* Builds then denies: random brief bursts */
  MODE_PHASE1,      /* 90-degree phase shift between channels */
  MODE_PHASE2,      /* 180-degree phase shift with level ramping */
  MODE_PHASE3,      /* Complex phase with level and width ramping */

  /* User-programmable modes - bytecode interpreter driven */
  MODE_USER1,       /* User program slot 1 */
  MODE_USER2,       /* User program slot 2 */
  MODE_USER3,       /* User program slot 3 */
  MODE_USER4,       /* User program slot 4 */
  MODE_USER5,       /* User program slot 5 */
  MODE_USER6,       /* User program slot 6 */
  MODE_USER7,       /* User program slot 7 */
  MODE_SPLIT,       /* Independent channel control */

  MODE_COUNT        /* Total number of modes (25) */
} MK312BTMode;

#endif
