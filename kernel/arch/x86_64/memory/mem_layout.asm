%define KERNEL_PHYS_START       0x0000000000100000
%define KERNEL_VIRT_LINK        0xFFFFFFFF80100000
%define KERNEL_VIRT_BASE        0xFFFFFFFF80000000

%define PHYSICAL_MEM_MAP_OFFSET 0xFFFF888000000000

%define P2K(x)           ((x) + KERNEL_VIRT_BASE)
%define K2P(x)           ((x) - KERNEL_VIRT_BASE)

%define P2V(x)           ((x) + PHYSICAL_MEM_MAP_OFFSET)
%define V2P(x)           ((x) - PHYSICAL_MEM_MAP_OFFSET)


%define PAGE_ENTRY_SIZE  0x8

%define PAGE_ADDR_BITSIZE 0x9
%define PML4_ADDR_INDEX   0x27

%define PML4_ENTRY_INDEX_KERN ((KERNEL_VIRT_LINK >> PML4_ADDR_INDEX) & ((0x1 << PAGE_ADDR_BITSIZE) - 0x1))
%define PML4_ENTRY_INDEX_PHYS ((PHYSICAL_MEM_MAP_OFFSET >> PML4_ADDR_INDEX) & ((0x1 << PAGE_ADDR_BITSIZE) - 0x1))

%define PDPT_ADDR_INDEX   0x1E
%define PDPT_ENTRY_INDEX_KERN ((KERNEL_VIRT_LINK >> PDPT_ADDR_INDEX) & ((0x1 << PAGE_ADDR_BITSIZE) - 0x1))
%define PDPT_ENTRY_INDEX_PHYS ((PHYSICAL_MEM_MAP_OFFSET >> PDPT_ADDR_INDEX) & ((0x1 << PAGE_ADDR_BITSIZE) - 0x1))