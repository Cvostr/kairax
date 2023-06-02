#include "pmm.h"
#include "string.h"
#include "stdio.h"

#define MAX_BITMASK_DATA 657356

extern uintptr_t __KERNEL_START;
extern uintptr_t __KERNEL_END;

uint64_t bitmap[MAX_BITMASK_DATA];
uint64_t pages_used = 0;

size_t physical_mem_size = 0;
size_t physical_mem_max_address = 0;

size_t pmm_get_physical_mem_size() {
	return physical_mem_size;
}

void pmm_set_physical_mem_size(size_t size) {
	physical_mem_size = size;
}

void pmm_set_physical_mem_max_addr(size_t size)
{
	physical_mem_max_address = size;
}

size_t pmm_get_physical_mem_max_addr()
{
	return physical_mem_max_address;
}

void set_bit(uint64_t bit) {
    bitmap[bit / 64] |=  (1ULL << (bit % 64));
}

void unset_bit(uint64_t bit) {
    bitmap[bit / 64] &= ~(1ULL << (bit % 64));
}

int test_bit(uint64_t bit)
{
    return (bitmap[bit / 64] & (1ULL << (bit % 64)));
}

uint64_t find_free_page()
{
	uint64_t i = 0;
	int bitpos;
	//сначала пропускаем все полностью заполненные 64-битные блоки
	while (bitmap[i] == 0xFFFFFFFFFFFFFFFF) {
    	i++;
    	if ( i == MAX_BITMASK_DATA)
      		return -1;
  	}

	uint64_t bits  = bitmap[i];
  	bitpos = 0;
	//ищем свободный бит
	while ((bits & 1) != 0) {
		bits >>= 1;
		bitpos++;
	}

	i *= (sizeof(uint64_t) * 8);
	i += bitpos;

	return i;
}

uint64_t find_free_pages(int pages) {
  	int bitpos;
  	uint64_t i = 0;
  	int nfound = 0;
  	while (i < MAX_BITMASK_DATA) {
		//сначала пропускаем все полностью заполненные 64-битные блоки
    	while (bitmap[i] == 0xFFFFFFFFFFFFFFFF) {
    		i++;
    	}
		uint64_t bits = bitmap[i];
		for (bitpos = 0; bitpos<64; bitpos++) {
			if ((bits & 1) != 0) {
				nfound = 0;
				bits >>= 1;
			} else {
				nfound++;
				if (pages == nfound) {
					i = i * (sizeof(uint64_t) * 8) + bitpos - pages + 1;
					return i;
				}
			}
		}
    	i++;   
  	}

  return -1;
}


uintptr_t* pmm_alloc_page() {
	//Найти номер первой свободной страницы
  	uint64_t i = find_free_page();
	if (i < 0) {
		return NULL;
	}
	//Установить бит занятости страницы
	set_bit(i);

	pages_used++;
	return (uintptr_t*)(i * PAGE_SIZE);
}

uintptr_t* pmm_alloc_pages(uint32_t pages){
	uint64_t i = find_free_pages(pages);

	if (i < 0) {
		return NULL;
	}
	//Установить бит занятости нескольких страниц
	for(uint32_t page_i = 0; page_i < pages; page_i ++){
		set_bit(i + page_i);
	}
	
	pages_used += pages;

	return (uintptr_t*)(i * PAGE_SIZE);
}

void pmm_free_page(uint64_t addr) { 
  	unset_bit(addr / PAGE_SIZE);
  	pages_used--;
}

void free_pages(uintptr_t* addr, uint32_t pages) {
	uint64_t page_index = ((uint64_t)addr) / PAGE_SIZE;
	for(uint32_t page_i = 0; page_i < pages; page_i ++){
		set_bit(page_index + page_i);
	}
	pages_used -= pages;
}

void pmm_set_mem_region(uint64_t offset, uint64_t size) {
  	if (size < PAGE_SIZE) {
    	set_bit(offset / PAGE_SIZE);
    	pages_used += 1;
  	} else {
    	for (int i = 0; i < size / PAGE_SIZE; i++) {      
      		set_bit(offset / PAGE_SIZE + i);
    	}
    	pages_used += size / PAGE_SIZE;
  	}
}

void init_pmm() {
	memset(bitmap, 0, sizeof(uint64_t) * MAX_BITMASK_DATA);
	pages_used = 0;
	uint64_t kernel_size = (uint64_t)&__KERNEL_END - (uint64_t)&__KERNEL_START;
	//Запретить выделение памяти в 1-м мегабайте
	pmm_set_mem_region(0x0, 0x100000);

	pmm_set_mem_region(0x100000, kernel_size);
}

uint64_t pmm_get_used_pages(){
	return pages_used;
}