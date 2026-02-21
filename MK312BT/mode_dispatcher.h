/*
 * MK-312BT Mode Dispatcher
 *
 * Routes modes to the appropriate execution engine:
 *   - Built-in modes (Waves-Phase3): parameter engine with ramping/modulation
 *   - User modes (User1-7, Split): inline bytecode execution
 *
 * Call mode_dispatcher_update() once per main loop iteration.
 * Live output values are in channel_a / channel_b (ChannelBlock).
 */
#ifndef MODE_DISPATCHER_H
#define MODE_DISPATCHER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void mode_dispatcher_init(void);
void mode_dispatcher_select_mode(uint8_t mode_number);
void mode_dispatcher_update(void);
uint8_t mode_dispatcher_get_mode(void);
void mode_dispatcher_pause(void);
void mode_dispatcher_resume(void);

void mode_dispatcher_set_split_modes(uint8_t mode_a, uint8_t mode_b);
uint8_t mode_dispatcher_get_split_mode_a(void);
uint8_t mode_dispatcher_get_split_mode_b(void);

void mode_dispatcher_request_mode(uint8_t mode_number);
void mode_dispatcher_request_pause(void);
void mode_dispatcher_request_next_mode(void);
void mode_dispatcher_request_prev_mode(void);
void mode_dispatcher_request_reload(void);
void mode_dispatcher_request_start_ramp(void);
uint8_t mode_dispatcher_poll_deferred(void);

#ifdef __cplusplus
}
#endif

#endif
