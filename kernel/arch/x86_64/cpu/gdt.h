#ifndef GDT_H
#define GDT_H

#include "types.h"

#define GDT_BASE_KERNEL_CODE_SEG    0x8
#define GDT_BASE_KERNEL_DATA_SEG    0x10
#define GDT_BASE_USER_DATA_SEG      0x18
#define GDT_BASE_USER_CODE_SEG      0x20

#define GDT_LIMIT               0
#define GDT_FLAGS               0b0010

#define GDT_ACCESS_EXEC_READ    0b10011010
#define GDT_ACCESS_READ_WRITE   0b10010010
#define GDT_TYPE_TSS            0b10001001

#define GDT_ACCESS_PRESENT      0b10000000  // Существует
#define GDT_ACCESS_PRIVILEGE_3  0b01100000  // уровень пользователя
#define GDT_ACCESS_NON_SYSTEM   0b00010000
#define GDT_ACCESS_EXECUTABLE   0b00001000  // Можно выполнять
#define GDT_ACCESS_RW           0b00000010  // Для DATA - возможность записи, для CODE - чтения
#define GDT_ACCESS_CODE_CONF    0b00000100

#define GDT_BASE_KERNEL_CODE_ACCESS  GDT_ACCESS_PRESENT | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_NON_SYSTEM | GDT_ACCESS_RW
#define GDT_BASE_KERNEL_DATA_ACCESS  GDT_ACCESS_PRESENT | GDT_ACCESS_NON_SYSTEM | GDT_ACCESS_RW
#define GDT_BASE_USER_CODE_ACCESS    GDT_ACCESS_PRESENT | GDT_ACCESS_PRIVILEGE_3 | GDT_ACCESS_NON_SYSTEM | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_RW
#define GDT_BASE_USER_DATA_ACCESS    GDT_ACCESS_PRESENT | GDT_ACCESS_PRIVILEGE_3 | GDT_ACCESS_NON_SYSTEM | GDT_ACCESS_RW

typedef struct PACKED {
    uint16_t    limit_0_15;
    uint16_t    base_0_15;
    uint8_t     base_16_23;
    uint8_t     access;
    uint8_t     limit16_19 : 4;
    uint8_t     flags : 4;
    uint8_t     base_24_31;
} gdt_entry_t;

typedef struct PACKED {
    uint16_t    limit_0_15;
    uint16_t    base_0_15;
    uint8_t     base_16_23;
    uint8_t     access;
    uint8_t     limit16_19 : 4;
    uint8_t     flags : 4;
    uint8_t     base_24_31;

    uint32_t    base_32_63;
    uint32_t    reserved;

} system_seg_desc_t;

typedef struct PACKED {
	uint16_t	limit;
	uint64_t	base;
} gdtr_t;

typedef struct PACKED {
    uint32_t    reserved;
    uintptr_t   rsp0;
    uintptr_t   rsp1;
    uintptr_t   rsp2;

    uint32_t    reserved2;
    uint32_t    reserved3;

    uintptr_t   ist1;
    uintptr_t   ist2;
    uintptr_t   ist3;
    uintptr_t   ist4;
    uintptr_t   ist5;
    uintptr_t   ist6;
    uintptr_t   ist7;

    uint32_t    reserved4;
    uint32_t    reserved5;

    uint16_t    reserved6;

    uint16_t    iopb;
} tss_t;

void new_gdt(   uint32_t entries_num, 
                uint32_t sys_seg_descs_num,
                gdt_entry_t** entry_begin,
                system_seg_desc_t** sys_seg_begin,
                uint32_t* size);

tss_t* new_tss();

size_t gdt_get_required_size(uint32_t entries_num, uint32_t sys_seg_descs_num);

void gdt_set(gdt_entry_t* entry_begin, uint32_t limit, uint32_t base, uint8_t access, uint8_t flags);

void gdt_set_sys_seg(system_seg_desc_t* sys_seg_begin, uint32_t limit, uintptr_t base, uint8_t access, uint8_t flags);

void gdt_init();

void gdt_create(gdt_entry_t** gdt, size_t* size, tss_t** tss);

void tss_set_rsp0(uintptr_t address);

extern void gdt_update(void*);
extern void x64_ltr(int);

#endif