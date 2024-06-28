#ifndef VM_AREA_H
#define VM_AREA_H

#define MAP_SHARED	    0x01
#define MAP_ANONYMOUS	0x20
#define MAP_STACK	    0x20000

struct mmap_range {
    uint64_t        base;
    uint64_t        length;
    int             protection;
    int             flags;
};

#endif