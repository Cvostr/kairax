#ifndef _DIRENT_H
#define _DIRENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include "inttypes.h"

#define MAX_DIRENT_NAME_LEN 260

#define DT_UNKNOWN       0
#define DT_FIFO          1
#define DT_CHR           2
#define DT_DIR           4
#define DT_BLK           6
#define DT_REG           8      // Обычный файл
#define DT_LNK          10
#define DT_SOCK         12
#define DT_WHT          14

struct dirent {
    ino_t       d_ino;      // номер inode
    off_t    	d_off;     // Номер в директории
	uint16_t	d_reclen;
    uint8_t     d_type;       // Тип
    char        d_name[MAX_DIRENT_NAME_LEN];     // Имя
};

struct __dirstream
{
	off_t tell;
	int fd;
	int buf_pos;
	int buf_end;
	volatile int lock[1];
	/* Any changes to this struct must preserve the property:
	 * offsetof(struct __dirent, buf) % sizeof(off_t) == 0 */
	char buf[2048];
};

typedef struct __dirstream DIR;

int readdir(int fd, struct dirent* direntry);

#ifdef __cplusplus
}
#endif

#endif