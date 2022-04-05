#ifndef PARTITION_H
#define PARTITION_H

#include <linux/types.h>

#define RAM_DEV_SECTOR_SIZE 512
#define PART1_SIZE_SEC (30 * 1024 * 1024 / RAM_DEV_SECTOR_SIZE) //30 MB
#define PART2_EX_SIZE_SEC (20 * 1024 * 1024 / RAM_DEV_SECTOR_SIZE) //20 MB
#define PART2_EX_1_SIZE_SEC (10 * 1024 * 1024 / RAM_DEV_SECTOR_SIZE) //10 MB
#define PART2_EX_2_SIZE_SEC PART2_EX_1_SIZE_SEC

#define RAM_DEVICE_SIZE_SECTORS (PART1_SIZE_SEC + PART2_EX_SIZE_SEC)

void copy_mbr_n_br(u8 *disk);

#endif