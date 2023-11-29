static void init()
{
	printk("Hello");
}

static void exit()
{

}

struct module_metadata
{
    char name[20];
    void (*init)(void);
    void (*exit)(void);
};

__attribute__((section(".modinfo")))
struct module_metadata dummy = {
    .name = "hello-module",
    .init = init,
    .exit = exit,
};