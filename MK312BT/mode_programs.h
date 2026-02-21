#ifndef MODE_PROGRAMS_H
#define MODE_PROGRAMS_H

#include <avr/pgmspace.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MODULE_COUNT 36

extern const uint8_t* const module_table[];

#ifdef __cplusplus
}
#endif

#endif
