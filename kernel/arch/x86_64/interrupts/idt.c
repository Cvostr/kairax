#include "idt.h"
#include "memory/mem_layout.h"
#include "mem/pmm.h"
#include "mem/kheap.h"
#include "stdio.h"

#define GDT_OFFSET_KERNEL_CODE 0x8
#define IDT_MAX_DESCRIPTORS 256

extern idt_descriptor_t idt_descriptors[255];

extern void* isr_stub_table[256]; //таблица ISR

extern void idt_update(idtr_t *idt);

void set_int_descriptor(uint8_t vector, void* isr, uint8_t ist, uint8_t flags){
	idt_descriptor_t* descriptor = &idt_descriptors[vector];
 
    descriptor->isr_0_16       = (uint64_t)isr & 0xFFFF;
    descriptor->cs             = GDT_OFFSET_KERNEL_CODE;
    descriptor->ist            = ist;
    descriptor->attributes     = flags;
    descriptor->isr_16_32      = ((uint64_t)isr >> 16) & 0xFFFF;
    descriptor->isr_32_63      = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
    descriptor->reserved       = 0;
}

void setup_idt(){
    idtr_t idtr;
	idtr.base = (uint64_t)(idt_descriptors); //адрес таблицы дескрипторов
    idtr.limit = (uint16_t)sizeof(idt_descriptor_t) * IDT_MAX_DESCRIPTORS - 1;

	for (uint8_t vector = 0; vector < 255; vector++) {
      	set_int_descriptor(vector, isr_stub_table[vector], 0, 0x8E);
    }

	idt_update(&idtr);

	asm volatile ("sti"); // включение прерываний
}