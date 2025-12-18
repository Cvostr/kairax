#ifndef _ERR_H
#define _ERR_H

#include <sys/cdefs.h>
#include <stdarg.h>

__BEGIN_DECLS

void err(int eval, const char *fmt, ...);
void errx(int eval, const char *fmt, ...);
void verr(int eval, const char* fmt, va_list args);

void warn(const char* fmt, ...);
void vwarn(const char *fmt, va_list args);
void vwarnx(const char *fmt, va_list args);

__END_DECLS

#endif
