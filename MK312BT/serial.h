/*
 * serial.h - ET-312 Serial Communication Protocol
 *
 * Host communication interface at 19200 baud with XOR encryption.
 * Supports READ/WRITE memory access and key exchange commands.
 * See SERIAL_PROTOCOL.md for full protocol documentation.
 */

#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Command opcodes (sent by host) */
#define SERIAL_CMD_SYNC         0x00  /* Sync/NOP - ignored */
#define SERIAL_CMD_RESET        0x08  /* Reset protocol state */
#define SERIAL_CMD_READ         0x3C  /* Read one byte from address */
#define SERIAL_CMD_WRITE        0x0D  /* Write bytes to address (low nibble) */
#define SERIAL_CMD_KEY_EXCHANGE 0x2F  /* Initiate encryption key exchange */

/* Reply opcodes (sent by device) */
#define SERIAL_REPLY_SYNC         0x07  /* Handshake sync acknowledgment */
#define SERIAL_REPLY_KEY_EXCHANGE 0x21  /* Key exchange response with box_key */
#define SERIAL_REPLY_READ         0x22  /* Read response with data byte */
#define SERIAL_REPLY_OK           0x06  /* Command acknowledged successfully */
#define SERIAL_REPLY_ERROR        0x07  /* Checksum mismatch or error */

/* Protocol constants */
#define SERIAL_EXTRA_ENCRYPT_KEY    0x55  /* XOR key mixed into encryption derivation */
#define SERIAL_PACKET_TIMEOUT_MS    500   /* Timeout for incomplete packets (ms) */
#define SERIAL_MAX_BYTES_PER_POLL   32    /* Max bytes to process per serial_process() call */
#define SERIAL_MODE_PROTOCOL_BASE   0x76  /* Offset mapping internal mode index to protocol mode number */

void serial_init(void);                                         /* Initialize serial state */
void serial_process(void);                                      /* Poll USART and process packets */
void serial_send_byte(uint8_t data);                           /* Transmit one byte (blocking) */
uint8_t serial_calculate_checksum(uint8_t *data, uint8_t length); /* Compute packet checksum */
void serial_handle_read_command(uint16_t address);             /* Process READ request */
void serial_handle_write_command(uint16_t address, uint8_t length); /* Process WRITE request */
void serial_handle_key_exchange(uint8_t host_key);             /* Process KEY_EXCHANGE request */
void serial_set_encryption_key(uint8_t box_key, uint8_t host_key); /* Derive shared XOR key */


#ifdef __cplusplus
}
#endif

#endif
