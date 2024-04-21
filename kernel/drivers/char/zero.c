#include "random.h"
#include "fs/devfs/devfs.h"
#include "string.h"

ssize_t zero_read (struct file* file, char* buffer, size_t size, loff_t offset);
struct file_operations zero_fops;

void zero_init()
{
    zero_fops.read = zero_read;
	devfs_add_char_device("zero", &zero_fops, NULL);
}

ssize_t zero_read (struct file* file, char* buffer, size_t size, loff_t offset)
{
    memset(buffer, 0, size);

    return size;
}