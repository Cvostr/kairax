#ifndef _SETJMP_H
#define _SETJMP_H

#include <sys/cdefs.h>
#include <signal.h>

__BEGIN_DECLS

#ifdef __X86_64__
typedef long long __jmp_buf[8];
#endif

struct __jmp_buf_tag {
    __jmp_buf _buf;
    sigset_t _sigmask;
};

typedef struct __jmp_buf_tag jmp_buf[1];

int __sigsetjmp(jmp_buf env, int savemask);

void longjmp(jmp_buf env, int val);

#define setjmp(env) __sigsetjmp(env, 1)

__END_DECLS

#endif