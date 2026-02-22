/*
 * serial_mem.c - Virtual Memory Address Translation for Serial Protocol
 *
 * Translates MK-312BT virtual addresses into reads/writes against:
 *   - Flash ROM region (0x0000-0x00FF): Read-only device identification
 *   - RAM region (0x4000-0x43FF): Live state from channel blocks, config
 *   - EEPROM region (0x8000-0x81FF): Persistent storage via EEPROM driver
 *
 * RAM addresses in the 0x4080-0x40BF range map to channel_a registers.
 * RAM addresses in the 0x4180-0x41BF range map to channel_b registers.
 */

#include "serial_mem.h"
#include "serial.h"
#include "MK312BT_Memory.h"
#include "MK312BT_Constants.h"
#include "MK312BT_Modes.h"
#include "channel_mem.h"
#include "config.h"
#include "eeprom.h"
#include "mode_dispatcher.h"
#include "lcd.h"
#include "adc.h"
#include <string.h>

static uint8_t mode_to_protocol(uint8_t mode) {
    return mode + SERIAL_MODE_PROTOCOL_BASE;
}

static uint8_t protocol_to_mode(uint8_t proto) {
    if (proto < SERIAL_MODE_PROTOCOL_BASE) return 0;
    uint8_t m = proto - SERIAL_MODE_PROTOCOL_BASE;
    return (m < MODE_COUNT) ? m : 0;
}

static uint8_t read_flash(uint16_t address) {
    switch (address) {
        case VIRT_FLASH_BOX_MODEL: return BOX_MODEL_MK312BT;
        case VIRT_FLASH_FW_MAJ:   return FIRMWARE_VER_MAJ;
        case VIRT_FLASH_FW_MIN:   return FIRMWARE_VER_MIN;
        case VIRT_FLASH_FW_INT:   return FIRMWARE_VER_INT;
        default:                  return 0x00;
    }
}

static uint8_t read_eeprom_region(uint16_t address) {
    uint16_t offset = address - VIRT_EEPROM_BASE;
    system_config_t* cfg = config_get();

    switch (offset) {
        case VIRT_EE_PROVISIONED:    return 0x55;
        case VIRT_EE_BOX_SERIAL_LO:  return 0x01;
        case VIRT_EE_BOX_SERIAL_HI:  return 0x00;
        case VIRT_EE_ELINK_SIG1:     return 0x01;
        case VIRT_EE_ELINK_SIG2:     return 0x01;
        case VIRT_EE_TOP_MODE:       return mode_to_protocol(cfg->current_mode);
        case VIRT_EE_POWER_LEVEL:    return cfg->power_level;
        case VIRT_EE_SPLIT_MODE_A:   return mode_to_protocol(cfg->split_a_mode);
        case VIRT_EE_SPLIT_MODE_B:   return mode_to_protocol(cfg->split_b_mode);
        case VIRT_EE_FAVOURITE_MODE: return mode_to_protocol(cfg->favorite_mode);
        case VIRT_EE_ADV_RAMP_LEVEL: return cfg->adv_ramp_level;
        case VIRT_EE_ADV_RAMP_TIME:  return cfg->adv_ramp_time;
        case VIRT_EE_ADV_DEPTH:      return cfg->adv_depth;
        case VIRT_EE_ADV_TEMPO:      return cfg->adv_tempo;
        case VIRT_EE_ADV_FREQUENCY:  return cfg->adv_frequency;
        case VIRT_EE_ADV_EFFECT:     return cfg->adv_effect;
        case VIRT_EE_ADV_WIDTH:      return cfg->adv_width;
        case VIRT_EE_ADV_PACE:       return cfg->adv_pace;
        default:
            return mk312bt_eeprom_read_byte(offset);
    }
}

static void write_eeprom_region(uint16_t address, uint8_t value) {
    uint16_t offset = address - VIRT_EEPROM_BASE;
    system_config_t* cfg = config_get();

    switch (offset) {
        case VIRT_EE_TOP_MODE:
            cfg->current_mode = protocol_to_mode(value);
            break;
        case VIRT_EE_POWER_LEVEL:
            if (value <= 2) cfg->power_level = value;
            break;
        case VIRT_EE_SPLIT_MODE_A:
            cfg->split_a_mode = protocol_to_mode(value);
            break;
        case VIRT_EE_SPLIT_MODE_B:
            cfg->split_b_mode = protocol_to_mode(value);
            break;
        case VIRT_EE_FAVOURITE_MODE:
            cfg->favorite_mode = protocol_to_mode(value);
            break;
        case VIRT_EE_ADV_RAMP_LEVEL: cfg->adv_ramp_level = value; break;
        case VIRT_EE_ADV_RAMP_TIME:  cfg->adv_ramp_time = value; break;
        case VIRT_EE_ADV_DEPTH:      cfg->adv_depth = value; break;
        case VIRT_EE_ADV_TEMPO:      cfg->adv_tempo = value; break;
        case VIRT_EE_ADV_FREQUENCY:  cfg->adv_frequency = value; break;
        case VIRT_EE_ADV_EFFECT:     cfg->adv_effect = value; break;
        case VIRT_EE_ADV_WIDTH:      cfg->adv_width = value; break;
        case VIRT_EE_ADV_PACE:       cfg->adv_pace = value; break;
        case VIRT_EE_PROVISIONED:
        case VIRT_EE_ELINK_SIG1:
        case VIRT_EE_ELINK_SIG2:
            break;
        default:
            if (offset >= sizeof(eeprom_config_t))
                mk312bt_eeprom_write_byte(offset, value);
            break;
    }
}

static void execute_box_command(uint8_t cmd);
static uint8_t last_box_command = 0xFF;

static uint8_t read_ram(uint16_t address) {
    system_config_t* cfg = config_get();

    if (address >= VIRT_RAM_CHAN_A_BASE && address < VIRT_RAM_CHAN_A_END) {
        uint8_t offset = address - VIRT_RAM_CHAN_A_BASE;
        return ((uint8_t*)&channel_a)[offset];
    }
    if (address >= VIRT_RAM_CHAN_B_BASE && address < VIRT_RAM_CHAN_B_END) {
        uint8_t offset = address - VIRT_RAM_CHAN_B_BASE;
        return ((uint8_t*)&channel_b)[offset];
    }

    switch (address) {
        case VIRT_RAM_LEVEL_A:
            return (uint8_t)(adc_read_level_a() >> 2);
        case VIRT_RAM_LEVEL_B:
            return (uint8_t)(adc_read_level_b() >> 2);

        case VIRT_RAM_BOX_COMMAND: return last_box_command;
        case VIRT_RAM_POT_LOCKOUT:  return *POT_LOCKOUT_FLAGS;
        case VIRT_RAM_MENU_STATE:   return 0x02;
        case VIRT_RAM_CURRENT_MODE: return mode_to_protocol(cfg->current_mode);

        case VIRT_RAM_TOP_MODE:     return mode_to_protocol(cfg->current_mode);
        case VIRT_RAM_POWER_LEVEL:  return cfg->power_level;
        case VIRT_RAM_SPLIT_MODE_A: return mode_to_protocol(cfg->split_a_mode);
        case VIRT_RAM_SPLIT_MODE_B: return mode_to_protocol(cfg->split_b_mode);
        case VIRT_RAM_FAVOURITE:    return mode_to_protocol(cfg->favorite_mode);
        case VIRT_RAM_ADV_RAMP_LVL: return cfg->adv_ramp_level;
        case VIRT_RAM_ADV_RAMP_TIME:return cfg->adv_ramp_time;
        case VIRT_RAM_ADV_DEPTH:    return cfg->adv_depth;
        case VIRT_RAM_ADV_TEMPO:    return cfg->adv_tempo;
        case VIRT_RAM_ADV_FREQUENCY:return cfg->adv_frequency;
        case VIRT_RAM_ADV_EFFECT:   return cfg->adv_effect;
        case VIRT_RAM_ADV_WIDTH:    return cfg->adv_width;
        case VIRT_RAM_ADV_PACE:     return cfg->adv_pace;
        case VIRT_RAM_BATTERY_LEVEL: return (uint8_t)(adc_read_battery() >> 2);
        case VIRT_RAM_MULTI_ADJUST: return *MULTI_ADJUST_VALUE;
        case VIRT_RAM_BOX_KEY:      return 0x00;
        case VIRT_RAM_POWER_SUPPLY: return 0x02;

        default: return 0x00;
    }
}

static void write_ram(uint16_t address, uint8_t value) {
    system_config_t* cfg = config_get();

    if (address >= VIRT_RAM_CHAN_A_BASE && address < VIRT_RAM_CHAN_A_END) {
        uint8_t offset = address - VIRT_RAM_CHAN_A_BASE;
        ((uint8_t*)&channel_a)[offset] = value;
        return;
    }
    if (address >= VIRT_RAM_CHAN_B_BASE && address < VIRT_RAM_CHAN_B_END) {
        uint8_t offset = address - VIRT_RAM_CHAN_B_BASE;
        ((uint8_t*)&channel_b)[offset] = value;
        return;
    }

    switch (address) {
        case VIRT_RAM_BOX_COMMAND:
            last_box_command = value;
            execute_box_command(value);
            last_box_command = 0xFF;
            break;

        case VIRT_RAM_CURRENT_MODE:
            cfg->current_mode = protocol_to_mode(value);
            mode_dispatcher_request_mode(cfg->current_mode);
            break;

        case VIRT_RAM_POT_LOCKOUT:
            *POT_LOCKOUT_FLAGS = value;
            break;

        case VIRT_RAM_POWER_LEVEL:
            if (value <= 2) cfg->power_level = value;
            *POWER_LEVEL_CONFIG = value;
            break;

        case VIRT_RAM_SPLIT_MODE_A:
            cfg->split_a_mode = protocol_to_mode(value);
            break;

        case VIRT_RAM_SPLIT_MODE_B:
            cfg->split_b_mode = protocol_to_mode(value);
            break;

        case VIRT_RAM_FAVOURITE:
            cfg->favorite_mode = protocol_to_mode(value);
            break;

        case VIRT_RAM_ADV_RAMP_LVL: cfg->adv_ramp_level = value; break;
        case VIRT_RAM_ADV_RAMP_TIME:cfg->adv_ramp_time = value; break;
        case VIRT_RAM_ADV_DEPTH:    cfg->adv_depth = value; break;
        case VIRT_RAM_ADV_TEMPO:    cfg->adv_tempo = value; break;
        case VIRT_RAM_ADV_FREQUENCY:cfg->adv_frequency = value; break;
        case VIRT_RAM_ADV_EFFECT:   cfg->adv_effect = value; break;
        case VIRT_RAM_ADV_WIDTH:    cfg->adv_width = value; break;
        case VIRT_RAM_ADV_PACE:     cfg->adv_pace = value; break;

        case VIRT_RAM_MULTI_ADJUST:
            *MULTI_ADJUST_VALUE = value;
            cfg->multi_adjust = value;
            break;

        case VIRT_RAM_BOX_KEY:
            //if (value == 0) serial_reset_encryption();
            break;

        default:
            break;
    }
}

static void execute_box_command(uint8_t cmd) {
    switch (cmd) {
        case BOX_CMD_RELOAD_MODE:
            mode_dispatcher_request_reload();
            break;

        case BOX_CMD_EXIT_MENU:
        case BOX_CMD_MAIN_MENU:
            break;

        case BOX_CMD_MUTE:
            mode_dispatcher_request_pause();
            break;

        case BOX_CMD_NEXT_MODE:
            mode_dispatcher_request_next_mode();
            break;

        case BOX_CMD_PREV_MODE:
            mode_dispatcher_request_prev_mode();
            break;

        case BOX_CMD_SET_MODE:
            mode_dispatcher_request_reload();
            break;

        case BOX_CMD_LCD_WRITE_CHAR:
        case BOX_CMD_LCD_WRITE_NUM:
        case BOX_CMD_LCD_WRITE_STR:
        case BOX_CMD_LCD_SET_POS:
            break;

        case BOX_CMD_START_RAMP:
            mode_dispatcher_request_start_ramp();
            break;

        case BOX_CMD_SWAP_CHANNELS: {
            ChannelBlock tmp;
            memcpy(&tmp, &channel_a, sizeof(ChannelBlock));
            memcpy(&channel_a, &channel_b, sizeof(ChannelBlock));
            memcpy(&channel_b, &tmp, sizeof(ChannelBlock));
            break;
        }

        case BOX_CMD_COPY_A_TO_B:
            memcpy(&channel_b, &channel_a, sizeof(ChannelBlock));
            break;

        case BOX_CMD_COPY_B_TO_A:
            memcpy(&channel_a, &channel_b, sizeof(ChannelBlock));
            break;

        default:
            break;
    }
}

uint8_t serial_mem_read(uint16_t address) {
    if (address < VIRT_FLASH_END) {
        return read_flash(address);
    }
    else if (address >= VIRT_RAM_BASE && address < VIRT_RAM_END) {
        return read_ram(address);
    }
    else if (address >= VIRT_EEPROM_BASE && address < VIRT_EEPROM_END) {
        return read_eeprom_region(address);
    }
    return 0x00;
}

void serial_mem_write(uint16_t address, uint8_t value) {
    if (address >= VIRT_RAM_BASE && address < VIRT_RAM_END) {
        write_ram(address, value);
    }
    else if (address >= VIRT_EEPROM_BASE && address < VIRT_EEPROM_END) {
        write_eeprom_region(address, value);
    }
}
