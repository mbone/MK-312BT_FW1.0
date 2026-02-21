/*
 * config.c - Runtime Configuration Manager
 *
 * Manages the system_config_t singleton that holds all runtime parameters:
 * current mode, power level, per-channel intensity/frequency/width,
 * Multi-Adjust knob value, audio gain, and all 8 advanced settings.
 *
 * Configuration can be loaded from EEPROM (persistent) or reset to
 * factory defaults. config_apply_to_memory() pushes current values
 * into the MK312BTState struct where the mode engine reads them.
 */

#include "config.h"
#include "MK312BT_Memory.h"
#include "MK312BT_Modes.h"
#include "channel_mem.h"
#include "eeprom.h"
#include <string.h>

static system_config_t system_config;  /* The one runtime config instance */

/* Initialize config with factory defaults */
void config_init(void) {
    config_set_defaults();
}

/* Factory default values for all parameters */
void config_set_defaults(void) {
    system_config.current_mode = MODE_WAVES;
    system_config.power_level = 1;
    system_config.split_mode = 0;
    system_config.split_a_mode = MODE_WAVES;
    system_config.split_b_mode = MODE_WAVES;
    system_config.intensity_a = 128;
    system_config.intensity_b = 128;
    system_config.frequency_a = 5;       /* ~100 Hz default */
    system_config.frequency_b = 5;
    system_config.width_a = 25;          /* ~180 us default */
    system_config.width_b = 25;
    system_config.multi_adjust = 128;    /* MA knob center position */
    system_config.audio_gain = 128;      /* Mid-range audio sensitivity */
    system_config.adv_ramp_level = 128;  /* Advanced: ramp target level */
    system_config.adv_ramp_time = 0;     /* Advanced: ramp duration */
    system_config.adv_depth = 50;        /* Advanced: intensity depth */
    system_config.adv_tempo = 50;        /* Advanced: cycle speed */
    system_config.adv_frequency = 107;   /* Advanced: frequency override */
    system_config.adv_effect = 128;      /* Advanced: effect intensity */
    system_config.adv_width = 130;       /* Advanced: width override */
    system_config.adv_pace = 50;         /* Advanced: width cycle speed */
    system_config.favorite_mode = MODE_WAVES;
}

/* Load configuration from EEPROM. Falls back to defaults if EEPROM
 * is blank (no magic byte) or checksum doesn't match. */
void config_load_from_eeprom(void) {
    eeprom_config_t eeprom_cfg;

    if (eeprom_load_config(&eeprom_cfg)) {
        system_config.current_mode = eeprom_cfg.top_mode;
        system_config.power_level = eeprom_cfg.power_level;
        system_config.split_mode = eeprom_cfg.split_mode;
        eeprom_load_split_modes(&system_config.split_a_mode, &system_config.split_b_mode);
        system_config.intensity_a = eeprom_cfg.intensity_a;
        system_config.intensity_b = eeprom_cfg.intensity_b;
        system_config.frequency_a = eeprom_cfg.frequency_a;
        system_config.frequency_b = eeprom_cfg.frequency_b;
        system_config.width_a = eeprom_cfg.width_a;
        system_config.width_b = eeprom_cfg.width_b;
        system_config.multi_adjust = eeprom_cfg.multi_adjust;
        system_config.audio_gain = eeprom_cfg.audio_gain;
        system_config.adv_ramp_level = eeprom_cfg.adv_ramp_level;
        system_config.adv_ramp_time = eeprom_cfg.adv_ramp_time;
        system_config.adv_depth = eeprom_cfg.adv_depth;
        system_config.adv_tempo = eeprom_cfg.adv_tempo;
        system_config.adv_frequency = eeprom_cfg.adv_frequency;
        system_config.adv_effect = eeprom_cfg.adv_effect;
        system_config.adv_width = eeprom_cfg.adv_width;
        system_config.adv_pace = eeprom_cfg.adv_pace;
        system_config.favorite_mode = eeprom_cfg.favorite_mode;
    } else {
        config_set_defaults();  /* EEPROM invalid - use factory defaults */
    }
}

/* Return pointer to the runtime config singleton */
system_config_t* config_get(void) {
    return &system_config;
}

void config_sync_from_eeprom_config(const eeprom_config_t* ecfg) {
    system_config.power_level = ecfg->power_level;
    system_config.audio_gain = ecfg->audio_gain;
    system_config.adv_ramp_level = ecfg->adv_ramp_level;
    system_config.adv_ramp_time = ecfg->adv_ramp_time;
    system_config.adv_depth = ecfg->adv_depth;
    system_config.adv_tempo = ecfg->adv_tempo;
    system_config.adv_frequency = ecfg->adv_frequency;
    system_config.adv_effect = ecfg->adv_effect;
    system_config.adv_width = ecfg->adv_width;
    system_config.adv_pace = ecfg->adv_pace;
}

void config_apply_to_memory(void) {
    channel_a.intensity_value = system_config.intensity_a;
    channel_b.intensity_value = system_config.intensity_b;
    channel_a.freq_value = system_config.frequency_a;
    channel_b.freq_value = system_config.frequency_b;
    channel_a.width_value = system_config.width_a;
    channel_b.width_value = system_config.width_b;
    *POWER_LEVEL_CONFIG = system_config.power_level;
}
