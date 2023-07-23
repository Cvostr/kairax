#include "cpuid.h"
#include "types.h"
#include "string.h"

#define cpuid(in,a,b,c,d) do { asm volatile ("cpuid" : "=a"(a),"=b"(b),"=c"(c),"=d"(d) : "a"(in)); } while(0)

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
	
        return 1;
    }

    return 0; 
}

int cpuid_get_apic_id()
{
    uint32_t id, unused;
    cpuid(1, unused, id, unused, unused);
    return (int)(id >> 24);
}