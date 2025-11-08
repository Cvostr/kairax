#ifndef _UTIME_H
#define _UTIME_H

#include <sys/cdefs.h>
#include <sys/types.h>
#include <time.h>

__BEGIN_DECLS

struct utimbuf {
  time_t actime;
  time_t modtime;
};

int utime(const char *path, const struct utimbuf *times) __THROW;

__END_DECLS

#endif