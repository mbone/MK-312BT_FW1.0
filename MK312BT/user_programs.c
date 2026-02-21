/*
 * user_programs.c - User-programmable bytecode module storage
 *
 * Maintains a RAM cache of 7 user program slots.  Each slot is
 * USER_PROG_SLOT_SIZE (32) bytes loaded from EEPROM at startup.
 *
 * Execution uses the same SET opcode interpreter as execute_module()
 * in mode_dispatcher.c.  Only SET instructions (0x80+) are supported
 * in user programs; COPY/MATHOP/MEMOP are not allowed so the bytecode
 * stays simple and verifiable.
 */

#include "user_programs.h"
#include "eeprom.h"
#include "channel_mem.h"
#include <string.h>

static uint8_t prog_cache[USER_PROG_SLOT_COUNT][USER_PROG_SLOT_SIZE];

void user_programs_init(void) {
    for (uint8_t s = 0; s < USER_PROG_SLOT_COUNT; s++) {
        eeprom_load_user_prog(s, prog_cache[s]);
    }
}

uint8_t user_prog_is_valid(uint8_t slot) {
    if (slot >= USER_PROG_SLOT_COUNT) return 0;
    return (prog_cache[slot][0] == USER_PROG_MAGIC) ? 1 : 0;
}

void user_prog_execute(uint8_t slot) {
    if (!user_prog_is_valid(slot)) return;

    const uint8_t *pc = &prog_cache[slot][1];
    const uint8_t *end = &prog_cache[slot][USER_PROG_SLOT_SIZE];

    while (pc < end) {
        uint8_t opcode = *pc;

        if (opcode == 0x00) break;

        if (opcode & 0x80) {
            if (pc + 1 >= end) break;
            uint8_t offset = opcode & 0x3F;
            uint8_t value  = *(pc + 1);

            if (opcode & 0x40) {
                uint8_t *reg = channel_get_reg_ptr(0x180 + offset);
                *reg = value;
            } else {
                uint8_t ac = channel_a.apply_channel;
                if (ac & 0x01) {
                    uint8_t *reg = channel_get_reg_ptr(0x080 + offset);
                    *reg = value;
                }
                if (ac & 0x02) {
                    uint8_t *reg = channel_get_reg_ptr(0x180 + offset);
                    *reg = value;
                }
            }
            pc += 2;
        } else {
            break;
        }
    }
}

void user_prog_write(uint8_t slot, const uint8_t *buf) {
    if (slot >= USER_PROG_SLOT_COUNT) return;
    memcpy(prog_cache[slot], buf, USER_PROG_SLOT_SIZE);
    prog_cache[slot][USER_PROG_SLOT_SIZE - 1] = 0x00;
    eeprom_save_user_prog(slot, prog_cache[slot]);
}

void user_prog_erase(uint8_t slot) {
    if (slot >= USER_PROG_SLOT_COUNT) return;
    memset(prog_cache[slot], 0xFF, USER_PROG_SLOT_SIZE);
    prog_cache[slot][0] = 0xFF;
    eeprom_erase_user_prog(slot);
}

uint8_t user_prog_read(uint8_t slot, uint8_t *buf) {
    if (slot >= USER_PROG_SLOT_COUNT) return 0;
    memcpy(buf, prog_cache[slot], USER_PROG_SLOT_SIZE);
    return (buf[0] == USER_PROG_MAGIC) ? 1 : 0;
}
