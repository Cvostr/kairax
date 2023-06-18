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

typedef struct {
  char* bootloader_string;
  char* command_line;
  uint32_t load_base_addr;

  size_t mmap_size;
  unsigned int mmap_len;
  mmap_t* mmap;
}kernel_boot_info_t;

int parse_mb2_tags(taglist_t *tags);

kernel_boot_info_t* get_kernel_boot_info();

int multiboot_get_memory_area(size_t entry_index, uintptr_t *start, uintptr_t *end, uint32_t *type);

int multiboot_page_used(uintptr_t start);