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
	uint16_t filesystem_info;
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
} PACKED;

struct fat_instance {
    struct superblock*  vfs_sb;
    drive_partition_t*  partition;
    
    struct fat_bpb*     bpb;
};

// Вызывается VFS при монтировании
struct inode* fat_mount(drive_partition_t* drive, struct superblock* sb);

// Вызывается при размонтировании
int fat_unmount(struct superblock* sb);

#endif