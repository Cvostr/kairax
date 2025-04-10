#include "pmm.h"
#include "string.h"
#include "stdio.h"
#include "sync/spinlock.h"
#include "kstdlib.h"

#define MAX_BITMASK_DATA 657356

extern uint64_t __KERNEL_START;
extern uint64_t __KERNEL_END;
extern uint64_t __KERNEL_VIRT_END;
extern uint64_t __KERNEL_VIRT_LINK;

uint64_t bitmap[MAX_BITMASK_DATA];
uint64_t pages_used = 0;
spinlock_t pmm_lock;

pmm_params_t pmm_params;

#define PMM_MAX_REGIONS 100
struct pmm_region pmm_regions[PMM_MAX_REGIONS];
size_t pmm_regions_count = 0;

void pmm_add_region(uint64_t base, uint64_t length, int flags)
{
	pmm_regions[pmm_regions_count].base = base;
	pmm_regions[pmm_regions_count].length = length;
	pmm_regions[pmm_regions_count].flags = flags;
	pmm_regions_count++;
}

struct pmm_region* pmm_get_regions()
{
	return pmm_regions;
}

void pmm_set_params(pmm_params_t* params)
{
	memcpy(&pmm_params, params, sizeof(pmm_params_t));
}

size_t pmm_get_physical_mem_size() {
	return pmm_params.physical_mem_total;
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
    	if (i == MAX_BITMASK_DATA)
      		return 0;
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
		for (bitpos = 0; bitpos < 64; bitpos++) {
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

  	return 0;
}


void* pmm_alloc_page() 
{
	void* result = NULL;
	acquire_spinlock(&pmm_lock);

	//Найти номер первой свободной страницы
  	uint64_t i = find_free_page();
	if (i == 0) {
		goto exit;
	}
	
	//Установить бит занятости страницы
	set_bit(i);
	// Увеличить количество занятых страниц
	pages_used++;

	result = (void*)(i * PAGE_SIZE);

exit:
	release_spinlock(&pmm_lock);

	return result;
}

void* pmm_alloc(size_t bytes, size_t* npages)
{
	size_t aligned_size = align(bytes, PAGE_SIZE);
	size_t _npages = aligned_size / PAGE_SIZE;

	if (npages != NULL)
	{
		*npages = _npages;
	}

	return pmm_alloc_pages(_npages);
}

void* pmm_alloc_pages(uint32_t pages)
{
	void* result = NULL;
	acquire_spinlock(&pmm_lock);

	uint64_t i = find_free_pages(pages);

	if (i == 0) {
		goto exit;
	}

	// Установить бит занятости нескольких страниц
	for (uint32_t page_i = 0; page_i < pages; page_i ++) {
		set_bit(i + page_i);
	}
	
	result = (void*)(i * PAGE_SIZE);
	pages_used += pages;

exit:
	release_spinlock(&pmm_lock);

	return result;
}

void pmm_free_page(void* addr) 
{
	acquire_spinlock(&pmm_lock);

  	unset_bit((uint64_t)addr / PAGE_SIZE);
  	pages_used--;

	release_spinlock(&pmm_lock);
}

void pmm_free_pages(void* addr, uint32_t pages)
{
	acquire_spinlock(&pmm_lock);

	uint64_t page_index = ((uint64_t)addr) / PAGE_SIZE;

	for(uint32_t page_i = 0; page_i < pages; page_i ++) {
		unset_bit(page_index + page_i);
	}

	pages_used -= pages;

	release_spinlock(&pmm_lock);
}

void pmm_set_mem_region(uint64_t offset, uint64_t size, int mark_as_used)
{
	offset -= (offset % PAGE_SIZE); // выравнивание в меньшую сторону
	size = align(size, PAGE_SIZE); //выравнивание в большую сторону

    for (uint64_t i = 0; i < size / PAGE_SIZE; i++) {      
      	set_bit(offset / PAGE_SIZE + i);
    }
    
	if (mark_as_used)
		pages_used += size / PAGE_SIZE;
}

void init_pmm() 
{
	memset(bitmap, 0, sizeof(uint64_t) * MAX_BITMASK_DATA);
	pages_used = 0;
}

void pmm_take_base_regions()
{
	uint64_t kernel_size = (uint64_t)&__KERNEL_VIRT_END - (uint64_t)&__KERNEL_VIRT_LINK;
	//Запретить выделение памяти в 1-м мегабайте
	pmm_set_mem_region(0x0, 0x100000, TRUE);
	// Запретить выделение памяти в области кода ядра
	pmm_set_mem_region(0x100000, kernel_size, TRUE);
}

uint64_t pmm_get_used_pages()
{
	acquire_spinlock(&pmm_lock);
	size_t r = pages_used;
	release_spinlock(&pmm_lock);
	return r;
}