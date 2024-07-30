#ifndef _CPUID_H
#define _CPUID_H

#define cpuid(in,a,b,c,d) do { asm volatile ("cpuid" : "=a"(a),"=b"(b),"=c"(c),"=d"(d) : "a"(in)); } while(0)

int cpuid_get_model_string(char* model);

int cpuid_get_apic_id();

#endif