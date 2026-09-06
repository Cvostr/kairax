#ifndef ARM_H
#define ARM_H

#define SCTLR_MMU_ENABLE    (1)
#define SCTLR_CACHE         (1 << 2)
#define SCTLR_ITD           (1 << 7)
#define SCTLR_INSTR_CACHE   (1 << 12)
#define SCTLR_TSCXT         (1 << 20)
#define SCTLR_SPAN          (1 << 23)
#define SCTLR_nTLSMD        (1 << 28)
#define SCTLR_LSMAOE        (1 << 29)

// TCR Offsets
#define TCR_T0SZ_OFFSET     0
#define TCR_IGRN0_OFFSET    8
#define TCR_OGRN0_OFFSET    10
#define TCR_SH0_OFFSET      12
#define TCR_TG0_OFFSET      14
#define TCR_T1SZ_OFFSET     16
#define TCR_IGRN1_OFFSET    24
#define TCR_OGRN1_OFFSET    26
#define TCR_SH1_OFFSET      28
#define TCR_TG1_OFFSET      30
#define TCR_IPS_OFFSET      32

// TCR Values
#define TCR_IPS_32BIT       0b000ULL
#define TCR_IPS_42BIT       0b011ULL
#define TCR_IPS_48BIT       0b101ULL

#define TCR_SH_INNER        0b11
#define TCR_SH_OUTER        0b10
#define TCR_SH_NON          0b00

#define TCR_TG0_4KB         0b00UL
#define TCR_TG0_64KB        0b01UL
#define TCR_TG0_16KB        0b10UL

#define TCR_TG1_4KB         0b10UL
#define TCR_TG1_64KB        0b11UL
#define TCR_TG1_16KB        0b01UL

#define TCR_NON_CACHE       0b00UL
#define TCR_WBRAWA          0b01UL    // Write-Back Read-Allocate Write-Allocate
#define TCR_WRITE_THROUGH   0b10UL    
#define TCR_WBnRA           0b11UL    // Write-Back no Read-Allocate

#define TCR_T0SZ_48BIT      16UL

#endif