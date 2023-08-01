#ifndef _ERRNO_H
#define _ERRNO_H

__thread int errno;

#define ERROR_BAD_FD                9
#define ERROR_NO_FILE               2
#define ERROR_IS_DIRECTORY          21
#define ERROR_NOT_A_DIRECTORY       20        
#define ERROR_TOO_MANY_OPEN_FILES   24

#endif