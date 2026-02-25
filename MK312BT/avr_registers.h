/*
 * avr_registers.h - ATmega16 Hardware Register Definitions
 *
 * Memory-mapped I/O register addresses and bit positions for the ATmega16
 * microcontroller. Used as a compatibility shim for non-AVR-GCC compilation
 * environments (e.g., Arduino IDE, simulation).
 *
 * Register groups:
 *   GPIO Ports   - PORTB (H-bridge FETs, SPI), PORTC (LCD/buttons), PORTD (LEDs, DAC CS, USART)
 *   USART        - UDR, UCSRA/B/C, UBRRL/H for 19200 baud serial link
 *   Timer1       - 16-bit CTC mode for Channel A pulse timing (PB2/PB3)
 *   Timer2       - 8-bit CTC mode for Channel B pulse timing (PB0/PB1)
 *   TIMSK        - Timer interrupt mask (OCIE1A, OCIE2)
 *   ADC          - ADMUX channel select, ADCSRA control, ADCL/ADCH result
 *   SPI          - SPCR/SPSR/SPDR for LTC1661 DAC communication
 *   EEPROM       - EEARL/H address, EEDR data, EECR control
 *   Interrupts   - MCUCR, GICR for external interrupt config
 *
 * Pin assignments are documented in MK312BT_Constants.h.
 */

#ifndef AVR_REGISTERS_H
#define AVR_REGISTERS_H

#include <stdint.h>

/* GPIO Port B: H-bridge FET gates (PB0-PB3), SPI bus (PB5-PB7) */
#define PORTB (*(volatile uint8_t*)0x38)
#define DDRB  (*(volatile uint8_t*)0x37)

/* GPIO Port C: LCD data bus (PC4-PC7), LCD control (PC1-PC3), button activate (PC0) */
#define PORTC (*(volatile uint8_t*)0x35)
#define DDRC  (*(volatile uint8_t*)0x34)

/* GPIO Port D: LEDs (PD5-PD6), DAC chip select (PD4), USART (PD0-PD1), LCD backlight (PD7) */
#define PORTD (*(volatile uint8_t*)0x32)
#define DDRD  (*(volatile uint8_t*)0x31)

/* USART registers - 19200 baud serial link via MAX232 level shifter */
#define UDR    (*(volatile uint8_t*)0x2C)  /* Data register (TX/RX) */
#define UCSRA  (*(volatile uint8_t*)0x2B)  /* Status: RXC (bit 7), UDRE (bit 5) */
#define UCSRB  (*(volatile uint8_t*)0x2A)  /* Control: enable TX/RX, interrupts */
#define UCSRC  (*(volatile uint8_t*)0x40)  /* Frame format: 8N1 */
#define UBRRL  (*(volatile uint8_t*)0x29)  /* Baud rate low byte */
#define UBRRH  (*(volatile uint8_t*)0x40)  /* Baud rate high byte (shares addr with UCSRC) */

#define RXC   7   /* UCSRA: Receive Complete flag */
#define UDRE  5   /* UCSRA: Data Register Empty (ready to transmit) */

/* UCSRB bit positions */
#define RXCIE 7   /* RX Complete Interrupt Enable */
#define RXEN  4   /* Receiver Enable */
#define UDRIE 5   /* USART Data Register Empty Interrupt Enable */
#define TXEN  3   /* Transmitter Enable */

/* UCSRC bit positions */
#define URSEL 7   /* Register Select (must be 1 to write UCSRC) */
#define UCSZ1 2   /* Character Size bit 1 */
#define UCSZ0 1   /* Character Size bit 0 */

/* Timer1 - 16-bit, CTC mode, /8 prescaler: Channel A biphasic pulse generation */
#define TCNT1L (*(volatile uint8_t*)0x4C)  /* Counter low byte */
#define TCNT1H (*(volatile uint8_t*)0x4D)  /* Counter high byte */
#define OCR1AL (*(volatile uint8_t*)0x4A)  /* Output Compare A low byte (pulse timing) */
#define OCR1AH (*(volatile uint8_t*)0x4B)  /* Output Compare A high byte */
#define TCCR1A (*(volatile uint8_t*)0x4F)  /* Control A (WGM bits) */
#define TCCR1B (*(volatile uint8_t*)0x4E)  /* Control B (WGM12, CS11 for CTC /8) */

/* Timer2 - 8-bit, CTC mode, /8 prescaler: Channel B biphasic pulse generation */
#define TCNT2  (*(volatile uint8_t*)0x44)  /* Counter value */
#define TCCR2  (*(volatile uint8_t*)0x45)  /* Control (WGM21, CS21 for CTC /8) */
#define OCR2   (*(volatile uint8_t*)0x43)  /* Output Compare (pulse timing, 8-bit limit) */

/* Timer Interrupt Mask - enables compare match interrupts for pulse ISRs */
#define TIMSK  (*(volatile uint8_t*)0x59)

/* ADC - 10-bit successive approximation, 6 channels on Port A */
#define ADMUX  (*(volatile uint8_t*)0x27)  /* Channel select + voltage reference */
#define ADCSRA (*(volatile uint8_t*)0x26)  /* Control: ADSC start, prescaler, enable */
#define ADCL   (*(volatile uint8_t*)0x24)  /* Result low byte (must read first) */
#define ADCH   (*(volatile uint8_t*)0x25)  /* Result high byte */

/* SPI - Master mode for LTC1661 DAC communication */
#define SPDR   (*(volatile uint8_t*)0x2F)  /* Data register (shift in/out) */
#define SPSR   (*(volatile uint8_t*)0x2E)  /* Status: SPIF transfer complete flag */
#define SPCR   (*(volatile uint8_t*)0x2D)  /* Control: SPE enable, MSTR master, clock rate */

/* EEPROM - persistent storage for user config and calibration */
#define EEARL  (*(volatile uint8_t*)0x3E)  /* Address low byte */
#define EEARH  (*(volatile uint8_t*)0x3F)  /* Address high byte */
#define EEDR   (*(volatile uint8_t*)0x3D)  /* Data register */
#define EECR   (*(volatile uint8_t*)0x3C)  /* Control: EERE read, EEWE write, EEMWE master write */

/* EECR bit positions */
#define EERE   0   /* Read enable - triggers read from EEPROM */
#define EEWE   1   /* Write enable - triggers write to EEPROM */
#define EEMWE  2   /* Master write enable - must set before EEWE */

/* Watchdog Timer */
#define WDTCR  (*(volatile uint8_t*)0x41)  /* Watchdog Timer Control Register */
#define WDTOE  4   /* Watchdog Turn-off Enable */
#define WDE    3   /* Watchdog Enable */

/* Timer0 - 8-bit (used for entropy seeding) */
#define TCNT0  (*(volatile uint8_t*)0x52)  /* Timer0 counter value */

/* External interrupt control */
#define MCUCR  (*(volatile uint8_t*)0x55)  /* MCU control (INT0/INT1 sense control) */
#define GICR   (*(volatile uint8_t*)0x5B)  /* General interrupt control (INT0/INT1 enable) */

/* Interrupt sense control bits (MCUCR) */
#define ISC01  1   /* INT0 sense control bit 1 */
#define ISC11  3   /* INT1 sense control bit 1 */

/* Timer interrupt enable bits (TIMSK register) */
#define OCIE1A 4   /* Timer1 Compare Match A interrupt enable */
#define OCIE2  7   /* Timer2 Compare Match interrupt enable */

/* Timer1 control bits */
#define WGM12  3   /* TCCR1B: CTC mode (clear timer on compare match) */
#define CS11   1   /* TCCR1B: /8 prescaler select */

/* Timer2 control bits */
#define WGM21  3   /* TCCR2: CTC mode */
#define CS21   1   /* TCCR2: /8 prescaler select */

/* SPI control/status bits */
#define SPIF   7   /* SPSR: Transfer complete flag */
#define SPE    6   /* SPCR: SPI enable */
#define MSTR   4   /* SPCR: Master mode select */
#define SPR0   0   /* SPCR: Clock rate select bit 0 */

/* ADC control bits */
#define ADEN   7   /* ADCSRA: ADC Enable */
#define ADSC   6   /* ADCSRA: Start Conversion */
#define ADPS2  2   /* ADCSRA: Prescaler bit 2 */
#define ADPS1  1   /* ADCSRA: Prescaler bit 1 */
#define ADPS0  0   /* ADCSRA: Prescaler bit 0 */

/* Status register (save/restore for atomic sections) */
#define SREG   (*(volatile uint8_t*)0x5F)

/* Global interrupt control */
#define sei() __asm__ __volatile__ ("sei" ::: "memory")
#define cli() __asm__ __volatile__ ("cli" ::: "memory")

#endif
