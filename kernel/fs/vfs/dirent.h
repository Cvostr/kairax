#ifndef _DIRENT_H
#define _DIRENT_H

#include "kairax/types.h"
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
    uint64_t    inode;      // номер inode
    uint64_t    offset;     // Номер в директории
    uint16_t	reclen;
    uint8_t     type;       // Тип
    char        name[MAX_DIRENT_NAME_LEN];     // Имя
};

#endif