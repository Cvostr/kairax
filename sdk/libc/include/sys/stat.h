#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#include "types.h"
#include "time.h"

/* File types.  */
#define	S_IFMT	    0170000	/* These bits determine file type.  */
#define	S_IFDIR	    0040000	/* Directory.  */
#define	S_IFCHR	    0020000	/* Character device.  */
#define	S_IFBLK	    0060000	/* Block device.  */
#define	S_IFREG	    0100000	/* Regular file.  */
#define	S_IFIFO	    0010000	/* FIFO.  */
#define	S_IFLNK	    0120000	/* Symbolic link.  */
#define	S_IFSOCK	0140000	/* Socket.  */

/* Check macros */
#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)

#define S_IRWXU	    00700
#define S_IRUSR	    00400	
#define S_IWUSR	    00200	
#define S_IXUSR	    00100
#define S_IRWXG	    00070	
#define S_IRGRP	    00040	
#define S_IWGRP	    00020	
#define S_IXGRP	    00010	
#define S_IRWXO	    00007
#define S_IROTH	    00004	
#define S_IWOTH	    00002	
#define S_IXOTH	    00001	

#define UTIME_NOW	1000000001
#define UTIME_OMIT	1000000002

struct stat {
    dev_t      st_dev;      /* ID of device containing file */
    ino_t      st_ino;      /* Inode number */
    mode_t     st_mode;     /* File type and mode */
    nlink_t    st_nlink;    /* Number of hard links */
    uid_t      st_uid;      /* User ID of owner */
    gid_t      st_gid;      /* Group ID of owner */
    dev_t      st_rdev;     /* Device ID (if special file) */
    off_t      st_size;     /* Total size, in bytes */
    blksize_t  st_blksize;  /* Block size for filesystem I/O */
    blkcnt_t   st_blocks;   /* Number of 512 B blocks allocated */

    struct timespec  st_atim;  
    struct timespec  st_mtim;
    struct timespec  st_ctim;

    #define st_atime  st_atim.tv_sec
    #define st_mtine  st_mtim.tv_sec
    #define st_ctime  st_ctim.tv_sec
};

int chmod(const char* filepath, mode_t mode);
int fchmod(int fd, mode_t mode);
int fchmodat(int dirfd, const char* filepath, mode_t mode, int flags);

int stat(const char* filepath, struct stat* st);
int fstat(int fd, struct stat* st);
int fstatat(int dirfd, const char* filepath, struct stat* st, int flags);

int mkdir(const char* dirpath, int mode);

int mknod(const char *path, mode_t mode, dev_t dev);
int mknodat (int dirfd, const char *path, mode_t mode, dev_t dev);

int utimensat(int fd, const char* path, const struct timespec times[2], int flag);

#endif