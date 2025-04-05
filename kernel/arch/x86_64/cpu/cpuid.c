#include "cpuid.h"
#include "kairax/types.h"
#include "string.h"

int cpuid_get_model_string(char* model)
{
    unsigned long a, unused;
    cpuid(0x80000000, a, unused, unused, unused);

    if (a >= 0x80000004) {
		uint32_t brand[12];
		cpuid(0x80000002, brand[0], brand[1], brand[2], brand[3]);
		cpuid(0x80000003, brand[4], brand[5], brand[6], brand[7]);
		cpuid(0x80000004, brand[8], brand[9], brand[10], brand[11]);
		memcpy(model, brand, 48);
	
        return 0;
    }

    return -1; 
}

int cpuid_get_vendor_string(char* vendor)
{
    int unused;
    uint32_t vend[3];

    cpuid(0, unused, vend[0], vend[2], vend[1]);

    memcpy(vendor, vend, 12);

    return 0;
}

int cpuid_get_apic_id()
{
    uint32_t id, unused;
    cpuid(1, unused, id, unused, unused);
    return (int)(id >> 24);
}