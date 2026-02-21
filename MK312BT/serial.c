/*
 * serial.c - MK-312BT Serial Communication Protocol
 *
 * Implements the MK-312BT's host serial protocol at 19200 baud (8N1).
 * The protocol supports three commands:
 *   - KEY_EXCHANGE (0x2F): Establishes XOR encryption key between host and box
 *   - READ (0x3C): Read a byte from the device's memory address space
 *   - WRITE (0x0D): Write bytes to the device's memory address space
 *
 * Encryption: After key exchange, all received bytes (including the command
 * byte) are XOR-decrypted with: box_key ^ host_key ^ 0x55
 *
 * Address translation is handled by serial_mem.c, which maps the three
 * virtual address regions (Flash ROM, RAM, EEPROM) to actual firmware state.
 *
 * Each packet ends with a checksum (low byte of sum of preceding bytes).
 */

#include "serial.h"
#include "serial_mem.h"
#include <stdint.h>
#include <stdbool.h>
#include "avr_registers.h"
#include "MK312BT_Memory.h"
#include "MK312BT_Constants.h"
#include "prng.h"

extern unsigned long millis(void);

static uint8_t encryption_key = 0x00;
static bool encryption_enabled = false;

#define RX_RING_SIZE 64
static volatile uint8_t rx_ring[RX_RING_SIZE];
static volatile uint8_t rx_ring_head = 0;
static volatile uint8_t rx_ring_tail = 0;

static uint8_t rx_packet[16];
static uint8_t rx_index = 0;
static uint8_t expected_bytes = 0;
static unsigned long rx_last_byte_ms = 0;


void serial_init(void) {
    encryption_key = 0x00;
    encryption_enabled = false;
    rx_ring_head = 0;
    rx_ring_tail = 0;
    rx_index = 0;
    expected_bytes = 0;
    rx_last_byte_ms = 0;
}

void serial_enable_rx_interrupt(void) {
    UCSRB |= (1 << RXCIE);
    sei();
}

uint8_t serial_calculate_checksum(uint8_t *data, uint8_t length) {
    if (length == 0) return 0;
    uint16_t sum = 0;
    for (uint8_t i = 0; i < length - 1; i++) {
        sum += data[i];
    }
    return (uint8_t)(sum & 0xFF);
}

void serial_send_byte(uint8_t data) {
    uint16_t timeout = 0;
    while (!(UCSRA & (1 << UDRE))) {
        if (++timeout > 60000) return;
    }
    UDR = data;
}

void serial_set_encryption_key(uint8_t box_key, uint8_t host_key) {
    encryption_key = box_key ^ host_key ^ SERIAL_EXTRA_ENCRYPT_KEY;
    encryption_enabled = true;
}

void serial_reset_encryption(void) {
    encryption_key = 0x00;
    encryption_enabled = false;
}

ISR(USART_RXC_vect) {
    uint8_t data = UDR;
    uint8_t next_head = (rx_ring_head + 1) % RX_RING_SIZE;
    if (next_head != rx_ring_tail) {
        rx_ring[rx_ring_head] = data;
        rx_ring_head = next_head;
    }
}

void serial_handle_key_exchange(uint8_t host_key) {
    uint8_t box_key = prng_next();

    uint8_t response[3];
    response[0] = SERIAL_REPLY_KEY_EXCHANGE;
    response[1] = box_key;
    response[2] = serial_calculate_checksum(response, 3);

    for (uint8_t i = 0; i < 3; i++) {
        serial_send_byte(response[i]);
    }

    serial_set_encryption_key(box_key, host_key);
}

void serial_handle_read_command(uint16_t address) {
    uint8_t value = serial_mem_read(address);

    uint8_t response[3];
    response[0] = SERIAL_REPLY_READ;
    response[1] = value;
    response[2] = serial_calculate_checksum(response, 3);

    for (uint8_t i = 0; i < 3; i++) {
        serial_send_byte(response[i]);
    }
}

void serial_handle_write_command(uint16_t address, uint8_t length) {
    if ((uint16_t)(3 + length) > sizeof(rx_packet)) return;
    for (uint8_t i = 0; i < length; i++) {
        serial_mem_write(address + i, rx_packet[3 + i]);
    }

    serial_send_byte(SERIAL_REPLY_OK);
}

/* Main serial polling function - call from main loop.
 *
 * WRITE command format: [0xND] [addr_hi] [addr_lo] [data...] [checksum]
 *   where N = total packet length, D = 0x0D (write opcode)
 * READ command format:  [0x3C] [addr_hi] [addr_lo] [checksum]  (4 bytes)
 * KEY_EXCHANGE format:  [0x2F] [host_key] [checksum]           (3 bytes)
 * RESET format:         [0x08]                                  (1 byte) */
void serial_process(void) {
    if (rx_index > 0 && (millis() - rx_last_byte_ms) > SERIAL_PACKET_TIMEOUT_MS) {
        rx_index = 0;
        expected_bytes = 0;
    }

    uint8_t bytes_processed = 0;
    while (rx_ring_head != rx_ring_tail && bytes_processed < SERIAL_MAX_BYTES_PER_POLL) {
        bytes_processed++;
        rx_last_byte_ms = millis();
        uint8_t raw_byte = rx_ring[rx_ring_tail];
        rx_ring_tail = (rx_ring_tail + 1) % RX_RING_SIZE;

        if (raw_byte == SERIAL_CMD_SYNC && rx_index == 0) {
            serial_reset_encryption();
            serial_send_byte(SERIAL_REPLY_SYNC);
            rx_index = 0;
            expected_bytes = 0;
            continue;
        }

        if (rx_index >= sizeof(rx_packet)) {
            rx_index = 0;
            expected_bytes = 0;
            serial_send_byte(SERIAL_REPLY_ERROR);
            continue;
        }

        uint8_t received = raw_byte;
        if (encryption_enabled) {
            received ^= encryption_key;
        }

        rx_packet[rx_index++] = received;

        if (rx_index == 1) {
            uint8_t cmd = rx_packet[0];

            if ((cmd & 0x0F) == SERIAL_CMD_WRITE) {
                uint8_t len_nibble = (cmd >> 4);
                if (len_nibble < 4 || len_nibble > 15) {
                    rx_index = 0;
                    expected_bytes = 0;
                    serial_send_byte(SERIAL_REPLY_ERROR);
                    continue;
                }
                expected_bytes = len_nibble + 1;
            }
            else if (cmd == SERIAL_CMD_READ) {
                expected_bytes = 4;
            }
            else if (cmd == SERIAL_CMD_KEY_EXCHANGE) {
                expected_bytes = 3;
            }
            else if (cmd == SERIAL_CMD_RESET) {
                serial_reset_encryption();
                serial_send_byte(SERIAL_REPLY_OK);
                rx_index = 0;
                expected_bytes = 0;
                continue;
            }
            else {
                rx_index = 0;
                expected_bytes = 0;
                continue;
            }
        }

        if (rx_index >= expected_bytes && expected_bytes > 0) {
            uint8_t checksum = serial_calculate_checksum(rx_packet, expected_bytes);

            if (checksum != rx_packet[expected_bytes - 1]) {
                serial_send_byte(SERIAL_REPLY_ERROR);
                rx_index = 0;
                expected_bytes = 0;
                continue;
            }

            uint8_t cmd = rx_packet[0];

            if (cmd == SERIAL_CMD_KEY_EXCHANGE) {
                uint8_t host_key = rx_packet[1];
                serial_handle_key_exchange(host_key);
            }
            else if (cmd == SERIAL_CMD_READ) {
                uint16_t address = ((uint16_t)rx_packet[1] << 8) | rx_packet[2];
                serial_handle_read_command(address);
            }
            else if ((cmd & 0x0F) == SERIAL_CMD_WRITE) {
                uint16_t address = ((uint16_t)rx_packet[1] << 8) | rx_packet[2];
                uint8_t len_nibble = (cmd >> 4);
                uint8_t data_length = (len_nibble >= 4) ? (len_nibble - 3) : 0;
                if (data_length > 0) {
                    serial_handle_write_command(address, data_length);
                }
            }

            rx_index = 0;
            expected_bytes = 0;
        }
    }
}
