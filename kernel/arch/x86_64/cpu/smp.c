#include "smp.h"
#include "dev/acpi/acpi.h"
#include "string.h"
#include "mem/pmm.h"
#include "memory/kernel_vmm.h"
#include "gdt.h"
#include "stdio.h"
#include "kstdlib.h"
#include "cpu_local_x64.h"
#include "mem/kheap.h"
#include "msr.h"
#include "interrupts/apic.h"
#include "interrupts/idt.h"
#include "cpuid.h"

extern char ap_trampoline[];
extern char ap_trampoline_end[];
extern char ap_gdtr[];
extern char ap_vmem[];
void* ap_stack;

int ap_counter = 0;
int ap_started_flag = 0;

struct cpu_local_x64* curr_cpu_local;

extern void syscall_entry_x64();

#define TRAMPOLINE_PAGE 1

static inline uint64_t read_tsc(void) {
	uint32_t lo, hi;
	asm volatile ( "rdtsc" : "=a"(lo), "=d"(hi) );
	return ((uint64_t)hi << 32) | (uint64_t)lo;
}

static void short_delay(unsigned long amount) {
	uint64_t clock = read_tsc();
	while (read_tsc() < clock + amount * 10000);
}

void ap_init()
{
    ap_counter ++;

    gdtr_t gdtr;
    gdtr.base = (uintptr_t)curr_cpu_local->gdt;
    gdtr.limit = curr_cpu_local->gdt_size - 1;
    // Установка GDT и TSS
    gdt_update(&gdtr);
    x64_ltr(TSS_DEFAULT_OFFSET);
    // Установить таблицу дескрипторов прерываний
    ap_setup_idt();
    lapic_write(LAPIC_REG_SPURIOUS, lapic_read(LAPIC_REG_SPURIOUS) | (1 << 8) | 0xff);
    // Установка параметров для syscall
    cpu_set_syscall_params(syscall_entry_x64, 0x8, 0x10, 0xFFFFFFFF);

    // Ядро запущено, можно запускать следующее
    ap_started_flag = 1;

    while(1) {
        char* addr = P2V(0xB8002);
        *addr = 'X';
    }
}

void smp_init()
{
    acpi_madt_t* madt = acpi_get_madt();

    size_t trampoline_size = (uint64_t)&ap_trampoline_end - (uint64_t)&ap_trampoline;

    // адрес тармплина
    uint64_t trampoline_addr = TRAMPOLINE_PAGE << 12;

    // Выделить память под копию физической страницы
    void* temp = pmm_alloc_page();
    memcpy(P2V(temp), P2V(trampoline_addr), PAGE_SIZE);

    // Скопировать код трамплина в первую физическую страницу
    memcpy(P2V(trampoline_addr), &ap_trampoline, trampoline_size);

    // Получить номер начального ядра
    uint8_t bspid = cpuid_get_apic_id();

    apic_local_cpu_t** lapic_array = acpi_get_cpus_apic();

    // Вычислить адрес GDTR трамплина
    uint64_t trampoline_gdtr_offset = (uint64_t)&ap_gdtr - (uint64_t)&ap_trampoline;
    gdtr_t* ap_gdtr = (gdtr_t*)P2V(trampoline_addr + trampoline_gdtr_offset); 

    // Вычислить адрес поля для адреса страницы памяти в трамплине
    uint64_t vmem_offset = (uint64_t)&ap_vmem - (uint64_t)&ap_trampoline;
    uint32_t* vmem_addr = P2V(trampoline_addr + vmem_offset);
    // Записать адрес таблицы
    *vmem_addr = K2P(get_kernel_pml4());

    // Страница для временных GDT
    char* bootstrap_page = (char*)pmm_alloc_page();
    map_page_mem(get_kernel_pml4(), bootstrap_page, bootstrap_page, PAGE_PRESENT | PAGE_WRITABLE);

    // замапить страницу с трамплином на 0x1000
    map_page_mem(get_kernel_pml4(), trampoline_addr, trampoline_addr, PAGE_PRESENT | PAGE_WRITABLE);

    // Создаем стартовую GDT, общую для старта всех ядер
    size_t reqd_gdt_size = gdt_get_required_size(5, 1);
    gdt_entry_t* gdt_addr = (gdt_entry_t*)bootstrap_page;
    // код ядра (0x8)
    gdt_set(gdt_addr + 1, 0, 0, GDT_BASE_KERNEL_CODE_ACCESS, GDT_FLAGS);
    // данные ядра (0x10)
    gdt_set(gdt_addr + 2, 0, 0, GDT_BASE_KERNEL_DATA_ACCESS, GDT_FLAGS);
    // данные пользователя + стэк (0x18)
    gdt_set(gdt_addr + 3, 0, 0, GDT_BASE_USER_DATA_ACCESS, GDT_FLAGS);
    // код пользователя (0x20)
    gdt_set(gdt_addr + 4, 0, 0, GDT_BASE_USER_CODE_ACCESS, GDT_FLAGS);
    // TSS
    tss_t* tss_addr = (tss_t*)(bootstrap_page + reqd_gdt_size);
    tss_addr->iopb = sizeof(tss_t) - 1;
    // TSS 0x28
    gdt_set_sys_seg(gdt_addr + 5, sizeof(tss_t) - 1, (uintptr_t)tss_addr, 0b10001001, 0b1001);

    // запись GDT в память трамплина
    ap_gdtr->base = gdt_addr;
    ap_gdtr->limit = reqd_gdt_size - 1;

    // Инициализировать ядра
    for (int cpu_i = 0; cpu_i < acpi_get_cpus_apic_count(); cpu_i ++) {

        if (cpu_i == bspid)
            continue;

        //printf("CORE %i\n", cpu_i);

        // сбрасываем флаг старта
        ap_started_flag = 0;

        // Выделить память для стека ядра и сохранить ее
        ap_stack = P2V(pmm_alloc_page() + PAGE_SIZE);

        // Создать структуру локальных данных ядра
        curr_cpu_local = (struct cpu_local_x64*)kmalloc(sizeof(struct cpu_local_x64));
        curr_cpu_local->lapic_id = lapic_array[cpu_i]->lapic_id;
        curr_cpu_local->id = cpu_i;

        // Создать GDT для ядра
        gdt_create(&curr_cpu_local->gdt, &curr_cpu_local->gdt_size, &curr_cpu_local->tss);

        // IPI
        lapic_send_ipi(lapic_array[cpu_i]->lapic_id, 0x4500);

        // задержка
		short_delay(5000UL);

		/* SIPI со страницей 1 */
		lapic_send_ipi(lapic_array[cpu_i]->lapic_id, (0x4600 | TRAMPOLINE_PAGE));

        do { asm volatile ("pause" : : : "memory"); } while (!ap_started_flag);
    }

    char* model[48];
	cpuid_get_model_string(model);
	printf("CPU %s, %i cores initialized\n", model, acpi_get_cpus_apic_count());

    memcpy(P2V(trampoline_addr), P2V(temp), PAGE_SIZE);
    pmm_free_page(temp);
    pmm_free_page(bootstrap_page);

    unmap_page(get_kernel_pml4(), bootstrap_page);
    unmap_page(get_kernel_pml4(), trampoline_addr);

    memset(P2V(K2P(get_kernel_pml4())), 0, 2048);
}