#ifndef _DIRENT_H
#define _DIRENT_H

#include "types.h"

#define DT_UNKNOWN       0
#define DT_FIFO          1
#define DT_CHR           2
#define DT_DIR           4
#define DT_BLK           6
#define DT_REG           8      // Обычный файл
#define DT_LNK          10
#define DT_SOCK         12
#define DT_WHT          14

typedef struct PACKED {
    uint64_t    inode;      // номер inode
    uint64_t    offset;     // Номер в директории
    uint16_t    reclen;     // Длина имени
    uint8_t     type;       // Тип
    char        name[];     // Имя
} dirent_t;

#endif