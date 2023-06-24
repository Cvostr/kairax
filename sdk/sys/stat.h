typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef unsigned long int ino_t;
typedef long int off_t;
typedef unsigned int mode_t;
typedef unsigned long int nlink_t;

typedef long int time_t;

struct timespec
{
	time_t tv_sec;
  	long int tv_nsec;
};

struct stat {
   // dev_t      st_dev;      /* ID of device containing file */
    ino_t      st_ino;      /* Inode number */
    mode_t     st_mode;     /* File type and mode */
    nlink_t    st_nlink;    /* Number of hard links */
    uid_t      st_uid;      /* User ID of owner */
    gid_t      st_gid;      /* Group ID of owner */
    //dev_t      st_rdev;     /* Device ID (if special file) */
    off_t      st_size;     /* Total size, in bytes */
    //blksize_t  st_blksize;  /* Block size for filesystem I/O */
    //blkcnt_t   st_blocks;   /* Number of 512 B blocks allocated */

    struct timespec  st_atim;  
    struct timespec  st_mtim;
    struct timespec  st_ctim;

    #define st_atime  st_atim.tv_sec
    #define st_mtine  st_mtim.tv_sec
    #define st_ctime  st_ctim.tv_sec
};