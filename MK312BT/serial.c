#include "serial.h"
#include "serial_mem.h"
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>
#include "MK312BT_Constants.h"
#include "prng.h"

extern unsigned long millis(void);

/* =========================
   Configuration
   ========================= */

#define RX_RING_SIZE 64
#define TX_RING_SIZE 64

/* =========================
   Ring Buffers
   ========================= */

static volatile uint8_t rx_ring[RX_RING_SIZE];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;

static volatile uint8_t tx_ring[TX_RING_SIZE];
static volatile uint8_t tx_head = 0;
static volatile uint8_t tx_tail = 0;

/* =========================
   Protocol State
   ========================= */

static uint8_t encryption_key = 0x00;
static bool encryption_enabled = false;

static uint8_t rx_buffer[16];
static uint8_t rx_index = 0;
static uint8_t expected_bytes = 0;
static unsigned long rx_last_byte_ms = 0;

/* =========================
   Interrupt Handlers
   ========================= */

ISR(USART_RXC_vect)
{
    uint8_t next = (rx_head + 1) % RX_RING_SIZE;
    uint8_t data = UDR;

    if (next != rx_tail) {
        rx_ring[rx_head] = data;
        rx_head = next;
    }
}

ISR(USART_UDRE_vect)
{
    if (tx_head == tx_tail) {
        UCSRB &= ~(1 << UDRIE);  // disable interrupt
        return;
    }

    UDR = tx_ring[tx_tail];
    tx_tail = (tx_tail + 1) % TX_RING_SIZE;
}

/* =========================
   TX Functions (Non-blocking)
   ========================= */

static void tx_enqueue(uint8_t data)
{
    uint8_t next = (tx_head + 1) % TX_RING_SIZE;

    while (next == tx_tail) {
        // buffer full — wait (rare, but safe)
    }

    tx_ring[tx_head] = data;
    tx_head = next;

    UCSRB |= (1 << UDRIE);  // enable TX interrupt
}

void serial_send_byte(uint8_t data)
{
    tx_enqueue(data);
}

static void serial_send_buffer(uint8_t *data, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        tx_enqueue(data[i]);
    }
}

/* =========================
   Utility
   ========================= */

uint8_t serial_calculate_checksum(uint8_t *data, uint8_t length)
{
    uint16_t sum = 0;
    for (uint8_t i = 0; i < length - 1; i++) {
        sum += data[i];
    }
    return (uint8_t)(sum & 0xFF);
}

void serial_set_encryption_key(uint8_t box_key, uint8_t host_key)
{
    encryption_key = box_key ^ host_key ^ SERIAL_EXTRA_ENCRYPT_KEY;
    encryption_enabled = true;
}

void serial_reset_encryption(void)
{
    encryption_key = 0;
    encryption_enabled = false;
}

/* =========================
   Command Handlers
   ========================= */

void serial_handle_key_exchange(uint8_t host_key)
{
    uint8_t box_key = prng_next();

    uint8_t response[3];
    response[0] = SERIAL_REPLY_KEY_EXCHANGE;
    response[1] = box_key;
    response[2] = serial_calculate_checksum(response, 3);

    serial_send_buffer(response, 3);
    serial_set_encryption_key(box_key, host_key);
}

static void serial_handle_read(uint16_t address)
{
    uint8_t value = serial_mem_read(address);

    uint8_t response[3];
    response[0] = SERIAL_REPLY_READ;
    response[1] = value;
    response[2] = serial_calculate_checksum(response, 3);

    serial_send_buffer(response, 3);
}

static void serial_handle_write(uint16_t address, uint8_t length)
{
    for (uint8_t i = 0; i < length; i++) {
        serial_mem_write(address + i, rx_buffer[3 + i]);
    }

    serial_send_byte(SERIAL_REPLY_OK);
}

/* =========================
   Public API
   ========================= */

void serial_init(void)
{
    encryption_key = 0;
    encryption_enabled = false;
    rx_index = 0;
    expected_bytes = 0;
    rx_last_byte_ms = 0;
}

void serial_process(void)
{
    while (rx_head != rx_tail) {

        uint8_t raw_byte = rx_ring[rx_tail];
        rx_tail = (rx_tail + 1) % RX_RING_SIZE;

        rx_last_byte_ms = millis();

        if (raw_byte == SERIAL_CMD_SYNC && rx_index == 0) {
            serial_reset_encryption();
            serial_send_byte(SERIAL_REPLY_SYNC);
            continue;
        }

        uint8_t received = raw_byte;
        if (encryption_enabled) {
            received ^= encryption_key;
        }

        if (rx_index >= sizeof(rx_buffer)) {
            rx_index = 0;
            expected_bytes = 0;
            serial_send_byte(SERIAL_REPLY_ERROR);
            continue;
        }

        rx_buffer[rx_index++] = received;

        if (rx_index == 1) {
            uint8_t cmd = rx_buffer[0];

            if ((cmd & 0x0F) == SERIAL_CMD_WRITE) {
                uint8_t len = (cmd >> 4);
                expected_bytes = len + 1;
            }
            else if (cmd == SERIAL_CMD_READ) {
                expected_bytes = 4;
            }
            else if (cmd == SERIAL_CMD_KEY_EXCHANGE) {
                expected_bytes = 3;
            }
            else {
                rx_index = 0;
                expected_bytes = 0;
                continue;
            }
        }

        if (expected_bytes > 0 && rx_index >= expected_bytes) {

            uint8_t checksum = serial_calculate_checksum(rx_buffer, expected_bytes);

            if (checksum != rx_buffer[expected_bytes - 1]) {
                serial_send_byte(SERIAL_REPLY_ERROR);
                rx_index = 0;
                expected_bytes = 0;
                continue;
            }

            uint8_t cmd = rx_buffer[0];

            if (cmd == SERIAL_CMD_KEY_EXCHANGE) {
                serial_handle_key_exchange(rx_buffer[1]);
            }
            else if (cmd == SERIAL_CMD_READ) {
                uint16_t addr = ((uint16_t)rx_buffer[1] << 8) | rx_buffer[2];
                serial_handle_read(addr);
            }
            else if ((cmd & 0x0F) == SERIAL_CMD_WRITE) {
                uint16_t addr = ((uint16_t)rx_buffer[1] << 8) | rx_buffer[2];
                uint8_t data_len = (cmd >> 4) - 3;
                serial_handle_write(addr, data_len);
            }

            rx_index = 0;
            expected_bytes = 0;
        }
    }
}
