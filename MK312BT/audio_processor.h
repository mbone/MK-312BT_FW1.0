/*
 * audio_processor.h - Audio Input Envelope Follower
 *
 * Rectifies and scales audio line-in signals to modulate
 * channel intensity for Audio1/Audio2/Audio3 modes.
 */

#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void audio_init(void);               /* Initialize gain to default (128 = unity) */
void audio_process_channel_a(void);  /* Process right line-in -> Ch A intensity */
void audio_process_channel_b(void);  /* Process left line-in/mic -> Ch B intensity */

#ifdef __cplusplus
}
#endif

#endif
