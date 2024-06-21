#ifndef VM_AREA_H
#define VM_AREA_H

struct mmap_range {
    uint64_t        base;
    uint64_t        length;
    int             protection;
    int             flags;
};


#endif