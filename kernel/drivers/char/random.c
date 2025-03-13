#include "random.h"
#include "fs/devfs/devfs.h"
#include "proc/timer.h"
#include "string.h"

struct file_operations random_fops;
unsigned int rand_seed = 1;
#define RAND_MAX 32768

ssize_t random_read (struct file* file, char* buffer, size_t size, loff_t offset);

void random_init()
{
    struct timespec ticks;
    timer_get_ticks(&ticks);
    rand_seed = ticks.tv_sec;

    krand();

    memset(&random_fops, 0, sizeof(random_fops));
    random_fops.read = random_read;

	devfs_add_char_device("random", &random_fops, NULL);
}

int krand()
{
    rand_seed = rand_seed * 1103515245 + 12345;
    return (unsigned int) ((rand_seed / 65536) % RAND_MAX);
}

ssize_t random_read (struct file* file, char* buffer, size_t size, loff_t offset)
{
    int rnd = 0;

    uint32_t bsize = sizeof(rnd);
    size_t blocks = size / bsize;
    size_t written = 0;

    for (size_t i = 0; i < blocks; i ++) 
    {
        rnd = krand();
        memcpy(buffer + written, &rnd, bsize);
        written += bsize;
    }

    rnd = krand();
    memcpy(buffer + written, &rnd, size % bsize);

    return size;
}