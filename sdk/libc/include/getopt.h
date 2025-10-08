#ifndef _GETOPT_H
#define _GETOPT_H

#include "sys/cdefs.h"

__BEGIN_DECLS

extern int optind, opterr, optopt;
extern char *optarg;
int getopt(int argc, char *const argv[], const char *options);

__END_DECLS

#endif