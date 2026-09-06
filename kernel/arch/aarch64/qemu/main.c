#include "../../../kairax/types.h"
#include "../fdt.h"
#include "../mmu.h"
#include "../arm.h"

#define QEMU_DTB_BASE     0x40000000UL
#define KERNEL_PHYS_BASE  0x41000000UL

void qemu_write(size_t size, uint8_t * buffer) 
{
	for (unsigned int i = 0; i < size; ++i) 
    {
		*(volatile unsigned int *)(0x09000000) = buffer[i];
	}
}

void qemu_write1(size_t size, uint8_t * buffer) 
{
	for (unsigned int i = 0; i < size; ++i) 
    {
		*(volatile unsigned int *)(0xffffff8009000000) = buffer[i];
	}
}

static struct BaseTables {
	uintptr_t l0_base[512];
	uintptr_t l1_high_gbs[512];
	uintptr_t l1_low_gbs[512];
	uintptr_t l2_kernel[512];
} _baseTables __attribute__((aligned(4096)));

void setup_bootstrap_mmu() 
{
    // корень
    _baseTables.l0_base[0]   = (uintptr_t)&_baseTables.l1_low_gbs | PTE_VALID | PTE_TABLE | PTE_AF;
    // последнее в корне
	_baseTables.l0_base[511] = (uintptr_t)&_baseTables.l1_high_gbs | PTE_VALID | PTE_TABLE | PTE_AF;

	// физическая память на виртуальную 1 к 1
	_baseTables.l1_low_gbs[1] = QEMU_DTB_BASE | PTE_VALID | PTE_AF | PTE_SH_OUTER | (1 << 2);

	// 64Гб физической памяти
	for (size_t i = 0; i < 64; ++i) 
    {
		_baseTables.l1_high_gbs[i] = (i << 30) | PTE_VALID | PTE_AF | PTE_SH_OUTER | (1 << 2);
	}

	// Код ядра в L1
	_baseTables.l1_high_gbs[510] = (uintptr_t)&_baseTables.l2_kernel | PTE_VALID | PTE_TABLE | PTE_AF;
    // Код ядра в L2
	for (size_t i = 0; i < 512; ++i) 
	{
		_baseTables.l2_kernel[i] = (KERNEL_PHYS_BASE + (i << 21)) | PTE_VALID | PTE_AF | PTE_SH_OUTER | (1 << 2);
	}

    // включаем MMU, кэши
	uint64_t sctlr = SCTLR_MMU_ENABLE | SCTLR_CACHE | SCTLR_ITD | SCTLR_INSTR_CACHE | SCTLR_TSCXT | SCTLR_SPAN | SCTLR_nTLSMD | SCTLR_LSMAOE;

	// Translate control register
	uint64_t tcr = 
        (TCR_IPS_42BIT << TCR_IPS_OFFSET) |         // 4Tб
	    (TCR_TG1_4KB << TCR_TG1_OFFSET) |           // TG1 4KB granules in TTBR1
		(TCR_T0SZ_48BIT << TCR_T1SZ_OFFSET) |       // T1SZ = 48bit (64 - 16 = 48)
		(TCR_SH_INNER << TCR_SH1_OFFSET) |          // SH1
		(TCR_WBRAWA << TCR_OGRN1_OFFSET) |          // ORGN1
		(TCR_WBRAWA << TCR_IGRN1_OFFSET) |          // IRGN1
		(TCR_TG0_4KB << TCR_TG0_OFFSET) |           // TG0 4KB granules in TTBR0
		(TCR_T0SZ_48BIT << TCR_T0SZ_OFFSET) |       // T0SZ = 48bit (64 - 16 = 48)
		(TCR_SH_INNER << TCR_SH0_OFFSET) |          // SH0
		(TCR_WBRAWA << TCR_OGRN0_OFFSET) |          // ORGN0
		(TCR_WBRAWA << TCR_IGRN0_OFFSET);           // IRGN0

	// MAIR
	uint64_t mair  = (0x000000000044ff00);
	asm volatile ("msr MAIR_EL1, %0" :: "r"(mair));

	asm volatile ("msr TCR_EL1, %0" : : "r"(tcr));
	asm volatile ("msr TTBR0_EL1, %0" : : "r"(&_baseTables.l0_base));
	asm volatile ("msr TTBR1_EL1, %0" : : "r"(&_baseTables.l0_base));
	asm volatile ("dsb ishst");
	asm volatile ("tlbi vmalle1is");
	asm volatile ("dsb ish");
	asm volatile ("isb" ::: "memory");
	asm volatile ("msr SCTLR_EL1, %0" : : "r"(sctlr));
	asm volatile ("isb" ::: "memory");

	qemu_write1(27, "bootstub: MMU initialized\n");
}

void kmain()
{
    qemu_write(4, "TEST");

    setup_bootstrap_mmu();

    while(1)
    {

    }
}