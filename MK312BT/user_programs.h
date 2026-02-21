/*
 * user_programs.h - User-programmable bytecode module storage
 *
 * Provides a RAM cache of the 7 user program slots read from EEPROM.
 * The dispatcher calls user_prog_execute() to run a slot's bytecode
 * exactly like the built-in execute_module() but from EEPROM/RAM.
 *
 * Each slot is USER_PROG_SLOT_SIZE bytes:
 *   [0]      USER_PROG_MAGIC (0xE3) — validity marker
 *   [1..30]  bytecode instructions (same format as PROGMEM modules)
 *   [31]     must be 0x00 (end-of-program guard)
 *
 * The bytecode format is identical to the built-in modules:
 *   SET  (0x80 | offset), value
 *   END  0x00, 0x00
 *   (no COPY/MATHOP/MEMOP in user programs to keep the parser simple)
 */

#ifndef USER_PROGRAMS_H
#define USER_PROGRAMS_H

#include <stdint.h>
#include "eeprom.h"
#include "channel_mem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Load all user program slots from EEPROM into RAM cache.
 * Call once at startup (after eeprom drivers are ready). */
void user_programs_init(void);

/* Execute user program slot (0-6) against channel registers.
 * Uses channel_a.apply_channel routing (same as built-in modules).
 * Does nothing if the slot has no valid program. */
void user_prog_execute(uint8_t slot);

/* Returns 1 if the slot contains a valid program. */
uint8_t user_prog_is_valid(uint8_t slot);

/* Write a new bytecode program into a slot and save to EEPROM.
 * buf must be USER_PROG_SLOT_SIZE bytes; caller sets buf[0]=USER_PROG_MAGIC. */
void user_prog_write(uint8_t slot, const uint8_t *buf);

/* Erase a slot in RAM cache and EEPROM. */
void user_prog_erase(uint8_t slot);

/* Read current slot contents into buf (USER_PROG_SLOT_SIZE bytes).
 * Returns 1 if valid. */
uint8_t user_prog_read(uint8_t slot, uint8_t *buf);

#ifdef __cplusplus
}
#endif

#endif
