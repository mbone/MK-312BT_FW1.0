#ifndef PARAM_ENGINE_H
#define PARAM_ENGINE_H

#include <stdint.h>
#include "channel_mem.h"

#ifdef __cplusplus
extern "C" {
#endif

void param_engine_init(void);
void param_engine_init_directions(void);
void param_engine_tick(void);

uint8_t param_engine_check_module_trigger(ChannelBlock *ch);
uint8_t param_engine_get_tick(void);
uint16_t param_engine_get_master_timer(void);

#ifdef __cplusplus
}
#endif

#endif
