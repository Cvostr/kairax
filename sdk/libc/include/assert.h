#ifndef _ASSERT_H
#define _ASSERT_H

#include <sys/cdefs.h>

__BEGIN_DECLS

void __assert_fail (const char *assertion, const char *file, unsigned int line, const char *function) __THROW;

#define assert(expr) ((void) ((expr) ? 0 :	(__assert_fail (#expr, __FILE__, __LINE__, __func__), 0)))

__END_DECLS

#endif