/*
 * MK312BT_Utils.h - Hardware Diagnostics and Self-Test Interface
 *
 * Startup diagnostic routines run before entering the main loop.
 * Implementation in utils.c.
 */

#ifndef MK312BT_UTILS_H
#define MK312BT_UTILS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Verify LTC1661 DAC output by writing known values and reading back via ADC.
 * Tests both DAC A and DAC B with 6 values each (12 tests total).
 * Returns 1 on success, 0 if any reading is out of tolerance. */
uint8_t dacTest(void);

/* Calibrate H-bridge FET pairs by driving each half-bridge and measuring
 * output current through PA0. Checks that each FET draws measurable current
 * (not open), current is within safe limits (not shorted), and positive/negative
 * FETs are balanced (<50% imbalance). Stores baseline readings for runtime
 * current monitoring. Returns 1 on success, 0 on failure. */
uint8_t fetCalibrate(void);


#ifdef __cplusplus
}
#endif

#endif
