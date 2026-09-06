#ifndef AARCH64_MMU_H
#define AARCH64_MMU_H

#define PTE_VALID       (1UL << 0)
#define PTE_TABLE       (1UL << 1)

// Non-Secure
#define PTE_NS          (1UL << 5)
// Access Flag
#define PTE_AF          (1UL << 10)
// Not-Global
#define PTE_NG          (1UL << 11)

#define PTE_CONT        (1UL << 52)
#define PTE_PXN         (1UL << 53)
#define PTE_XN          (1UL << 54)

// Inner shareable
#define PTE_SH          (3UL << 8)
// Outer shareable
#define PTE_SH_OUTER    (1UL << 9)

#endif