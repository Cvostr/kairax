#include "module.h"
#include "functions.h"

int fopen(struct inode *inode, struct file *file)
{
	printk("fopen\n");
	return 0;
}

struct file_operations fops = {.open = fopen};

int init()
{
	printk("Hello from module %i\n", 123);

	devfs_add_char_device("hellodev", &fops);
	
	return 0;
}

void exit()
{
	printk("Bye");
}

DEFINE_MODULE("hello-module", init, exit)