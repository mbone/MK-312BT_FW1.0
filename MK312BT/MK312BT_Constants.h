/*
 * MK312BT_Constants.h - Hardware Constants and Pin Definitions
 *
 * ATmega16 running at 8 MHz. Defines:
 *   - CPU clock and USART baud rate calculation
 *   - ADC voltage reference setting
 *   - SPI master mode configuration
 *   - HD44780 LCD command bytes
 *   - Complete pin assignment map for all 4 ports (PA, PB, PC, PD)
 *   - Logical pin aliases for ADC channels, buttons, LCD bus, DAC
 *   - LTC1661 DAC command codes (load, update, sleep/wake)
 */

#ifndef MK312BT_CONSTANTS_H
#define MK312BT_CONSTANTS_H

#include <stdint.h>

/* CPU clock: 8 MHz internal RC oscillator */
#define F_CPU 8000000UL

/* USART: 19200 baud for serial link via MAX232 */
#define USART_BAUD_RATE 19200
#define USART_UBRR_VALUE ((F_CPU / (16UL * USART_BAUD_RATE)) - 1)

/* ADC: use AVCC (5V) as voltage reference */
#define ADC_VREF_AVCC 0x40

/* SPI: master mode, fosc/4 base clock */
#define SPI_MASTER_MODE 0x50
#define SPI_CLOCK_DIV_16 0x01

/* HD44780 LCD initialization commands */
#define LCD_4BIT_MODE 0x28   /* 4-bit interface, 2 lines, 5x8 font */
#define LCD_DISPLAY_ON 0x0C  /* Display on, cursor off, blink off */
#define LCD_CLEAR 0x01       /* Clear display and return home */
#define LCD_ENTRY_MODE 0x06  /* Increment cursor, no display shift */

/* ---- Port A: ADC inputs (directly wired to analog signals) ---- */
#define  PA7  31  /* Line In R - low-pass filtered right audio */
#define  PA6  30  /* Mixed Line In L / Mic/Remote - low-pass filtered */
#define  PA5  29  /* Channel level B - DAC B output feedback */
#define  PA4  28  /* Channel level A - DAC A output feedback */
#define  PA3  27  /* 12V measurement - battery/supply voltage divider */
#define  PA2  26  /* V+ measurement - main supply voltage divider */
#define  PA1  25  /* Multi-Adjust pot (VR3G$1) / LINE1 IN */
#define  PA0  24  /* Output current sense - H-bridge current monitor */

/* ---- Port B: H-bridge FET gates and SPI bus ---- */
#define  PB7   7  /* SCK - SPI clock to LTC1661 DAC pin 2 / ISP header */
#define  PB6   6  /* MISO - ISP header only (not connected to DAC) */
#define  PB5   5  /* MOSI - SPI data to LTC1661 DAC pin 3 / ISP header */
#define  PB4   4  /* Not connected */
#define  PB3   3  /* Output A Gate- (negative FET of Channel A H-bridge) */
#define  PB2   2  /* Output A Gate+ (positive FET of Channel A H-bridge) */
#define  PB1   1  /* Output B Gate- (negative FET of Channel B H-bridge) */
#define  PB0   0  /* Output B Gate+ (positive FET of Channel B H-bridge) */

/* ---- Port C: LCD data bus (shared with buttons) and LCD control ---- */
#define  PC7  23  /* LCD_DB7 / Menu button      (active low via PC0) */
#define  PC6  22  /* LCD_DB6 / Right/Up button   (active low via PC0) */
#define  PC5  21  /* LCD_DB5 / OK button         (active low via PC0) */
#define  PC4  20  /* LCD_DB4 / Left/Down button   (active low via PC0) */
#define  PC3  19  /* LCD_RS - Register Select (0=command, 1=data) */
#define  PC2  18  /* LCD_E  - Enable strobe (active high pulse) */
#define  PC1  17  /* LCD_RW - Read/Write (0=write, 1=read) */
#define  PC0  16  /* Button activate - pull low to read buttons on PC4-PC7 */

/* ---- Port D: LEDs, DAC chip select, audio switches, USART ---- */
#define  PD7  15  /* LCD backlight cathode (active low = backlight on) */
#define  PD6  14  /* Output LED1 (Channel A activity indicator) */
#define  PD5  13  /* Output LED2 (Channel B activity indicator) */
#define  PD4  12  /* CS/LD - LTC1661 DAC chip select (active low) */
#define  PD3  11  /* Multi-Adjust VR3G$1 - Line In R audio switch */
#define  PD2  10  /* Multi-Adjust VR3G$2 - Mixed Line In L / Mic switch */
#define  PD1  9   /* USART TXD via MAX232 level shifter */
#define  PD0  8   /* USART RXD via MAX232 level shifter */

/* Logical pin aliases for ADC channels */
#define ADC_CHANNEL_LEVEL_B_PIN   PA5  /* DAC B output feedback */
#define ADC_CHANNEL_LEVEL_A_PIN   PA4  /* DAC A output feedback */
#define ADC_MULTI_ADJ_VR3G1       PA1  /* Multi-Adjust potentiometer */

/* Button pin aliases (directly sampled when PC0 pulled low) */
#define BUTTON_MENU_PIN           PC7
#define BUTTON_UP_PIN             PC6
#define BUTTON_OK_PIN             PC5
#define BUTTON_DOWN_PIN           PC4

/* LCD bus bit positions within PORTC */
#define LCD_DB7_BIT               7
#define LCD_DB6_BIT               6
#define LCD_DB5_BIT               5
#define LCD_DB4_BIT               4
#define LCD_RS_BIT                3    /* Register Select */
#define LCD_E_BIT                 2    /* Enable strobe */
#define LCD_RW_BIT                1    /* Read/Write */
#define BUTTON_ACTIVATE_BIT       0    /* HIGH = buttons active, LOW = LCD mode */
#define LCD_DATA_MASK             0xF0 /* Upper nibble mask for 4-bit mode */

/* LCD backlight control (Port D) */
#define LCD_BACKLIGHT_BIT         7    /* PD7: active low */

/* PORTD bit positions (used for direct register access, NOT Arduino pin numbers) */
#define PORTD_BIT_BACKLIGHT       7   /* PD7: LCD backlight (active low) */
#define PORTD_BIT_LED_A           6   /* PD6: Channel A activity LED */
#define PORTD_BIT_LED_B           5   /* PD5: Channel B activity LED */
#define PORTD_BIT_DAC_CS          4   /* PD4: LTC1661 DAC chip select (active low) */

/* DAC chip select (directly driven, active low) */
#define DAC_CS_LD                 PORTD_BIT_DAC_CS

/* LTC1661 DAC command codes (upper nibble of 16-bit command word)
 * Format: [CMD:4][DATA:10][XX:2] - 10-bit data left-justified */
#define DAC_CMD_LOAD_A  0x10  /* Load DAC A input register (no update) */
#define DAC_CMD_LOAD_B  0x20  /* Load DAC B input register (no update) */
#define DAC_CMD_UPDATE  0x80  /* Update both DAC outputs from input registers */
#define DAC_CMD_LOUPA   0x90  /* Load and update DAC A simultaneously */
#define DAC_CMD_LOUPB   0xA0  /* Load and update DAC B simultaneously */
#define DAC_CMD_SLEEP   0xE0  /* Enter low-power sleep mode */
#define DAC_CMD_WAKE    0xD0  /* Wake from sleep mode */
#define DAC_MAX_VALUE   1023  /* 10-bit DAC maximum output value */

/* Logical pin aliases for remaining ADC channels */
#define ADC_AUDIO_A_PIN           PA7  /* Right line-in audio input */
#define ADC_AUDIO_B_PIN           PA6  /* Left line-in / mic audio input */
#define ADC_BATTERY_PIN           PA3  /* Battery voltage (12V divider) */
#define ADC_CURRENT_SENSE_PIN     PA0  /* H-bridge output current monitor */

/* ADC MUX channel indices (ATmega16 MUX select bits for round-robin sampling) */
#define ADC_MUX_LEVEL_A    0  /* PA4 - Channel A intensity pot */
#define ADC_MUX_LEVEL_B    1  /* PA5 - Channel B intensity pot */
#define ADC_MUX_MA         2  /* PA1 - Multi-Adjust knob */
#define ADC_MUX_AUDIO_A    3  /* PA7 - Right line-in audio */
#define ADC_MUX_AUDIO_B    4  /* PA6 - Left line-in / mic audio */
#define ADC_MUX_BATTERY    5  /* PA3 - Battery voltage (12V divider) */
#define ADC_MUX_CURRENT    0  /* PA0 - Output current sense (direct HW channel) */
#define ADC_MUX_COUNT      6

/* ADC signal constants */
#define ADC_CENTER_POINT   512  /* AC-coupled audio signal center (half of 10-bit range) */

/* H-bridge FET control masks (PORTB bit positions) */
#define HBRIDGE_CH_A_POS   PB2  /* Channel A positive FET gate */
#define HBRIDGE_CH_A_NEG   PB3  /* Channel A negative FET gate */
#define HBRIDGE_CH_B_POS   PB0  /* Channel B positive FET gate */
#define HBRIDGE_CH_B_NEG   PB1  /* Channel B negative FET gate */
#define HBRIDGE_FETS_MASK  ((1<<PB0)|(1<<PB1)|(1<<PB2)|(1<<PB3))
#define DEAD_TIME_TICKS    4    /* 4 us dead time between H-bridge polarity transitions */

/* DAC power level base values (higher DAC value = lower output intensity) */
#define PWR_LEVEL_LOW_BASE       650
#define PWR_LEVEL_NORMAL_BASE    590
#define PWR_LEVEL_HIGH_BASE      500
#define PWR_LEVEL_LOW_MOD        220
#define PWR_LEVEL_NORMAL_MOD     330
#define PWR_LEVEL_HIGH_MOD       440

/* FET calibration parameters */
#define FET_CAL_DAC_VALUE      128   /* DAC value for test pulses (~mid-range) */
#define FET_CAL_CURRENT_MIN    5     /* Minimum current delta (FET not open) */
#define FET_CAL_CURRENT_MAX    800   /* Maximum current (FET not shorted) */
#define FET_CAL_SETTLE_MS      2     /* Wait time after DAC change (no FETs) */
#define FET_CAL_PULSE_US       150   /* Gate-on time per test pulse (us) */
#define FET_CAL_NUM_SAMPLES    3     /* ADC averaging count */

/* Battery ADC thresholds (through voltage divider on PA3) */
#define BATTERY_ADC_EMPTY      584   /* ~9.4V = 0% */
#define BATTERY_ADC_FULL       676   /* ~10.9V = 100% */
#define BATTERY_ADC_RANGE      (BATTERY_ADC_FULL - BATTERY_ADC_EMPTY)

/* Device identity (reported via serial protocol flash region) */
#define BOX_MODEL_MK312BT  0x0C
#define FIRMWARE_VER_MAJ   0x01
#define FIRMWARE_VER_MIN   0x06
#define FIRMWARE_VER_INT   0x00

/* HD44780 LCD protocol constants */
#define LCD_RETURN_HOME    0x02  /* Return cursor to home position */
#define LCD_SET_DDRAM      0x80  /* Set DDRAM address command base */
#define LCD_SET_CGRAM      0x40  /* Set CGRAM address command base */
#define LCD_ROW1_ADDR      0x40  /* DDRAM start address for row 1 */
#define LCD_INIT_8BIT      0x30  /* 8-bit mode init command */
#define LCD_INIT_4BIT      0x20  /* Switch to 4-bit mode command */

/* PORTD initial state: PD7-PD2 high, PD1-PD0 low */
#define PORTD_INIT_STATE   ((1<<PORTD_BIT_BACKLIGHT)|(1<<PORTD_BIT_LED_A)|(1<<PORTD_BIT_LED_B)|(1<<PORTD_BIT_DAC_CS)|(1<<3)|(1<<2))

#endif
