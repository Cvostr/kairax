#include "module.h"
#include "functions.h"

void myfunc()
{
	printk("Hello %i", 12);
}

int init()
{
	printk("Hello");
	myfunc();
	return 0;
}

void exit()
{
	printk("Bye");
}

DEFINE_MODULE("hello-module", init, exit)