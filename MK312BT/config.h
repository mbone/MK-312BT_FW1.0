/*
 * config.h - Runtime Configuration Manager
 *
 * Holds the system_config_t struct with all runtime parameters.
 * Loaded from EEPROM at startup, modified by menus and knobs,
 * and pushed to hardware via config_apply_to_memory().
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include "eeprom.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Runtime configuration - all values 0-255 unless noted */
typedef struct {
    uint8_t current_mode;       /* Active mode (MK312BTMode enum value) */
    uint8_t power_level;        /* 0=Low, 1=Normal, 2=High */
    uint8_t split_mode;         /* 0=linked channels, 1=independent */
    uint8_t split_a_mode;       /* Built-in mode for split channel A (0-16) */
    uint8_t split_b_mode;       /* Built-in mode for split channel B (0-16) */
    uint8_t intensity_a;        /* Channel A base intensity */
    uint8_t intensity_b;        /* Channel B base intensity */
    uint8_t frequency_a;        /* Channel A base frequency index */
    uint8_t frequency_b;        /* Channel B base frequency index */
    uint8_t width_a;            /* Channel A base pulse width index */
    uint8_t width_b;            /* Channel B base pulse width index */
    uint8_t multi_adjust;       /* MA knob position (0-255) */
    uint8_t audio_gain;         /* Audio input gain/sensitivity */
    uint8_t adv_ramp_level;     /* Advanced: ramp target level */
    uint8_t adv_ramp_time;      /* Advanced: ramp duration */
    uint8_t adv_depth;          /* Advanced: intensity depth */
    uint8_t adv_tempo;          /* Advanced: cycle speed */
    uint8_t adv_frequency;      /* Advanced: frequency override */
    uint8_t adv_effect;         /* Advanced: effect intensity */
    uint8_t adv_width;          /* Advanced: width override */
    uint8_t adv_pace;           /* Advanced: width cycle speed */
    uint8_t favorite_mode;      /* Favourite mode (protocol mode number) */
} system_config_t;

void config_init(void);                /* Initialize with factory defaults */
void config_load_from_eeprom(void);    /* Load from EEPROM (fallback to defaults) */
void config_set_defaults(void);        /* Reset to factory defaults */
system_config_t* config_get(void);     /* Get pointer to runtime config */
void config_sync_from_eeprom_config(const eeprom_config_t* ecfg);
void config_apply_to_memory(void);

#ifdef __cplusplus
}
#endif

#endif
