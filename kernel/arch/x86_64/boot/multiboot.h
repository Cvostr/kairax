#pragma once

#include "types.h"

typedef struct PACKED {
  uint32_t type;
  uint32_t size;
  uint8_t data[];
} tag_t;

typedef struct PACKED {
  uint32_t total_size;
  uint32_t reserved;
  tag_t tags[];
} taglist_t;

typedef struct PACKED {
  uint64_t base;
  uint64_t len;
  uint32_t type;
  uint32_t reserved;
} mmap_entry_t;

typedef struct PACKED {
  uint32_t entry_size;
  uint32_t entry_version;
  mmap_entry_t entries[];
} mmap_t ;

#define BOOTLOADER_STRING_MAX_LEN 64
#define BOOTLOADER_ARGS_MAX_LEN   256  
#define RSDP_DATA_MAX_SIZE        200

typedef struct  PACKED {
    void* fb_addr;
    uint32_t fb_pitch;
    uint32_t fb_width;
    uint32_t fb_height;
    uint8_t  fb_bpp;
    uint8_t  fb_type;
    uint8_t  reserved; 
} framebuffer_info_t;

typedef struct {
    char bootloader_string[BOOTLOADER_STRING_MAX_LEN];
    char command_line[BOOTLOADER_ARGS_MAX_LEN];
    uint32_t load_base_addr;

    size_t mmap_size;
    unsigned int mmap_len;
    mmap_t* mmap;

    framebuffer_info_t fb_info;

    int rsdp_version;
    int rsdp_size;
    char rsdp_data[RSDP_DATA_MAX_SIZE];
} kernel_boot_info_t;

int parse_mb2_tags(taglist_t *tags);

kernel_boot_info_t* get_kernel_boot_info();

int multiboot_get_memory_area(size_t entry_index, uintptr_t *start, uintptr_t *length, uint32_t *type);