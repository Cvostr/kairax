void enable_interrupts()
{
    asm volatile("sti": : : "memory");
}

void disable_interrupts()
{
    asm volatile("cli": : : "memory");
}