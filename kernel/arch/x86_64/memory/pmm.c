#include "pmm.h"
#include "string.h"
#include "stdio.h"

#define MAX_BITMASK_DATA 655356

extern uintptr_t __KERNEL_START;
extern uintptr_t __KERNEL_END;

uint64_t bitmap[MAX_BITMASK_DATA];
uint64_t pages_used = 0;

void set_bit(uint64_t bit) {
    bitmap[bit / 64] |=  (1ull << (bit % 64));
}

void unset_bit(uint64_t bit) {
    bitmap[bit / 64] &= ~(1ull << (bit % 64));
}

int test_bit(uint64_t bit)
{
    return (bitmap[bit / 64] & (1ull << (bit % 64)));
}

uint64_t first_free_page()
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

uint64_t *alloc_page() {
  	uint64_t addr;

  	uint64_t i = first_free_page();

	if (i<0) {
		return 0;
	}

	set_bit(i);

	i *= PAGE_SIZE;
	pages_used++;

	return (uint64_t*)i;
}

void free_page(uint64_t addr) { 
  	unset_bit(addr / PAGE_SIZE);
  	pages_used--;
}

void set_mem_region(uint64_t offset, uint64_t size){
  	if (size < PAGE_SIZE) {
    	set_bit(offset / PAGE_SIZE);
    	pages_used += 1;
  	} else {
    	for (int i = 0; i< size / PAGE_SIZE; i++) {      
      		set_bit(offset / PAGE_SIZE + i);
    	}
    	pages_used += size / PAGE_SIZE;
  	}
}

void init_pmm(){
	memset(bitmap, 0, sizeof(uint64_t) * MAX_BITMASK_DATA);
	pages_used = 0;
	uint64_t kernel_size = (uint64_t)&__KERNEL_END - (uint64_t)&__KERNEL_START;
	//Запретить выделение памяти в 1-м мегабайте
	set_mem_region(0x0, 0x100000);

	set_mem_region(0x100000, kernel_size);
}