#ifndef _PIT_H
#define _PIT_H

#include "types.h"

uint32_t pit_get_count();

void pit_set_reload_value(uint32_t count);

void pit_set_frequency(uint64_t frequency);

#endif