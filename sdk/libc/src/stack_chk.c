#include "unistd.h"
#include "stdlib.h"

static const char __stack_smash_msg[] = "Stack smashed, terminating...";

void __stack_chk_fail(void) {
    write(2, __stack_smash_msg, sizeof(__stack_smash_msg) - 1);
    abort();
}