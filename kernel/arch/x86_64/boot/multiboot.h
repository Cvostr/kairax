#pragma once

#include "stdint.h"

typedef struct PACKED {
  uint32 type;
  uint32 size;
  uint8 data[];
} tag_t;

typedef struct PACKED {
  uint32 total_size;
  uint32 reserved;
  tag_t tags[];
} taglist_t;

typedef struct PACKED {
  uint64 base;
  uint64 len;
  uint32 type;
  uint32 reserved;
} mmap_entry_t;

typedef struct PACKED {
  uint32 entry_size;
  uint32 entry_version;
  mmap_entry_t entries[];
} mmap_t ;

typedef struct {
  char* bootloader_string;
  char* command_line;

  size_t mmap_size;
  unsigned int mmap_len;
  mmap_t* mmap;
}kernel_boot_info_t;

int parse_mb2_tags(taglist_t *tags);

kernel_boot_info_t* get_kernel_boot_info();

int multiboot_get_memory_area(size_t entry_index, uintptr_t *start, uintptr_t *end, uint32 *type);

int multiboot_page_used(uintptr_t start);