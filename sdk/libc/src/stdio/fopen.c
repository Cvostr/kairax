#include "stdio.h"
#include "string.h"
#include "fcntl.h"
#include "stdlib.h"
#include "unistd.h"
#include "stdio_impl.h"

int compute_flags(const char *restrict mode) 
{
    int flags = 0;

    if (strchr(mode, '+')) {
        flags = O_RDWR;
    } else if (*mode == 'r') {
        flags = O_RDONLY;
    } else 
        flags = O_WRONLY;

	if (strchr(mode, 'x')) {
        flags |= O_EXCL;
    }

	if (strchr(mode, 'e')) {
        flags |= O_CLOEXEC;
    }

	if (*mode != 'r') flags |= O_CREAT;
	if (*mode == 'w') flags |= O_TRUNC;
	if (*mode == 'a') flags |= O_APPEND;

    return flags;
}

FILE *fopen(const char *restrict filename, const char *restrict mode)
{
    if (!strchr("rwa", *mode)) {
        //set errno to INVALID VAL
		return 0;
	}

    int flags = compute_flags(mode);

    int fd = open(filename, flags, 0666);
    
    if (fd == -1) {
        return NULL;
    }

    FILE* file = fdopen(fd, mode);
    if (file) {
        return file;
    }
    
    close(fd);

    return NULL;
}

FILE *fdopen(int fd, const char* restrict mode)
{
    int uflags = compute_flags(mode);
    FILE* fil = malloc(sizeof(struct IO_FILE));
    memset(fil, 0, sizeof(struct IO_FILE));

    fil->_fileno = fd;
    fil->_buffer = malloc(STDIO_BUFFER_LENGTH);
    fil->_buf_len = STDIO_BUFFER_LENGTH;

    switch (uflags) {
        case O_RDWR:
            fil->_flags |= FSTREAM_CANWRITE;
        case O_RDONLY:
            fil->_flags |= FSTREAM_CANREAD;
            break;
        case O_WRONLY:
            fil->_flags |= FSTREAM_CANWRITE;
    }

    return fil;
}