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
#define KFREE_SAFE(x) if (x) {kfree(x);}

void* pmm_alloc_page();
void* pmm_alloc_pages(uint32_t pages);
void* pmm_alloc(size_t bytes, size_t* npages);
void pmm_free_page(void* addr);
void pmm_free_pages(void* addr, uint32_t pages);

int devfs_add_char_device(const char* name, struct file_operations* fops, void* private_data);

struct process;
struct thread;

struct process*  create_new_process(struct process* parent);
struct thread* create_kthread(struct process* process, void (*function)(void), void* arg);

void process_set_name(struct process* process, const char* name);
void thread_set_name(struct thread* thread, const char* name);

void scheduler_add_thread(struct thread* thread);

void eth_handle_frame(struct net_buffer* nbuffer);

void wait_active_ms(uint64_t ms);

#endif