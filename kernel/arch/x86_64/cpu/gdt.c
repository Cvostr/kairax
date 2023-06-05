#include "gdt.h"
#include "mem/kheap.h"
#include "string.h"

extern gdt_update(void*);

void new_gdt(   uint32_t entries_num, 
                uint32_t sys_seg_descs_num,
                gdt_entry_t** entry_begin,
                system_seg_desc_t** sys_seg_begin,
                uint32_t* size)
{
    *size = entries_num * sizeof(gdt_entry_t) + sys_seg_descs_num * sizeof(system_seg_desc_t);

    void* mem = kmalloc(*size);
    memset(mem, 0, *size);

    *entry_begin = (gdt_entry_t*)mem;
    *sys_seg_begin = (system_seg_desc_t*)(*entry_begin + entries_num);
}

tss_t* new_tss()
{
    tss_t* result = kmalloc(sizeof(tss_t));
    memset(result, 0, sizeof(tss_t));
    return result;
}

void gdt_set(gdt_entry_t* entry_begin, uint32_t limit, uint32_t base, uint8_t access, uint8_t flags)
{
    entry_begin->base_0_15 = base & 0xFFFF;
    entry_begin->base_16_23 = (base >> 16) & 0xFF;
    entry_begin->limit_0_15 = limit & 0xFFFF;
    entry_begin->limit16_19 = (limit >> 16) & 0xF;
    entry_begin->access = access;
    entry_begin->flags = flags;
}

void gdt_set_sys_seg(system_seg_desc_t* sys_seg_begin, uint32_t limit, uintptr_t base, uint8_t access, uint8_t flags)
{
    sys_seg_begin->base_0_15 = base & 0xFFFF;
    sys_seg_begin->base_16_23 = (base >> 16) & 0xFF;
    sys_seg_begin->base32_63 = (base >> 32) & 0xFFFFFFFF;
    sys_seg_begin->limit_0_15 = limit & 0xFFFF;
    sys_seg_begin->limit16_19 = (limit >> 16) & 0xF;
    sys_seg_begin->access = access;
    sys_seg_begin->flags = flags;
}

void gdt_init()
{
    uint32_t size;
    gdt_entry_t* entry_ptr;
    system_seg_desc_t* sys_seg_ptr;
    new_gdt(5, 1, &entry_ptr, &sys_seg_ptr, &size);
    // код ядра
    gdt_set(entry_ptr + 1, 0, 0, GDT_ACCESS_EXEC_READ, GDT_FLAGS);
    // данные ядра
    gdt_set(entry_ptr + 2, 0, 0, GDT_ACCESS_READ_WRITE, GDT_FLAGS);
    // код пользователя
    gdt_set(entry_ptr + 3, 0, 0, 0b11111010, GDT_FLAGS);
    // данные пользователя
    gdt_set(entry_ptr + 3, 0, 0, 0b11110010, GDT_FLAGS);

    tss_t* tss = new_tss();
    tss->ist1 = 0x1000;

    gdt_set_sys_seg(sys_seg_ptr, sizeof(tss_t), tss, 0b10001001, 0);

    gdtr_t gdtr;
    gdtr.base = entry_ptr;
    gdtr.limit = size - 1;

    gdt_update(&gdtr);
}