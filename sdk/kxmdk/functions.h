#ifndef KXMDK_FUNCTIONS_H
#define KXMDK_FUNCTIONS_H

#include "mem/vmm.h"
#include "fs/vfs/inode.h"
#include "kairax/intctl.h"
#include "net/net_buffer.h"

#define PAGE_SIZE 4096

int printk(const char* __restrict, ...);

void* kmalloc(uint64_t size);
void kfree(void* mem);

void* pmm_alloc_page();
void* pmm_alloc_pages(uint32_t pages);
void pmm_free_pages(void* addr, uint32_t pages);

int devfs_add_char_device(const char* name, struct file_operations* fops, void* private_data);

struct process;
struct thread;

struct process*  create_new_process(struct process* parent);
struct thread* create_kthread(struct process* process, void (*function)(void), void* arg);

void scheduler_add_thread(struct thread* thread);

void eth_handle_frame(struct net_buffer* nbuffer);

#endif