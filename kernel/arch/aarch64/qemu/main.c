#include "../../../kairax/types.h"
#include "../fdt.h"
#include "../mmu.h"
#include "../arm.h"

#define QEMU_DTB_BASE     0x40000000UL
#define KERNEL_PHYS_BASE  0x41000000UL

#define FW_CFG_BASE            0x09020000
#define FW_CFG_REG_DATA        (FW_CFG_BASE + 0x00)
#define FW_CFG_REG_SELECTOR    (FW_CFG_BASE + 0x08)
#define FW_CFG_FILE_DIR        0x0019

#define P2V(x)	(0xffffff8000000000 | x)

void qemu_write(size_t size, char * buffer) 
{
	for (unsigned int i = 0; i < size; ++i) 
    {
		*(volatile unsigned int *)(0x09000000) = buffer[i];
	}
}

void qemu_write1(size_t size, char * buffer) 
{
	for (unsigned int i = 0; i < size; ++i) 
    {
		*(volatile unsigned int *)(P2V(0x9000000)) = buffer[i];
	}
}

struct qemu_fw_cfg_file {
    uint32_t size;         // File size (Big Endian)
    uint16_t select;       // Key selector for file contents (Big Endian)
    uint16_t reserved;
    char name[56];         // Null-terminated name string
};

static inline uint32_t swap32(uint32_t val) 
{
    return __builtin_bswap32(val);
}
static inline uint16_t swap16(uint16_t val) 
{
    return __builtin_bswap16(val);
}

static int __strcmp(const char *s1, const char *s2) 
{
    while (*s1 && (*s1 == *s2)) 
	{ 
		s1++; s2++; 
	}

    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int fw_cfg_find_file(const char *target_name, uint16_t *out_select, uint32_t *out_size) 
{
    volatile uint16_t *selector = (volatile uint16_t *) P2V(FW_CFG_REG_SELECTOR);
    volatile uint32_t *data32   = (volatile uint32_t *) P2V(FW_CFG_REG_DATA);
    volatile uint8_t  *data8    = (volatile uint8_t  *) P2V(FW_CFG_REG_DATA);

    // 1. Select the file directory
    *selector = swap16(FW_CFG_FILE_DIR);

    // 2. Read total count of files in the catalog
    uint32_t count = swap32(*data32);

    // 3. Loop through entries and check names
    struct qemu_fw_cfg_file entry;
    for (uint32_t i = 0; i < count; i++) {
        
        // Stream the struct layout out byte-by-byte from the MMIO data register
        uint8_t *entry_ptr = (uint8_t *)&entry;
        for (size_t j = 0; j < sizeof(struct qemu_fw_cfg_file); j++) {
            entry_ptr[j] = *data8;
        }

        // Compare target string with the entry name
        if (__strcmp(entry.name, target_name) == 0) {
            *out_select = swap16(entry.select);
            *out_size   = swap32(entry.size);
            return 1; // Success
        }
    }

    return 0; // Not found
}

void fw_cfg_read_file(uint16_t key, void *buffer, uint32_t size) 
{
    volatile uint16_t *selector = (volatile uint16_t *) P2V(FW_CFG_REG_SELECTOR);
    volatile uint8_t  *data8    = (volatile uint8_t  *) P2V(FW_CFG_REG_DATA);
    uint8_t *dest = (uint8_t *)buffer;

    // Select the key tied to the file contents
    *selector = swap16(key);

    // Drain the bytes sequentially into memory
    for (uint32_t i = 0; i < size; i++) {
        dest[i] = *data8;
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

	uint16_t key;
	uint32_t kernel_size;
	
	// Explicitly target the literal string given to QEMU's name argument
    if (fw_cfg_find_file("opt/kairax.kernel", &key, &kernel_size)) {
        
        // Ensure you have a target buffer large enough to accommodate file_size bytes
        uint8_t *kernel_load_address = (uint8_t *)P2V(0x45000000); 
        
        fw_cfg_read_file(key, kernel_load_address, kernel_size);
        
        // The binary data from kernel.bin is now sitting at 0x45000000
    } else {
		qemu_write1(10, "NOTFOUND!");
        // TODO: halt!
	}

    while(1)
    {

    }
}