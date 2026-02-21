/*
 * menu.c - Menu System and User Interface
 *
 * Implements the 4-button (Up/Down/OK/Menu) LCD menu system with
 * four hierarchical screens:
 *
 *   MENU_MAIN:        Mode selection + status display
 *     Row 1: "A## B## Mode  %%"  (channel levels, mode name, battery)
 *     Row 2: "<> Select Mode" or ramp progress or low battery warning
 *     Up/Down: cycle modes, OK: start ramp, Menu: enter options
 *
 *   MENU_OPTIONS:     Settings submenu
 *     0: Start Ramp Up    - begin gradual intensity ramp
 *     1: Set As Favorite  - save current mode to EEPROM
 *     2: Set Power Level  - enter power level submenu
 *     3: Adjust Advanced  - enter advanced settings submenu
 *     4: Save Settings    - persist all settings to EEPROM
 *     5: Reset Settings   - restore factory defaults
 *
 *   MENU_POWER_LEVEL: Low/Normal/High power selection
 *
 *   MENU_ADVANCED:    8 advanced parameter adjustments (0-255 each)
 *     Ramp Level, Ramp Time, Depth, Tempo, Frequency, Effect, Width, Pace
 *
 * All display strings are stored in PROGMEM to conserve SRAM.
 * Custom LCD characters (0-4) are used for battery level icons.
 */

#include "menu.h"
#include "lcd.h"
#include "MK312BT_Constants.h"
#include "eeprom.h"
#include "mode_dispatcher.h"
#include "user_programs.h"
#include "adc.h"
#include "config.h"
#include "MK312BT_Modes.h"
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdbool.h>
#include <string.h>

/* ---- PROGMEM String Tables ---- */

const char mode_names[] PROGMEM =
    "Waves\0Stroke\0Climb\0Combo\0Intense\0"
    "Rhythm\0Audio1\0Audio2\0Audio3\0Random1\0"
    "Random2\0Toggle\0Orgasm\0Torment\0Phase1\0"
    "Phase2\0Phase3\0User1\0User2\0User3\0"
    "User4\0User5\0User6\0User7\0Split\0";

const char option_names[] PROGMEM =
    "Start Ramp Up?  \0Config Split?   \0Set As Favorite?\0Set Pwr Level?  \0"
    "Adjust Advanced?\0Save Settings?  \0Reset Settings? \0";

const char power_level_names[] PROGMEM =
    "Pwr Lev: Low    \0Pwr Lev: Normal \0Pwr Lev: High   \0";

const char advanced_names[] PROGMEM =
    "RampLevl Adjust?\0RampTime Adjust?\0"
    "   Depth Adjust?\0   Tempo Adjust?\0"
    "   Freq. Adjust?\0  Effect Adjust?\0"
    "   Width Adjust?\0    Pace Adjust?\0";

const char split_hdr_a[] PROGMEM = "Split: Ch.A Mode";
const char split_hdr_b[] PROGMEM = "Split: Ch.B Mode";
const char nav_hint[] PROGMEM = "Press <- or OK >";

/* ---- State Variables ---- */

MenuState menu_state;
char line_buffer[17];           /* LCD line formatting buffer (16 chars + null) */
char line_buffer2[17];          /* Second line buffer for mode name display */

extern eeprom_config_t g_menu_config;  /* Shared EEPROM config from main */

static bool ramp_up_active = false;     /* True while intensity ramp is running */
static uint8_t ramp_counter = 0;        /* Ramp progress 0-100% */

static bool menu_needs_clear = true;        /* Set on menu transitions, cleared after first draw */
static bool output_enabled = false;         /* Output off until user starts ramp */

/* Split sub-menu state: 0 = selecting ch.A mode, 1 = selecting ch.B mode */
static uint8_t split_edit_channel = 0;
static uint8_t split_mode_a_sel = 0;
static uint8_t split_mode_b_sel = 0;
static uint8_t split_prev_mode = 0;

/* ---- Helper Functions ---- */

static void copy_progmem_string(char* dest, const char* table, uint8_t index);
static uint8_t cycle_index(uint8_t current, uint8_t max_val, bool forward);
static uint8_t cycle_mode(uint8_t current, bool forward);
static bool mode_is_available(uint8_t mode);
static void display_split_channel(void);
static void display_generic_menu(const char* text);
static uint8_t get_battery_percent(void);
static void return_to_main(void);

/* Copy the nth null-terminated string from a flat PROGMEM string pack */
static void copy_progmem_string(char* dest, const char* table, uint8_t index) {
    const char* p = table;
    while (index--) {
        while (pgm_read_byte(p++) != '\0') {}
    }
    strcpy_P(dest, p);
}

static char* fmt_u8_2(char* p, uint8_t v) {
    *p++ = '0' + (v / 10);
    *p++ = '0' + (v % 10);
    return p;
}

static char* fmt_u8_3(char* p, uint8_t v) {
    if (v >= 100) {
        *p++ = '0' + (v / 100);
        v %= 100;
        *p++ = '0' + (v / 10);
    } else if (v >= 10) {
        *p++ = ' ';
        *p++ = '0' + (v / 10);
    } else {
        *p++ = ' ';
        *p++ = ' ';
    }
    *p++ = '0' + (v % 10);
    return p;
}

static char* fmt_str_padded8(char* p, const char* src) {
    uint8_t n = 0;
    while (n < 8 && src[n]) { *p++ = src[n++]; }
    while (n++ < 8) *p++ = ' ';
    return p;
}

/* Wrap-around index increment/decrement for circular menu navigation */
static uint8_t cycle_index(uint8_t current, uint8_t max_val, bool forward) {
    if (forward) {
        return (current + 1) % max_val;
    } else {
        return (current == 0) ? (max_val - 1) : (current - 1);
    }
}

/* Returns true if a mode should be shown in the mode cycle.
 * User modes (17-23) are only shown if data is loaded in that slot.
 * Split mode (24) is configured via the Options menu, not mode cycle. */
static bool mode_is_available(uint8_t mode) {
    if (mode == MODE_SPLIT) return false;
    if (mode >= MODE_USER1 && mode < MODE_SPLIT) {
        return user_prog_is_valid(mode - MODE_USER1) != 0;
    }
    return true;
}

/* Advance the mode index forward or backward, skipping unavailable modes.
 * Guaranteed to return a valid mode (will wrap fully if needed). */
static uint8_t cycle_mode(uint8_t current, bool forward) {
    uint8_t next = cycle_index(current, MODE_COUNT, forward);
    uint8_t tries = MODE_COUNT;
    while (!mode_is_available(next) && tries > 0) {
        next = cycle_index(next, MODE_COUNT, forward);
        tries--;
    }
    if (!mode_is_available(next)) return current;
    return next;
}

/* Show the split sub-menu for channel A or B selection. */
static void display_split_channel(void) {
    uint8_t sel = (split_edit_channel == 0) ? split_mode_a_sel : split_mode_b_sel;
    lcd_disable_buttons();
    lcd_command_raw(LCD_CLEAR);
    if (split_edit_channel == 0) {
        strcpy_P(line_buffer, split_hdr_a);
    } else {
        strcpy_P(line_buffer, split_hdr_b);
    }
    lcd_write_string_raw(line_buffer);
    lcd_set_cursor_raw(0, 1);
    copy_progmem_string(line_buffer2, mode_names, sel);
    char* _p = line_buffer;
    _p = fmt_str_padded8(_p, line_buffer2);
    _p[0]=' '; _p[1]='O'; _p[2]='K'; _p[3]='/'; _p[4]='M'; _p[5]='e'; _p[6]='n'; _p[7]='u'; _p[8]='\0';
    lcd_write_string_raw(line_buffer);
    lcd_enable_buttons();
}

/* Display a standard menu screen: text on row 1, navigation hint on row 2.
 * Keeps buttons disabled for the entire write to avoid bus contention
 * when a button is still held during the screen redraw. */
static void display_generic_menu(const char* text) {
    lcd_disable_buttons();
    lcd_command_raw(LCD_CLEAR);
    lcd_write_string_raw(text);
    lcd_set_cursor_raw(0, 1);
    strcpy_P(line_buffer, nav_hint);
    lcd_write_string_raw(line_buffer);
    lcd_enable_buttons();
}

static void return_to_main(void) {
    mode_dispatcher_resume();
    menu_state.current_menu = MENU_MAIN;
    menu_needs_clear = true;
    menuShowMode(g_menu_config.top_mode);
}

/* Convert raw battery ADC reading to 0-100%.
 * ADC 584 = 0% (empty), ADC 676 = 100% (full).
 * Range corresponds to ~9.4V-10.9V through voltage divider. */
static uint8_t get_battery_percent(void) {
    uint16_t battery = adc_read_battery();
    if (battery < BATTERY_ADC_EMPTY) return 0;
    if (battery > BATTERY_ADC_FULL) return 100;

    uint8_t percent = ((battery - BATTERY_ADC_EMPTY) * 100) / BATTERY_ADC_RANGE;
    return percent;
}

/* ---- Public Functions ---- */

/* Initialize menu state and define custom LCD battery characters (slots 0-4) */
void menuInit(void) {
    menu_state.current_menu = MENU_MAIN;
    menu_state.menu_position = 0;
    menu_state.edit_mode = false;
    ramp_up_active = false;
    ramp_counter = 0;
    menu_needs_clear = true;
    output_enabled = false;

    static const uint8_t battery_empty[8] PROGMEM = {0x0E, 0x1F, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F};
    static const uint8_t battery_low[8]   PROGMEM = {0x0E, 0x1F, 0x11, 0x11, 0x11, 0x11, 0x1F, 0x1F};
    static const uint8_t battery_mid[8]   PROGMEM = {0x0E, 0x1F, 0x11, 0x11, 0x1F, 0x1F, 0x1F, 0x1F};
    static const uint8_t battery_high[8]  PROGMEM = {0x0E, 0x1F, 0x11, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
    static const uint8_t battery_full[8]  PROGMEM = {0x0E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
    lcd_create_char_P(0, battery_empty);
    lcd_create_char_P(1, battery_low);
    lcd_create_char_P(2, battery_mid);
    lcd_create_char_P(3, battery_high);
    lcd_create_char_P(4, battery_full);
}

/* Show startup splash screen with firmware version and battery level */
void menuShowStartup(void) {
    lcd_clear();
    lcd_set_cursor(0, 1);
    lcd_write_string_P(PSTR("Press Any Key..."));
    lcd_set_cursor(0, 0);

    uint16_t battery = adc_read_battery();
    uint8_t percent = (battery > BATTERY_ADC_EMPTY) ? ((battery - BATTERY_ADC_EMPTY) * 100) / BATTERY_ADC_RANGE : 0;
    if (percent > 100) percent = 100;

    {
        char* _p = line_buffer;
        _p[0]='B'; _p[1]='a'; _p[2]='t'; _p[3]='t'; _p[4]='e'; _p[5]='r'; _p[6]='y'; _p[7]=':'; _p[8]=' ';
        _p += 9;
        if (percent == 100) { _p[0]='1'; _p[1]='0'; _p[2]='0'; _p += 3; }
        else { if (percent >= 10) *_p++ = '0' + (percent / 10); *_p++ = '0' + (percent % 10); }
        *_p++ = '%'; *_p = '\0';
    }
    lcd_disable_buttons();
    lcd_write_string_raw(line_buffer);
    lcd_enable_buttons();

    _delay_ms(1500);
}

/* Display the main mode screen with channel levels and mode name.
 * Row 1: "A## B## ModeName"
 * Row 2: ramp progress or navigation hint */
void menuShowMode(uint8_t mode_index) {
    if (mode_index >= MODE_COUNT) mode_index = 0;

    lcd_disable_buttons();

    if (menu_needs_clear) {
        lcd_command_raw(LCD_CLEAR);
        menu_needs_clear = false;
    }

    lcd_set_cursor_raw(0, 0);

    uint16_t level_a = adc_read_level_a();
    uint16_t level_b = adc_read_level_b();
    uint8_t pct_a = (uint8_t)((level_a * 99UL + 511) / 1023);
    uint8_t pct_b = (uint8_t)((level_b * 99UL + 511) / 1023);
    if (pct_a > 99) pct_a = 99;
    if (pct_b > 99) pct_b = 99;

    copy_progmem_string(line_buffer2, mode_names, mode_index);

    {
        char* _p = line_buffer;
        *_p++ = 'A';
        _p = fmt_u8_2(_p, pct_a);
        *_p++ = ' ';
        *_p++ = 'B';
        _p = fmt_u8_2(_p, pct_b);
        *_p++ = ' ';
        _p = fmt_str_padded8(_p, line_buffer2);
        *_p = '\0';
    }
    lcd_write_string_raw(line_buffer);

    lcd_set_cursor_raw(0, 1);
    if (ramp_up_active) {
        {
            char* _p = line_buffer;
            _p[0]='R'; _p[1]='a'; _p[2]='m'; _p[3]='p'; _p[4]=':'; _p[5]=' '; _p += 6;
            _p = fmt_u8_2(_p, ramp_counter);
            _p[0]=' '; _p[1]='<'; _p[2]='>'; _p[3]='='; _p[4]='M'; _p[5]='o'; _p[6]='d'; _p[7]='e'; _p[8]='\0';
        }
        lcd_write_string_raw(line_buffer);
    } else {
        uint8_t batt_pct = get_battery_percent();
        uint8_t batt_icon;
        if (batt_pct >= 80)      batt_icon = 4;
        else if (batt_pct >= 60) batt_icon = 3;
        else if (batt_pct >= 40) batt_icon = 2;
        else if (batt_pct >= 20) batt_icon = 1;
        else                     batt_icon = 0;
        lcd_write_string_raw("<> Select Mode");
        lcd_set_cursor_raw(15, 1);
        lcd_write_custom_char_raw(batt_icon);
    }

    lcd_enable_buttons();
}

/* Advance ramp counter. Called periodically from main loop.
 * adv_ramp_time (0-255) controls speed: lower = faster.
 * At 0 the ramp completes instantly; at 255 it takes ~8x longer than default. */
void menuHandleRampUp(void) {
    if (!ramp_up_active) return;

    static uint8_t ramp_sub = 0;
    system_config_t* cfg = config_get();
    uint8_t divisor = (cfg->adv_ramp_time >> 5) + 1;

    ramp_sub++;
    if (ramp_sub < divisor) return;
    ramp_sub = 0;

    ramp_counter++;

    if (ramp_counter >= 100) {
        ramp_counter = 100;
        ramp_up_active = false;
        if (menu_state.current_menu == MENU_MAIN) {
            menu_needs_clear = true;
            menuShowMode(g_menu_config.top_mode);
        }
    }
}

/* Begin intensity ramp-up sequence */
void menuStartRamp(void) {
    ramp_up_active = true;
    ramp_counter = 0;
    menu_needs_clear = true;
    menuShowMode(g_menu_config.top_mode);
}

bool menuIsOutputEnabled(void) {
    return output_enabled;
}

uint8_t menuGetRampPercent(void) {
    if (!ramp_up_active) {
        return output_enabled ? 100 : 0;
    }
    return ramp_counter;
}

void menuStartOutput(void) {
    output_enabled = true;
    menu_needs_clear = true;
    menuStartRamp();
}

void menuStopOutput(void) {
    output_enabled = false;
    ramp_up_active = false;
    ramp_counter = 0;
}

/* ---- Menu Screen Handlers ---- */

/* Main screen: Up/Down cycle modes with auto-ramp, OK restarts ramp, Menu enters options */
static void menu_handle_main(ButtonEvent event) {
    switch (event) {
        case BUTTON_UP:
            g_menu_config.top_mode = cycle_mode(g_menu_config.top_mode, true);
            menuStartOutput();
            menuShowMode(g_menu_config.top_mode);
            break;

        case BUTTON_DOWN:
            g_menu_config.top_mode = cycle_mode(g_menu_config.top_mode, false);
            menuStartOutput();
            menuShowMode(g_menu_config.top_mode);
            break;

        case BUTTON_OK:
            menuStartOutput();
            break;

        case BUTTON_MENU:
            mode_dispatcher_pause();
            menu_state.current_menu = MENU_OPTIONS;
            menu_state.menu_position = 0;
            copy_progmem_string(line_buffer, option_names, 0);
            display_generic_menu(line_buffer);
            break;

        default:
            break;
    }
}

/* Options screen: cycle through 6 options, OK executes selected action */
static void menu_handle_options(ButtonEvent event) {
    switch (event) {
        case BUTTON_UP:
            menu_state.menu_position = cycle_index(menu_state.menu_position, OPTION_COUNT, true);
            copy_progmem_string(line_buffer, option_names, menu_state.menu_position);
            display_generic_menu(line_buffer);
            break;

        case BUTTON_DOWN:
            menu_state.menu_position = cycle_index(menu_state.menu_position, OPTION_COUNT, false);
            copy_progmem_string(line_buffer, option_names, menu_state.menu_position);
            display_generic_menu(line_buffer);
            break;

        case BUTTON_OK:
            switch (menu_state.menu_position) {
                case 0:  /* Start Ramp Up */
                    return_to_main();
                    menuStartOutput();
                    break;

                case 1:  /* Config Split */
                    split_prev_mode = g_menu_config.top_mode;
                    split_edit_channel = 0;
                    split_mode_a_sel = mode_dispatcher_get_split_mode_a();
                    split_mode_b_sel = mode_dispatcher_get_split_mode_b();
                    menu_state.current_menu = MENU_SPLIT;
                    display_split_channel();
                    break;

                case 2:  /* Set As Favorite */
                    g_menu_config.favorite_mode = g_menu_config.top_mode;
                    eeprom_save_config(&g_menu_config);
                    lcd_clear();
                    lcd_write_string_P(PSTR("Favorite Saved!"));
                    _delay_ms(1000);
                    return_to_main();
                    break;

                case 3:  /* Set Power Level */
                    menu_state.current_menu = MENU_POWER_LEVEL;
                    if (g_menu_config.power_level > 2) g_menu_config.power_level = 1;
                    menu_state.menu_position = g_menu_config.power_level;
                    copy_progmem_string(line_buffer, power_level_names, g_menu_config.power_level);
                    display_generic_menu(line_buffer);
                    break;

                case 4:  /* Adjust Advanced */
                    menu_state.current_menu = MENU_ADVANCED;
                    menu_state.menu_position = 0;
                    copy_progmem_string(line_buffer, advanced_names, 0);
                    display_generic_menu(line_buffer);
                    break;

                case 5:  /* Save Settings */
                    eeprom_save_config(&g_menu_config);
                    lcd_clear();
                    lcd_write_string_P(PSTR("Settings Saved!"));
                    _delay_ms(1000);
                    return_to_main();
                    break;

                case 6:  /* Reset Settings */
                    eeprom_init_defaults(&g_menu_config);
                    eeprom_save_config(&g_menu_config);
                    lcd_clear();
                    lcd_write_string_P(PSTR("Settings Reset!"));
                    _delay_ms(1000);
                    return_to_main();
                    break;
            }
            break;

        case BUTTON_MENU:
            return_to_main();
            break;

        default:
            break;
    }
}

/* Power level screen: Up/Down select Low/Normal/High, OK saves and returns */
static void menu_handle_power_level(ButtonEvent event) {
    switch (event) {
        case BUTTON_UP:
            if (g_menu_config.power_level < 2) {
                g_menu_config.power_level++;
                copy_progmem_string(line_buffer, power_level_names, g_menu_config.power_level);
                display_generic_menu(line_buffer);
            }
            break;

        case BUTTON_DOWN:
            if (g_menu_config.power_level > 0) {
                g_menu_config.power_level--;
                copy_progmem_string(line_buffer, power_level_names, g_menu_config.power_level);
                display_generic_menu(line_buffer);
            }
            break;

        case BUTTON_OK:
        case BUTTON_MENU:
            menu_state.current_menu = MENU_OPTIONS;
            menu_state.menu_position = 3;
            copy_progmem_string(line_buffer, option_names, 3);
            display_generic_menu(line_buffer);
            break;

        default:
            break;
    }
}

/* Advanced settings screen: cycle 8 parameters, OK enters value edit mode */
static void menu_handle_advanced(ButtonEvent event) {
    switch (event) {
        case BUTTON_UP:
            menu_state.menu_position = cycle_index(menu_state.menu_position, ADVANCED_COUNT, true);
            copy_progmem_string(line_buffer, advanced_names, menu_state.menu_position);
            display_generic_menu(line_buffer);
            break;

        case BUTTON_DOWN:
            menu_state.menu_position = cycle_index(menu_state.menu_position, ADVANCED_COUNT, false);
            copy_progmem_string(line_buffer, advanced_names, menu_state.menu_position);
            display_generic_menu(line_buffer);
            break;

        case BUTTON_OK: {
            menu_state.edit_mode = true;
            lcd_disable_buttons();
            lcd_command_raw(LCD_CLEAR);
            lcd_set_cursor_raw(0, 0);
            copy_progmem_string(line_buffer, advanced_names, menu_state.menu_position);
            lcd_write_string_raw(line_buffer);
            lcd_set_cursor_raw(0, 1);

            uint8_t value = 0;
            switch (menu_state.menu_position) {
                case 0: value = g_menu_config.adv_ramp_level; break;
                case 1: value = g_menu_config.adv_ramp_time; break;
                case 2: value = g_menu_config.adv_depth; break;
                case 3: value = g_menu_config.adv_tempo; break;
                case 4: value = g_menu_config.adv_frequency; break;
                case 5: value = g_menu_config.adv_effect; break;
                case 6: value = g_menu_config.adv_width; break;
                case 7: value = g_menu_config.adv_pace; break;
            }
            {
                char* _p = line_buffer;
                _p[0]='V'; _p[1]='a'; _p[2]='l'; _p[3]='u'; _p[4]='e'; _p[5]=':'; _p[6]=' '; _p += 7;
                _p = fmt_u8_3(_p, value);
                _p[0]=' '; _p[1]=' '; _p[2]=' '; _p[3]=' '; _p[4]=' '; _p[5]=' '; _p[6]='\0';
            }
            lcd_write_string_raw(line_buffer);
            lcd_enable_buttons();
            break;
        }

        case BUTTON_MENU:
            menu_state.current_menu = MENU_OPTIONS;
            menu_state.menu_position = 4;
            copy_progmem_string(line_buffer, option_names, menu_state.menu_position);
            display_generic_menu(line_buffer);
            break;

        default:
            break;
    }
}

/* Advanced value edit mode: Up/Down increment/decrement value (0-255),
 * OK or Menu exits edit mode and returns to parameter selection */
static void menu_handle_advanced_edit(ButtonEvent event) {
    uint8_t* value_ptr = NULL;

    /* Map menu position to the config field being edited */
    switch (menu_state.menu_position) {
        case 0: value_ptr = &g_menu_config.adv_ramp_level; break;
        case 1: value_ptr = &g_menu_config.adv_ramp_time; break;
        case 2: value_ptr = &g_menu_config.adv_depth; break;
        case 3: value_ptr = &g_menu_config.adv_tempo; break;
        case 4: value_ptr = &g_menu_config.adv_frequency; break;
        case 5: value_ptr = &g_menu_config.adv_effect; break;
        case 6: value_ptr = &g_menu_config.adv_width; break;
        case 7: value_ptr = &g_menu_config.adv_pace; break;
    }

    if (value_ptr == NULL) return;

    switch (event) {
        case BUTTON_UP:
            if (*value_ptr < 255) {
                (*value_ptr)++;
                lcd_disable_buttons();
                lcd_set_cursor_raw(0, 1);
                { char* _p = line_buffer; _p[0]='V'; _p[1]='a'; _p[2]='l'; _p[3]='u'; _p[4]='e'; _p[5]=':'; _p[6]=' '; _p += 7; _p = fmt_u8_3(_p, *value_ptr); _p[0]=' '; _p[1]=' '; _p[2]=' '; _p[3]=' '; _p[4]=' '; _p[5]=' '; _p[6]='\0'; }
                lcd_write_string_raw(line_buffer);
                lcd_enable_buttons();
            }
            break;

        case BUTTON_DOWN:
            if (*value_ptr > 0) {
                (*value_ptr)--;
                lcd_disable_buttons();
                lcd_set_cursor_raw(0, 1);
                { char* _p = line_buffer; _p[0]='V'; _p[1]='a'; _p[2]='l'; _p[3]='u'; _p[4]='e'; _p[5]=':'; _p[6]=' '; _p += 7; _p = fmt_u8_3(_p, *value_ptr); _p[0]=' '; _p[1]=' '; _p[2]=' '; _p[3]=' '; _p[4]=' '; _p[5]=' '; _p[6]='\0'; }
                lcd_write_string_raw(line_buffer);
                lcd_enable_buttons();
            }
            break;

        case BUTTON_OK:
        case BUTTON_MENU:
            menu_state.edit_mode = false;
            copy_progmem_string(line_buffer, advanced_names, menu_state.menu_position);
            display_generic_menu(line_buffer);
            break;

        default:
            break;
    }
}

/* Split sub-menu: pick the mode for channel A then channel B.
 * Up/Down cycle through built-in modes only (user/split excluded).
 * OK advances to ch.B then commits; Menu cancels back to main. */
static void menu_handle_split(ButtonEvent event) {
    uint8_t *sel = (split_edit_channel == 0) ? &split_mode_a_sel : &split_mode_b_sel;

    switch (event) {
        case BUTTON_UP: {
            uint8_t next = cycle_index(*sel, MODE_USER1, true);
            *sel = next;
            display_split_channel();
            break;
        }

        case BUTTON_DOWN: {
            uint8_t next = cycle_index(*sel, MODE_USER1, false);
            *sel = next;
            display_split_channel();
            break;
        }

        case BUTTON_OK:
            if (split_edit_channel == 0) {
                split_edit_channel = 1;
                display_split_channel();
            } else {
                mode_dispatcher_set_split_modes(split_mode_a_sel, split_mode_b_sel);
                menu_state.current_menu = MENU_OPTIONS;
                menu_state.menu_position = 1;
                copy_progmem_string(line_buffer, option_names, 1);
                display_generic_menu(line_buffer);
            }
            break;

        case BUTTON_MENU:
            menu_state.current_menu = MENU_OPTIONS;
            menu_state.menu_position = 1;
            copy_progmem_string(line_buffer, option_names, 1);
            display_generic_menu(line_buffer);
            break;

        default:
            break;
    }
}

/* Main button dispatch: routes events to the active menu screen handler.
 * If in advanced edit mode, routes to the value editor instead. */
void menuHandleButton(ButtonEvent event) {
    if (menu_state.edit_mode) {
        menu_handle_advanced_edit(event);
        return;
    }

    switch (menu_state.current_menu) {
        case MENU_MAIN:
            menu_handle_main(event);
            break;

        case MENU_OPTIONS:
            menu_handle_options(event);
            break;

        case MENU_POWER_LEVEL:
            menu_handle_power_level(event);
            break;

        case MENU_ADVANCED:
            menu_handle_advanced(event);
            break;

        case MENU_SPLIT:
            menu_handle_split(event);
            break;

        default:
            return_to_main();
            break;
    }
}

bool menuIsRampActive(void) {
    return ramp_up_active;
}
