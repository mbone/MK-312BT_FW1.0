/*
 * lcd.h - HD44780 LCD Driver (4-bit Mode)
 *
 * 16x2 character LCD interface shared with button inputs on PORTC.
 * All LCD functions automatically manage the button/LCD bus sharing
 * via lcd_disable_buttons() / lcd_enable_buttons() pairs.
 */

#ifndef LCD_H
#define LCD_H

#include <stdint.h>
#include <avr/pgmspace.h>

#ifdef __cplusplus
extern "C" {
#endif

void initialize_lcd(void);                                    /* Full HD44780 init sequence */
void lcd_clear(void);                                         /* Clear display and home cursor */
void lcd_set_cursor(uint8_t col, uint8_t row);               /* Set cursor position (0-15, 0-1) */
void lcd_write_string_P(const char *str);                     /* Write null-terminated PROGMEM string */
void lcd_create_char_P(uint8_t location, const uint8_t charmap[]); /* Define custom char from PROGMEM */
void lcd_show_progress(uint8_t step, uint8_t total);          /* Draw progress bar on row 2 */
void lcd_enable_buttons(void);                                /* Switch PORTC to button input mode */
void lcd_disable_buttons(void);                               /* Switch PORTC to LCD output mode */
void lcd_backlight_on(void);                                  /* PD7 high */
void lcd_command_raw(uint8_t cmd);
void lcd_write_string_raw(const char *str);
void lcd_write_custom_char_raw(uint8_t location);
void lcd_set_cursor_raw(uint8_t col, uint8_t row);

#ifdef __cplusplus
}
#endif

#endif
