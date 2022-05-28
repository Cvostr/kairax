#ifndef _PAGING_H
#define _PAGING_H

#include "stdint.h"
#include "hh_offset.h"

#define PAGE_PRESENT 		    0x1
#define PAGE_WRITABLE 		    0x2
#define PAGE_USER_ACCESSIBLE 	0x4
#define PAGE_HUGE 		        0x80
#define PAGE_GLOBAL 		    0x100
#define PAGE_STACK_GUARD	    0x200
#define PAGE_WRITE_THROUGH      0x8
#define PAGE_UNCACHED           0b10000
#define PAGE_EXEC_DISABLE       (1ULL << 63)

#define PAGE_ENTRY_NBR  0x200
#define INDEX_MASK      0x1FF

#define MAX_PAGES_4     (512 * 512 * 512 * 512)

//получить из 48-битного виртуального адреса индекс записи в таблице 4-го уровня (0x27 = 39)
#define GET_4_LEVEL_PAGE_INDEX(x) ((((uint64)(x)) >> 0x27) & INDEX_MASK) 
#define GET_3_LEVEL_PAGE_INDEX(x) ((((uint64)(x)) >> 0x1E) & INDEX_MASK)
#define GET_2_LEVEL_PAGE_INDEX(x) ((((uint64)(x)) >> 0x15) & INDEX_MASK) 
#define GET_1_LEVEL_PAGE_INDEX(x) ((((uint64)(x)) >> 0x0C) & INDEX_MASK) 
#define GET_PAGE_OFFSET(x) ((uint64)(x) & (4096 - 0x1)) 
#define GET_PAGE_FRAME(x)   (void*)((uint64)(x) & ~(0xFFF))

typedef uintptr_t virtual_addr_t;
typedef uintptr_t physical_addr_t;
typedef uint64_t    table_entry_t;

typedef struct PACKED {
    table_entry_t entries[512]; 
} page_table_t;

//Базовая корневая страница 4-го уровня ядра
page_table_t* get_kernel_pml4();

void set_kernel_pml4(page_table_t* pml4);

page_table_t* new_page_table();
//Создать виртуальную страницу с указанным адресом и назначить ей указанный физический
void map_page_mem(page_table_t* root, virtual_addr_t virtual_addr, physical_addr_t physical_addr, uint64_t flags);
//Создать виртуальную страницу с указанным адресом
void map_page(page_table_t* root, virtual_addr_t virtual_addr, uint64_t flags);
//Удалить виртуальную страницу
int unmap_page(page_table_t* root, uintptr_t virtual_addr);
//Получить физический адрес из виртуального для указанной корневой таблицы
physical_addr_t get_physical_address(page_table_t* root, virtual_addr_t virtual_addr);
//Проверить, создана ли запись для указанного виртуального адреса
int is_mapped(page_table_t* root, uintptr_t virtual_addr);

virtual_addr_t get_first_free_pages(page_table_t* root, uint64_t pages_count);

int copy_to_vm(page_table_t* root, virtual_addr_t dst, void* src, size_t size);
//Переключить текущую 4х уровневую таблицу страниц
void switch_pml4(page_table_t* pml4);

static inline void write_cr3(uintptr_t cr3)
{
    asm volatile("mov %0, %%cr3" :: "r"(cr3));
}

static inline void enable_paging(void)
{
    uintptr_t value;
    asm volatile("mov %%cr0, %0" : "=a"(value));
    value |= 0b10000000000000000000000000000000;
    asm volatile("mov %0, %%cr0" :: "r"(value));
}

static inline void disable_paging(void)
{
    uintptr_t value;
    asm volatile("mov %%cr0, %0" : "=a"(value));
    value &= 0b01111111111111111111111111111111;
    asm volatile("mov %0, %%cr0" :: "r"(value));
}

#endif