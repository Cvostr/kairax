extern unsigned long x64_getrflags();

void enable_interrupts()
{
    asm volatile("sti": : : "memory");
}

void disable_interrupts()
{
    asm volatile("cli": : : "memory");
}

int check_interrupts() 
{
    volatile unsigned long flags = x64_getrflags();

    return (flags & 0x200) != 0;
}