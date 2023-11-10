#ifndef _TTY_H
#define _TTY_H

#include "fs/vfs/file.h"
#include "fs/vfs/inode.h"

void tty_init();
int master_file_close(struct inode *inode, struct file *file);
int slave_file_close(struct inode *inode, struct file *file);

int tty_create(struct file **master, struct file **slave);

ssize_t master_file_write(struct file* file, const char* buffer, size_t count, loff_t offset);
ssize_t master_file_read(struct file* file, char* buffer, size_t count, loff_t offset);

ssize_t slave_file_write(struct file* file, const char* buffer, size_t count, loff_t offset);
ssize_t slave_file_read(struct file* file, char* buffer, size_t count, loff_t offset);

#endif