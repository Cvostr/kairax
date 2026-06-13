// ВНИМАНИЕ!!!
// Намеренно не делаем защиту от нескольких включений (include guard)

#include <sys/cdefs.h>

__BEGIN_DECLS

void __assert_fail (const char *assertion, const char *file, unsigned int line, const char *function) __THROW;

// если assert уже было объявлено - снимаем объявление чтобы переопределить ниже
#ifdef assert
#undef assert
#endif

#ifdef NDEBUG
#define assert(expr) ((void) 0)
#else
#define assert(expr) ((void) ((expr) ? 0 :	(__assert_fail (#expr, __FILE__, __LINE__, __func__), 0)))
#endif

__END_DECLS