#ifndef _PAGING_X64_H
#define _PAGING_X64_H

#include "types.h"
#include "mem_layout.h"
#include "mem/paging.h"
#include "mem/vm_area.h"

#define PAGE_PRESENT 		    0x1
#define PAGE_WRITABLE 		    0x2
#define PAGE_USER_ACCESSIBLE 	0x4
#define PAGE_HUGE 		        0x80
#define PAGE_GLOBAL 		    0x100
#define PAGE_STACK_GUARD	    0x200
#define PAGE_WRITE_THROUGH      0x8
#define PAGE_UNCACHED           0b10000
#define PAGE_EXEC_DISABLE       (1ULL << 63)

#define PAGE_BASE_FLAGS         PAGE_PRESENT | PAGE_WRITABLE

#define PAGE_ENTRY_NBR  0x200
#define INDEX_MASK      0x1FF

#define MAX_PAGES_4     (1ULL << 36)

//получить из 48-битного виртуального адреса индекс записи в таблице 4-го уровня (0x27 = 39)
#define GET_4_LEVEL_PAGE_INDEX(x) ((((uint64_t)(x)) >> 0x27) & INDEX_MASK) 
#define GET_3_LEVEL_PAGE_INDEX(x) ((((uint64_t)(x)) >> 0x1E) & INDEX_MASK)
#define GET_2_LEVEL_PAGE_INDEX(x) ((((uint64_t)(x)) >> 0x15) & INDEX_MASK) 
#define GET_1_LEVEL_PAGE_INDEX(x) ((((uint64_t)(x)) >> 0x0C) & INDEX_MASK) 
#define GET_PAGE_OFFSET(x)        ((uint64_t)(x) & (0xFFF)) 
#define GET_PAGE_FRAME(x)         (void*)((uint64_t)(x) & ~(PAGE_EXEC_DISABLE | 0xFFF))
#define GET_PAGE_FLAGS(x)         ((uint64_t)(x) & (PAGE_EXEC_DISABLE | 0xFFF)) 

extern void write_cr3(void*);
extern uint64_t read_cr3();

typedef uintptr_t virtual_addr_t;
typedef uintptr_t physical_addr_t;
typedef uint64_t    table_entry_t;

typedef struct PACKED {
    table_entry_t entries[512]; 
} page_table_t;

page_table_t* new_page_table();

//Создать виртуальную страницу с указанным адресом и назначить ей указанный физический
int map_page_mem(page_table_t* root, virtual_addr_t virtual_addr, physical_addr_t physical_addr, uint64_t flags);
//Удалить виртуальную страницу
int unmap_page(page_table_t* root, uintptr_t virtual_addr);
//Удалить виртуальную страницу
int unmap_page1(page_table_t* root, uintptr_t virtual_addr, uintptr_t* phys_addr);
//Получить физический адрес из виртуального для указанной корневой таблицы
physical_addr_t get_physical_address(page_table_t* root, virtual_addr_t virtual_addr);

int set_page_flags(page_table_t* root, uintptr_t virtual_addr, uint64_t flags);

int page_table_mmap_fork(page_table_t* src, page_table_t* dest, struct mmap_range* area, int cow);

// Уничтожить страницы в таблице виртуальной памяти
void destroy_page_table(table_entry_t* entries, int level, uint64_t mask);

virtual_addr_t get_first_free_pages(page_table_t* root, uint64_t pages_count);

virtual_addr_t get_first_free_pages_from(virtual_addr_t start, page_table_t* root, uint64_t pages_count);

//Переключить текущую 4х уровневую таблицу страниц
void switch_pml4(page_table_t* pml4);


#endif