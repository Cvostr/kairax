#ifndef _SYS_MOUNT_H
#define _SYS_MOUNT_H

#include <sys/cdefs.h>
#include "types.h"

__BEGIN_DECLS

int mount(const char* device, const char* mount_dir, const char* fs, unsigned long flags);

__END_DECLS

#endif