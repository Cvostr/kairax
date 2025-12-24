#include "proc/syscalls.h"
#include "proc/process.h"
#include "cpu/cpu_local.h"
#include "fs/vfs/superblock.h"
#include "fs/vfs/vfs.h"

int sys_statfs(const char *path, struct statfs *buf)
{
    int rc;
    struct process* process = cpu_get_current_thread()->process;
    VALIDATE_USER_STRING(process, path)
    VALIDATE_USER_POINTER_PROTECTION(process, buf, sizeof(struct statfs), PAGE_PROTECTION_WRITE_ENABLE)

    struct inode* target_inode = vfs_fopen(process->pwd, path, NULL);
    if (target_inode == NULL)
    {
        return -ENOENT;
    }

    struct superblock *sb = target_inode->sb;
    rc = superblock_stat(sb, buf);

    inode_close(target_inode);
    return rc;
}

int sys_fstatfs(int fd, struct statfs *buf)
{
    int rc;
    struct file* file;
    struct process* process = cpu_get_current_thread()->process;
    VALIDATE_USER_POINTER_PROTECTION(process, buf, sizeof(struct statfs), PAGE_PROTECTION_WRITE_ENABLE)

    file = process_get_file_ex(process, fd, TRUE);

    if (file == NULL) {
        return -ERROR_BAD_FD;
    }

    struct superblock *sb = file->inode->sb;
    rc = superblock_stat(sb, buf);

    file_close(file);
    return rc;
}