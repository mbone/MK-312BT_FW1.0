/*
 * prng.h - Pseudo-Random Number Generator
 *
 * 16-bit LCG for random parameter variation in modes.
 * Returns 8-bit or 16-bit pseudo-random values.
 */

#ifndef PRNG_H
#define PRNG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void prng_init(uint16_t seed);   /* Seed the generator (0 uses default 0xACE1) */
uint8_t prng_next(void);        /* Next 8-bit random value (high byte of state) */
uint16_t prng_next16(void);     /* Next 16-bit random value */

#ifdef __cplusplus
}
#endif

#endif
