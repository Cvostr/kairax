#ifndef _FAT_H
#define _FAT_H

#include "types.h"
#include "../vfs/vfs.h"
#include "../vfs/file.h"
#include "../vfs/superblock.h"
#include "../vfs/dentry.h"

struct fat_ext_pbp {
	uint8_t drive_number;
	uint8_t nt_flags;
	uint8_t boot_signature;
	uint32_t volume_id;
	uint8_t volume_label[11];
	uint8_t system_id_string[8];
	uint8_t boot_code[448];
} PACKED;

struct fat_ext_pbp_32 {
	uint32_t fat_size;
	uint16_t extended_flags;
	uint16_t filesystem_version;
	uint32_t root_cluster;
	uint16_t filesystem_info; // Fat32 FSinfo
	uint16_t backup_boot_sector;
	uint8_t reserved0[12];
	uint8_t drive_number;
	uint8_t nt_flags;
	uint8_t boot_signature;
	uint32_t volume_id;
	uint8_t volume_label[11];
	uint8_t system_id_string[8];
	uint8_t boot_code[420];
} PACKED;

struct fat_bpb {
	uint8_t jump_op[3];
	uint8_t oem_name[8];
	uint16_t bytes_per_sector;
	uint8_t sectors_per_cluster;
	uint16_t reserved_sector_count;
	uint8_t fats_count;
	uint16_t root_entry_count;
	uint16_t total_sectors16;
	uint8_t media_type;
	uint16_t fat_size16;
	uint16_t sectors_per_track;
	uint16_t number_of_heads;
	uint32_t hidden_sector_count;
	uint32_t total_sectors32;
	union
	{
		struct fat_ext_pbp ext;
		struct fat_ext_pbp_32 ext_32;
	};

	uint16_t boot_part_signature;
} PACKED;

struct fat_fsinfo32 {
	uint32_t 	lead_signature;
	uint8_t		rsvd[480];
	uint32_t 	signature;
	uint32_t	last_free_cluster_count; // Contains the last known free cluster count on the volume
	uint32_t 	look_from; // Indicates the cluster number at which the filesystem driver should start looking for available clusters.
	uint8_t		rsvd1[12];
	uint32_t	trail_signature;
} PACKED;

struct fat_date {
	uint16_t day   : 5;
	uint16_t month : 4;
	uint16_t year  : 7;
};

struct fat_time {
	uint16_t second : 5;
	uint16_t minute : 6;
	uint16_t hour   : 5;
};

struct fat_direntry {
	uint8_t		name[11];
	uint8_t		attr;
	uint8_t		nt_reserved;
	uint8_t		creat_time_hundredths;
	struct fat_time creat_time;
	struct fat_date creat_date;
	struct fat_date last_access_date;
	uint16_t	first_cluster_hi;
	struct fat_time write_time;
	struct fat_date write_date;
	uint16_t	first_cluster_lo;
	uint32_t	file_size;
} PACKED;

struct fat_lfn {
	uint8_t		order;
	uint16_t	name_0[5];
	uint8_t		attr;
	uint8_t		type;
	uint8_t		checksum;
	uint16_t	name_1[6];
	uint16_t 	zero;
	uint16_t	name_2[2];
} PACKED;

struct fat_instance {
    struct superblock*  vfs_sb;
	struct inode*		vfs_root_inode;
    drive_partition_t*  partition;
    
    struct fat_bpb*     bpb;
	struct fat_fsinfo32* fsinfo32;
	uint32_t			fat_buffer_size;	// Размер буфера для чтения FAT
	uint16_t 			bytes_per_sector;
	uint32_t			sectors_count;
	uint32_t 			fat_size;
	uint32_t			root_dir_sector;
	uint32_t			root_dir_sectors;
	uint32_t 			first_fat_sector;
	uint32_t			first_data_sector;
	uint32_t			data_sectors;
	uint32_t 			total_clusters;
	int 				fs_type;
};

#define FSINFO32_SIGNATURE				0x41615252
#define FSINFO32_TRAIL_SIGNATURE		0xAA550000
#define BOOTABLE_PARTITION_SIGNATURE	0xAA55

#define FAT_DIRENTRY_SZ		32
#define FAT_LFN_MAX_SZ		256

#define FAT_EOC		0x0FFFFFF8
#define EXFAT_EOC	0xFFFFFFF8

#define FS_FAT12 	1
#define FS_FAT16	2
#define FS_FAT32	3
#define FS_EXFAT	4

#define FILE_ATTR_RDONLY	0x01
#define FILE_ATTR_HIDDEN	0x02
#define FILE_ATTR_SYSTEM	0x04
#define FILE_ATTR_VOLUME_ID	0x08
#define FILE_ATTR_DIRECTORY	0x10
#define FILE_ATTR_ARCHIVE	0x20
#define FILE_ATTR_LFN		(FILE_ATTR_RDONLY | FILE_ATTR_HIDDEN | FILE_ATTR_SYSTEM | FILE_ATTR_VOLUME_ID)

#define FILE_NTRES_NAME_LOWERCASE	0x8
#define FILE_NTRES_EXT_LOWERCASE	0x10

#define FAT_DIRENTRY_GET_CLUSTER(x) ((((uint32_t) x.first_cluster_hi) << 16) | x.first_cluster_lo)

// Функции для получения номера следующего кластера в цепочке
uint32_t fat_get_next_cluster_ex(struct fat_instance* inst, uint32_t cluster, char* fat_buffer);
uint32_t fat_get_next_cluster(struct fat_instance* inst, uint32_t cluster);

// Вызывается VFS при монтировании
struct inode* fat_mount(drive_partition_t* drive, struct superblock* sb);

// Вызывается при размонтировании
int fat_unmount(struct superblock* sb);

struct inode* fat_read_node(struct superblock* sb, uint64_t ino_num);
uint64_t fat_find_dentry(struct superblock* sb, struct inode* parent_inode, const char *name, int* type);
int fat_statfs(struct superblock *sb, struct statfs* stat);

struct dirent* fat_file_readdir(struct file* dir, uint32_t index);

ssize_t fat_file_read(struct file* file, char* buffer, size_t count, loff_t offset);

#endif