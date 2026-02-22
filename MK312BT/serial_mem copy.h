/*
 * serial_mem.h - Virtual Memory Address Translation for Serial Protocol
 *
 * Maps MK-312BT virtual addresses used by the serial protocol to actual
 * firmware state variables, EEPROM storage, and Flash ROM constants.
 *
 * Address regions:
 *   0x0000-0x00FF: Flash ROM (read-only, device ID and firmware version)
 *   0x4000-0x43FF: RAM/Registers (read/write, maps to config + MK312BTState)
 *   0x8000-0x81FF: EEPROM (read/write, persistent settings)
 */

#ifndef SERIAL_MEM_H
#define SERIAL_MEM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Virtual memory region boundaries */
#define VIRT_FLASH_BASE    0x0000
#define VIRT_FLASH_END     0x0100
#define VIRT_RAM_BASE      0x4000
#define VIRT_RAM_END       0x4400
#define VIRT_EEPROM_BASE   0x8000
#define VIRT_EEPROM_END    0x8200

/* Flash region addresses (read-only device identification) */
#define VIRT_FLASH_BOX_MODEL   0x00FC
#define VIRT_FLASH_FW_MAJ      0x00FD
#define VIRT_FLASH_FW_MIN      0x00FE
#define VIRT_FLASH_FW_INT      0x00FF

/* RAM region: channel register blocks */
#define VIRT_RAM_CHAN_A_BASE   0x4080
#define VIRT_RAM_CHAN_A_END    0x40C0
#define VIRT_RAM_CHAN_B_BASE   0x4180
#define VIRT_RAM_CHAN_B_END    0x41C0

/* RAM region: individual virtual registers */
#define VIRT_RAM_POT_LOCKOUT   0x400F
#define VIRT_RAM_LEVEL_A       0x4064
#define VIRT_RAM_LEVEL_B       0x4065
#define VIRT_RAM_MENU_STATE    0x406D
#define VIRT_RAM_BOX_COMMAND   0x4070
#define VIRT_RAM_CURRENT_MODE  0x407B
#define VIRT_RAM_TOP_MODE      0x41F3
#define VIRT_RAM_POWER_LEVEL   0x41F4
#define VIRT_RAM_SPLIT_MODE_A  0x41F5
#define VIRT_RAM_SPLIT_MODE_B  0x41F6
#define VIRT_RAM_FAVOURITE     0x41F7
#define VIRT_RAM_ADV_RAMP_LVL  0x41F8
#define VIRT_RAM_ADV_RAMP_TIME 0x41F9
#define VIRT_RAM_ADV_DEPTH     0x41FA
#define VIRT_RAM_ADV_TEMPO     0x41FB
#define VIRT_RAM_ADV_FREQUENCY 0x41FC
#define VIRT_RAM_ADV_EFFECT    0x41FD
#define VIRT_RAM_ADV_WIDTH     0x41FE
#define VIRT_RAM_ADV_PACE      0x41FF
#define VIRT_RAM_BATTERY_LEVEL 0x4203
#define VIRT_RAM_MULTI_ADJUST  0x420D
#define VIRT_RAM_BOX_KEY       0x4213
#define VIRT_RAM_POWER_SUPPLY  0x4215

/* EEPROM region offsets (from VIRT_EEPROM_BASE = 0x8000) */
#define VIRT_EE_PROVISIONED     0x0001
#define VIRT_EE_BOX_SERIAL_LO   0x0002
#define VIRT_EE_BOX_SERIAL_HI   0x0003
#define VIRT_EE_ELINK_SIG1      0x0006
#define VIRT_EE_ELINK_SIG2      0x0007
#define VIRT_EE_TOP_MODE        0x0008
#define VIRT_EE_POWER_LEVEL     0x0009
#define VIRT_EE_SPLIT_MODE_A    0x000A
#define VIRT_EE_SPLIT_MODE_B    0x000B
#define VIRT_EE_FAVOURITE_MODE  0x000C
#define VIRT_EE_ADV_RAMP_LEVEL  0x000D
#define VIRT_EE_ADV_RAMP_TIME   0x000E
#define VIRT_EE_ADV_DEPTH       0x000F
#define VIRT_EE_ADV_TEMPO       0x0010
#define VIRT_EE_ADV_FREQUENCY   0x0011
#define VIRT_EE_ADV_EFFECT      0x0012
#define VIRT_EE_ADV_WIDTH       0x0013
#define VIRT_EE_ADV_PACE        0x0014

/* Box command codes (written to VIRT_RAM_BOX_COMMAND) */
#define BOX_CMD_RELOAD_MODE    0x00
#define BOX_CMD_EXIT_MENU      0x04
#define BOX_CMD_MAIN_MENU      0x0A
#define BOX_CMD_NEXT_MODE      0x10
#define BOX_CMD_PREV_MODE      0x11
#define BOX_CMD_SET_MODE       0x12
#define BOX_CMD_LCD_WRITE_CHAR 0x13
#define BOX_CMD_LCD_WRITE_NUM  0x14
#define BOX_CMD_LCD_WRITE_STR  0x15
#define BOX_CMD_MUTE           0x18
#define BOX_CMD_SWAP_CHANNELS  0x19
#define BOX_CMD_COPY_A_TO_B    0x1A
#define BOX_CMD_COPY_B_TO_A    0x1B
#define BOX_CMD_START_RAMP     0x21
#define BOX_CMD_LCD_SET_POS    0x23

uint8_t serial_mem_read(uint16_t address);
void serial_mem_write(uint16_t address, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif
