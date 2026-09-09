.extern __bootstrap_stack_top
.globl start
start:
    // пускаем только первое ядро
    mrs     x0, mpidr_el1
    and     x0, x0, #0xFF
    cbz     x0, continue

    // остальные тут
hang:
    wfe
    b hang

continue:
    ldr x30, =__bootstrap_stack_top
    mov sp, x30
    bl kmain
    // не должны сюда попадать, но на всякий случай
    b hang
