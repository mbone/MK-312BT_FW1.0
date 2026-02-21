/*
 * prng.c - Pseudo-Random Number Generator
 *
 * 16-bit linear congruential generator (LCG) using the classic
 * constants from the ANSI C standard (truncated to 16 bits):
 *   state = (state * 1103515245 + 12345) & 0xFFFF
 *
 * Used by Random1/Random2 modes and bytecode programs for
 * random parameter variation.
 *
 * prng_next16() returns the full 16-bit state.
 * prng_next() returns the high byte only (better distribution
 * for the LCG - low bits have short periods).
 */

#include "prng.h"

static uint16_t prng_state = 0xACE1;  /* Non-zero default seed */

/* Seed the generator. Zero is replaced with default to avoid stuck state. */
void prng_init(uint16_t seed) {
    if (seed == 0) {
        seed = 0xACE1;
    }
    prng_state = seed;
}

/* Generate next 16-bit pseudo-random value */
uint16_t prng_next16(void) {
    prng_state = (prng_state * 1103515245UL + 12345UL) & 0xFFFF;
    return prng_state;
}

/* Generate next 8-bit pseudo-random value (high byte for better quality) */
uint8_t prng_next(void) {
    uint16_t val = prng_next16();
    return (uint8_t)(val >> 8);
}
