#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#include "cdefs.h"
#include "types.h"

__BEGIN_DECLS

struct statfs {

    u_long    f_type;
    blksize_t f_bsize;
    
    blkcnt_t f_blocks;
    blkcnt_t f_bfree;

    inocnt_t f_files;
    inocnt_t f_ffree;
};

int statfs(const char *path, struct statfs *buf);
int fstatfs(int fd, struct statfs *buf);

__END_DECLS

#endif