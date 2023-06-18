#ifndef _IDT_H
#define _IDT_H

#include "types.h"

typedef struct PACKED {
	uint16_t	isr_0_16;      // первые 16 бит адреса ISR
	uint16_t    cs;    // The GDT segment selector that the CPU will load into CS before calling the ISR
	uint8_t	ist;          // The IST in the TSS that the CPU will load into RSP; set to zero for now
	uint8_t     attributes;   // Аттрибуты
	uint16_t    isr_16_32;      // следующие 16 бит адреса ISR
	uint32_t    isr_32_63;     // последние 32 бита адреса ISR
	uint32_t    reserved;     // Зарезервировано, устанавливается в 0
} idt_descriptor_t;

typedef struct PACKED {
	uint16_t	limit;
	uint64_t	base;
} idtr_t;

void set_int_descriptor(uint8_t vector, void* isr, uint8_t ist, uint8_t flags);

void setup_idt();

#endif