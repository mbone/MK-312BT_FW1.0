/*
 * lcd.c - HD44780 LCD Driver (4-bit Mode)
 *
 * Drives a 16x2 character LCD via 4-bit parallel interface on PORTC.
 * The LCD data bus (PC4-PC7) is shared with the button inputs, so
 * lcd_disable_buttons() / lcd_enable_buttons() must bracket every
 * LCD access to avoid bus conflicts.
 *
 * Pin assignments (PORTC):
 *   PC7-PC4: LCD_DB7-DB4 (shared with Menu/Up/OK/Down buttons)
 *   PC3:     LCD_RS (Register Select: 0=command, 1=data)
 *   PC2:     LCD_E  (Enable strobe)
 *   PC1:     LCD_RW (Read/Write: always 0 for write-only)
 *   PC0:     Button activate (active-high enables button pull-ups)
 *
 * Backlight: PD7 (LCD_BACKLIGHT_BIT)
 *
 * The 4-bit init sequence follows the HD44780 datasheet:
 * three 0x30 commands at specific intervals, then 0x20 to enter 4-bit mode.
 */

#include "lcd.h"
#include "avr_registers.h"
#include "MK312BT_Constants.h"
#include <util/delay.h>
#include <avr/pgmspace.h>

/* Strobe the Enable pin to latch data on the LCD */
static void lcd_pulse_enable(void) {
    PORTC |= (1 << LCD_E_BIT);
    _delay_us(1);
    PORTC &= ~(1 << LCD_E_BIT);
    _delay_us(100);
}

/* Write 4-bit nibble to the LCD data bus (upper nibble of PORTC) */
static void lcd_write_nibble(uint8_t nibble) {
    PORTC = (PORTC & ~LCD_DATA_MASK) | (nibble & LCD_DATA_MASK);
    lcd_pulse_enable();
}

/* Send a full byte to the LCD as two nibbles.
 * rs=0 for command, rs=1 for data. High nibble sent first. */
static void lcd_send_byte(uint8_t value, uint8_t rs) {
    if (rs) {
        PORTC |= (1 << LCD_RS_BIT);      /* RS=1: data register */
    } else {
        PORTC &= ~(1 << LCD_RS_BIT);     /* RS=0: command register */
    }
    PORTC &= ~(1 << LCD_RW_BIT);         /* RW=0: write mode */

    lcd_write_nibble(value & 0xF0);       /* High nibble first */
    lcd_write_nibble((value << 4) & 0xF0); /* Low nibble second */
}

/* Send a command byte (RS=0). Clear and Home commands need extra delay. */
static void lcd_command(uint8_t cmd) {
    lcd_send_byte(cmd, 0);
    if (cmd == LCD_CLEAR || cmd == LCD_RETURN_HOME) {
        _delay_ms(2);
    }
}

/* Send a data byte to display (RS=1) */
static void lcd_data(uint8_t data) {
    lcd_send_byte(data, 1);
}

/* Switch PORTC to LCD output mode: set data pins as outputs,
 * deactivate button scanning (PC0 LOW) to avoid bus contention. */
void lcd_disable_buttons(void) {
    PORTC &= ~(LCD_DATA_MASK | (1 << BUTTON_ACTIVATE_BIT));
    DDRC |= LCD_DATA_MASK;
}

/* Switch PORTC to button input mode: set data pins as inputs with
 * pull-ups, activate button scanning (PC0 HIGH). */
void lcd_enable_buttons(void) {
    PORTC &= ~(1 << LCD_E_BIT);
    DDRC &= ~LCD_DATA_MASK;
    PORTC |= LCD_DATA_MASK;
    PORTC |= (1 << BUTTON_ACTIVATE_BIT);
    _delay_us(10);
}

/* Full LCD initialization sequence per HD44780 datasheet.
 * Must be called after power-on with >40ms delay.
 * Initializes 4-bit mode, display on (no cursor), clear, left-to-right. */
void initialize_lcd(void) {
    DDRC |= (1 << BUTTON_ACTIVATE_BIT);
    DDRC |= (1 << LCD_RW_BIT);
    PORTC &= ~(1 << LCD_RW_BIT);  /* RW always low (write-only) */

    lcd_disable_buttons();

    DDRC |= (1 << LCD_RS_BIT) | (1 << LCD_E_BIT) | (1 << LCD_RW_BIT) | LCD_DATA_MASK;

    _delay_ms(50);  /* Wait >40ms after power-on */

    PORTC &= ~(1 << LCD_RS_BIT);
    PORTC &= ~(1 << LCD_E_BIT);

    /* HD44780 initialization by instruction: three 0x30 (8-bit mode) commands */
    lcd_write_nibble(LCD_INIT_8BIT);
    _delay_ms(5);
    lcd_write_nibble(LCD_INIT_8BIT);
    _delay_us(150);
    lcd_write_nibble(LCD_INIT_8BIT);
    _delay_us(150);

    lcd_write_nibble(LCD_INIT_4BIT);

    _delay_us(150);

    lcd_command(LCD_4BIT_MODE);   /* 4-bit, 2-line, 5x8 font (0x28) */
    lcd_command(LCD_DISPLAY_ON);  /* Display on, cursor off (0x0C) */
    lcd_command(LCD_CLEAR);       /* Clear display (0x01) */
    lcd_command(LCD_ENTRY_MODE);  /* Left-to-right, no shift (0x06) */

    lcd_enable_buttons();
}

/* Clear the display and return cursor to home */
void lcd_clear(void) {
    lcd_disable_buttons();
    lcd_command(LCD_CLEAR);
    lcd_enable_buttons();
}

/* Set cursor position. Row 0 starts at DDRAM 0x00, row 1 at 0x40. */
void lcd_set_cursor(uint8_t col, uint8_t row) {
    uint8_t addr = col + (row == 0 ? 0x00 : LCD_ROW1_ADDR);
    lcd_disable_buttons();
    lcd_command(LCD_SET_DDRAM | addr);
    lcd_enable_buttons();
}

void lcd_create_char_P(uint8_t location, const uint8_t charmap[]) {
    lcd_disable_buttons();
    lcd_command(LCD_SET_CGRAM | ((location & 0x07) << 3));
    for (uint8_t i = 0; i < 8; i++) {
        lcd_data(pgm_read_byte(&charmap[i]));
    }
    lcd_enable_buttons();
}

void lcd_write_string_P(const char *str) {
    lcd_disable_buttons();
    char c;
    while ((c = pgm_read_byte(str++)) != '\0') {
        lcd_data((uint8_t)c);
    }
    lcd_enable_buttons();
}

void lcd_command_raw(uint8_t cmd) {
    lcd_command(cmd);
}

void lcd_write_string_raw(const char *str) {
    while (*str) {
        lcd_data((uint8_t)*str++);
    }
}

void lcd_write_custom_char_raw(uint8_t location) {
    lcd_data(location & 0x07);
}

void lcd_set_cursor_raw(uint8_t col, uint8_t row) {
    uint8_t addr = col + (row == 0 ? 0x00 : LCD_ROW1_ADDR);
    lcd_command(LCD_SET_DDRAM | addr);
}

/* Turn LCD backlight on (PD7 low = active low) */
void lcd_backlight_on(void) {
    PORTD &= ~(1 << LCD_BACKLIGHT_BIT);
}

/* Display a progress bar on LCD row 2 (16 characters wide).
 * Filled positions use 0xFF (full block), empty use space. */
void lcd_show_progress(uint8_t step, uint8_t total) {
    if (total == 0) return;
    uint8_t filled = (step * 16) / total;
    lcd_disable_buttons();
    lcd_command(LCD_SET_DDRAM | LCD_ROW1_ADDR);
    for (uint8_t i = 0; i < 16; i++) {
        if (i < filled) {
            lcd_data(0xFF);    /* Full block character */
        } else {
            lcd_data(' ');
        }
    }
    lcd_enable_buttons();
}
