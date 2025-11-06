#ifndef _SYS_PARAM_H
#define _SYS_PARAM_H

#undef MIN
#undef MAX
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#endif