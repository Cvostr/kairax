#include "random.h"
#include "fs/devfs/devfs.h"
#include "string.h"

ssize_t null_read (struct file* file, char* buffer, size_t size, loff_t offset);
ssize_t null_write (struct file* file, const char* buffer, size_t size, loff_t offset);
struct file_operations null_fops;

void null_init()
{
    null_fops.read = null_read;
    null_fops.write = null_write;
	devfs_add_char_device("null", &null_fops, NULL);
}

ssize_t null_write (struct file* file, const char* buffer, size_t size, loff_t offset)
{
    return size;
}

ssize_t null_read (struct file* file, char* buffer, size_t size, loff_t offset)
{
    return size;
}