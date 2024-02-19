#ifndef _HPET_H
#define _HPET_H

#include "dev/acpi/acpi_tables.h"

#define HPET_COUNTER_CLOCK_OFFSET 0x20

#define HPET_GENERAL_CAPABILITIES 0x000
#define HPET_GENERAL_CONFIG 0x010
#define HPET_MAIN_COUNTER_VALUE 0x0F0

#define HPET_CONFIG_ENABLE 0b1
#define HPET_CONFIG_DISABLE 0b0
#define HPET_CONFIG_LEGACY_MODE 0b10

int init_hpet(acpi_hpet_t* hpet);
uint64_t hpet_get_counter();
void hpet_reset_counter();

void hpet_nanosleep(uint64_t nanoseconds);
void hpet_sleep(uint64_t milliseconds);

#endif