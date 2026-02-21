/*
 * eeprom.c - EEPROM Persistent Storage Driver
 *
 * Low-level EEPROM read/write for the ATmega16's internal 512-byte EEPROM,
 * plus a structured config save/load with magic byte validation and
 * XOR checksum integrity checking.
 *
 * EEPROM layout: eeprom_config_t struct stored at EEPROM_CONFIG_BASE (0x00).
 * First byte is magic (0xA5), last byte is XOR checksum of all preceding bytes.
 *
 * EEPROM register bits used:
 *   EECR bit 0 (EERE): Read enable - set to trigger read
 *   EECR bit 1 (EEWE): Write enable - set to trigger write, poll for completion
 *   EECR bit 2 (EEMWE): Master write enable - must set before EEWE
 */

#include "eeprom.h"
#include "MK312BT_Modes.h"
#include "avr_registers.h"
#include <avr/wdt.h>
#include <string.h>

/* Write one byte to EEPROM at the given address.
 * Waits for any previous write to complete, then triggers a new write.
 * EEMWE (bit 2) must be set before EEWE (bit 1) per ATmega16 datasheet. */
void mk312bt_eeprom_write_byte(uint16_t address, uint8_t data) {
    while (EECR & (1 << EEWE)) { wdt_reset(); }

    EEARL = (uint8_t)(address & 0xFF);
    EEARH = (uint8_t)(address >> 8);
    EEDR = data;

    uint8_t sreg = SREG;
    cli();
    EECR = (1 << EEMWE);
    EECR = (1 << EEMWE) | (1 << EEWE);
    SREG = sreg;

    while (EECR & (1 << EEWE)) { wdt_reset(); }
}

/* Read one byte from EEPROM at the given address.
 * Waits for any previous write to complete before reading. */
uint8_t mk312bt_eeprom_read_byte(uint16_t address) {
    while (EECR & (1 << EEWE));  /* Wait for any pending write */

    EEARL = (uint8_t)(address & 0xFF);
    EEARH = (uint8_t)(address >> 8);
    EECR |= (1 << EERE);  /* EERE - trigger read */
    while (EECR & (1 << EERE));  /* Wait for read to complete (EERE auto-clears after 4 cycles) */

    return EEDR;
}

/* XOR checksum of all bytes except the last (checksum field itself).
 * Simple integrity check to detect corrupted EEPROM data. */
static uint8_t eeprom_calculate_checksum(eeprom_config_t *config) {
    uint8_t checksum = 0;
    uint8_t *ptr = (uint8_t*)config;
    uint16_t len = sizeof(eeprom_config_t);

    if (len < 2) return 0;
    for (uint16_t i = 0; i < len - 1; i++) {
        checksum ^= ptr[i];
    }

    return checksum;
}

/* Save config to EEPROM: set magic byte, compute checksum, write all bytes */
void eeprom_save_config(eeprom_config_t *config) {
    config->magic = EEPROM_MAGIC_BYTE;
    config->checksum = eeprom_calculate_checksum(config);

    uint8_t *ptr = (uint8_t*)config;
    for (uint16_t i = 0; i < sizeof(eeprom_config_t); i++) {
        wdt_reset();
        mk312bt_eeprom_write_byte(EEPROM_CONFIG_BASE + i, ptr[i]);
    }
}

/* Load config from EEPROM. Returns 1 on success, 0 if:
 *   - Magic byte doesn't match (EEPROM is blank/uninitialized)
 *   - Checksum doesn't match (EEPROM data is corrupted) */
uint8_t eeprom_load_config(eeprom_config_t *config) {
    uint8_t *ptr = (uint8_t*)config;

    for (uint16_t i = 0; i < sizeof(eeprom_config_t); i++) {
        ptr[i] = mk312bt_eeprom_read_byte(EEPROM_CONFIG_BASE + i);
    }

    if (config->magic != EEPROM_MAGIC_BYTE) {
        return 0;  /* Not initialized */
    }

    uint8_t calculated_checksum = eeprom_calculate_checksum(config);
    if (calculated_checksum != config->checksum) {
        return 0;  /* Corrupted */
    }

    return 1;
}

/* Initialize config struct with factory default values. */
void eeprom_init_defaults(eeprom_config_t *config) {
    memset(config, 0, sizeof(eeprom_config_t));

    config->magic = EEPROM_MAGIC_BYTE;
    config->top_mode = 0;
    config->favorite_mode = 0;
    config->power_level = 1;
    config->intensity_a = 128;
    config->intensity_b = 128;
    config->frequency_a = 5;
    config->frequency_b = 5;
    config->width_a = 25;
    config->width_b = 25;
    config->multi_adjust = 128;
    config->audio_gain = 128;
    config->split_mode = 0;
    config->adv_ramp_level = 128;
    config->adv_ramp_time = 0;
    config->adv_depth = 50;
    config->adv_tempo = 50;
    config->adv_frequency = 107;
    config->adv_effect = 128;
    config->adv_width = 130;
    config->adv_pace = 50;
}

void eeprom_save_split_modes(uint8_t mode_a, uint8_t mode_b) {
    mk312bt_eeprom_write_byte(EEPROM_SPLIT_A_MODE, mode_a);
    mk312bt_eeprom_write_byte(EEPROM_SPLIT_B_MODE, mode_b);
}

void eeprom_load_split_modes(uint8_t *mode_a, uint8_t *mode_b) {
    uint8_t a = mk312bt_eeprom_read_byte(EEPROM_SPLIT_A_MODE);
    uint8_t b = mk312bt_eeprom_read_byte(EEPROM_SPLIT_B_MODE);
    *mode_a = (a < MODE_SPLIT) ? a : 0;
    *mode_b = (b < MODE_SPLIT) ? b : 0;
}

static uint16_t user_prog_addr(uint8_t slot) {
    return EEPROM_USER_PROG_BASE + (uint16_t)slot * USER_PROG_SLOT_SIZE;
}

void eeprom_save_user_prog(uint8_t slot, const uint8_t *buf) {
    if (slot >= USER_PROG_SLOT_COUNT) return;
    uint16_t base = user_prog_addr(slot);
    for (uint8_t i = 0; i < USER_PROG_SLOT_SIZE; i++) {
        wdt_reset();
        mk312bt_eeprom_write_byte(base + i, buf[i]);
    }
}

uint8_t eeprom_load_user_prog(uint8_t slot, uint8_t *buf) {
    if (slot >= USER_PROG_SLOT_COUNT) return 0;
    uint16_t base = user_prog_addr(slot);
    for (uint8_t i = 0; i < USER_PROG_SLOT_SIZE; i++) {
        buf[i] = mk312bt_eeprom_read_byte(base + i);
    }
    return (buf[0] == USER_PROG_MAGIC) ? 1 : 0;
}

void eeprom_erase_user_prog(uint8_t slot) {
    if (slot >= USER_PROG_SLOT_COUNT) return;
    mk312bt_eeprom_write_byte(user_prog_addr(slot), 0xFF);
}
