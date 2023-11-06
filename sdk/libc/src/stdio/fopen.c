#include "stdio.h"
#include "string.h"
#include "fcntl.h"
#include "stdlib.h"
#include "unistd.h"
#include "stdio_impl.h"

int compute_flags(const char *mode) 
{
    int flags = 0;

    while (1) {
        
        switch (*mode) {
            case 'r': 
                flags |= O_RDONLY;
                break;
            case 'a':
                flags |= (O_WRONLY | O_CREAT | O_APPEND);
                break;
            case 'w':
                flags |= (O_WRONLY | O_CREAT | O_TRUNC); 
                break;
            case 'x':
                flags |= O_EXCL;
                break;
            case '+':
                flags |= (flags & (~O_WRONLY)) | O_RDWR;
                break;
            case '\0':
                return flags;
        }

        mode++;
    }

    return 0;
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

    switch (uflags & 3) {
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