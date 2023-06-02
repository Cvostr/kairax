#include "multiboot.h"
#include "string.h"

#define MBOOT_REPLY 0x36D76289

#define MBOOT2_COMMANDLINE 1
#define MBOOT2_BOOTLOADER 2
#define MBOOT2_MMAP 6
#define MBOOT2_IMAGE_BASE 21

static kernel_boot_info_t kernel_boot_info;

int parse_mb2_tags(taglist_t *tags)
{
  tag_t *tag = tags->tags;
  mmap_t *mmap;
  int index = 0;
  while(tag->type)
  {
    switch(tag->type)
    {
      case MBOOT2_BOOTLOADER:
        kernel_boot_info.bootloader_string = (char *)tag->data;
        break;
      case MBOOT2_COMMANDLINE:
        kernel_boot_info.command_line = (char *)tag->data;
        break;
      case MBOOT2_MMAP:
        mmap = kernel_boot_info.mmap = (mmap_t*)tag->data;
        kernel_boot_info.mmap_len = (tag->size - 8) / mmap->entry_size;
        kernel_boot_info.mmap_size = (tag->size - 8);
        break;
      case MBOOT2_IMAGE_BASE:
        kernel_boot_info.load_base_addr = *(uint32_t*)tag->data;
        break;
    }
    tag = &tags->tags[++index];
  }
  return 0;
}

kernel_boot_info_t* get_kernel_boot_info(){
  return &kernel_boot_info;
}


int multiboot_get_memory_area(size_t entry_index, uintptr_t *start, uintptr_t *end, uint32_t *type)
{
  if(entry_index >= kernel_boot_info.mmap_len) return 1;

  mmap_t *mmap = kernel_boot_info.mmap;
  mmap_entry_t *entry = &mmap->entries[entry_index];

  *start = entry->base;
  *end = entry->base + entry->len;
  *type = entry->type;
  return 0;
}

int multiboot_page_used(uintptr_t start)
{
  #define PAGE_SIZE 0x1000
#define overlap(st, len) ((uintptr_t)st < (start + PAGE_SIZE) && start <= ((uintptr_t)st + len))
  if(
      overlap(kernel_boot_info.bootloader_string, strlen(kernel_boot_info.bootloader_string)) ||
      overlap(kernel_boot_info.command_line, strlen(kernel_boot_info.command_line)) ||
      overlap(kernel_boot_info.mmap, kernel_boot_info.mmap_size) ||
    0)
    return 1;

  return 0;
}