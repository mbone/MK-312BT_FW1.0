/*
 * eeprom.h - EEPROM Persistent Storage
 *
 * ATmega16 internal EEPROM driver with structured config storage.
 * Uses magic byte (0xA5) and XOR checksum for data integrity.
 *
 * EEPROM Memory Layout (512 bytes total):
 *   0x000–0x015  (22B)  eeprom_config_t  - main settings (magic + checksum)
 *   0x016        (1B)   split_a_mode     - mode number for split channel A
 *   0x017        (1B)   split_b_mode     - mode number for split channel B
 *   0x018–0x01F  (8B)   reserved
 *   0x020–0x0FF  (224B) user programs    - 7 slots × 32 bytes (USER_PROG_SLOT_SIZE)
 */

#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EEPROM_CONFIG_BASE      0x000
#define EEPROM_MAGIC_BYTE       0xA6
#define EEPROM_SPLIT_A_MODE     0x016
#define EEPROM_SPLIT_B_MODE     0x017
#define EEPROM_USER_PROG_BASE   0x020
#define USER_PROG_SLOT_SIZE     32
#define USER_PROG_SLOT_COUNT    7

/* Slot validity marker stored as first byte of each user program slot */
#define USER_PROG_MAGIC         0xE3

/* Persistent configuration stored in EEPROM. */
typedef struct {
    uint8_t magic;
    uint8_t top_mode;
    uint8_t favorite_mode;
    uint8_t power_level;
    uint8_t intensity_a;
    uint8_t intensity_b;
    uint8_t frequency_a;
    uint8_t frequency_b;
    uint8_t width_a;
    uint8_t width_b;
    uint8_t multi_adjust;
    uint8_t audio_gain;
    uint8_t split_mode;
    uint8_t adv_ramp_level;
    uint8_t adv_ramp_time;
    uint8_t adv_depth;
    uint8_t adv_tempo;
    uint8_t adv_frequency;
    uint8_t adv_effect;
    uint8_t adv_width;
    uint8_t adv_pace;
    uint8_t checksum;
} eeprom_config_t;

void    mk312bt_eeprom_write_byte(uint16_t address, uint8_t data);
uint8_t mk312bt_eeprom_read_byte(uint16_t address);
void    eeprom_save_config(eeprom_config_t *config);
uint8_t eeprom_load_config(eeprom_config_t *config);
void    eeprom_init_defaults(eeprom_config_t *config);

/* Split mode channel selectors */
void    eeprom_save_split_modes(uint8_t mode_a, uint8_t mode_b);
void    eeprom_load_split_modes(uint8_t *mode_a, uint8_t *mode_b);

/* User program slot API.
 * slot: 0–6 (USER_PROG_SLOT_COUNT-1)
 * buf must be USER_PROG_SLOT_SIZE bytes.
 * First byte of buf is USER_PROG_MAGIC when slot is valid.
 * Bytecode starts at buf[1], terminated by 0x00 pair. */
void    eeprom_save_user_prog(uint8_t slot, const uint8_t *buf);
uint8_t eeprom_load_user_prog(uint8_t slot, uint8_t *buf);
void    eeprom_erase_user_prog(uint8_t slot);

#ifdef __cplusplus
}
#endif

#endif
