#include <linux/string.h>
#include <linux/kernel.h>
 
#include "partition.h"
 
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))
 
#define MBR_SIZE RAM_DEV_SECTOR_SIZE
#define MBR_DISK_SIGNATURE_OFFSET 440
#define MBR_DISK_SIGNATURE_SIZE 4
#define PARTITION_TABLE_OFFSET 446
#define PARTITION_ENTRY_SIZE 16 // sizeof(PartEntry)
#define PARTITION_TABLE_SIZE 64 // sizeof(PartTable)
#define MBR_SIGNATURE_OFFSET 510
#define MBR_SIGNATURE_SIZE 2
#define MBR_SIGNATURE 0xAA55
#define BR_SIZE RAM_DEV_SECTOR_SIZE
#define BR_SIGNATURE_OFFSET 510
#define BR_SIGNATURE_SIZE 2
#define BR_SIGNATURE 0xAA55

#define HEADS_PER_CYL 255
#define SEC_PER_HEAD 63
#define SEC_PER_CYL (HEADS_PER_CYL * SEC_PER_HEAD)
#define SEC_TO_HEADS(SEC) ((SEC) / SEC_PER_HEAD + 1)
#define SEC_TO_CYL(SEC) (SEC_TO_HEADS(SEC) / HEADS_PER_CYL + 1)

typedef struct
{
    unsigned char boot_type; // 0x00 - Inactive; 0x80 - Active (Bootable)
    unsigned char start_head;
    unsigned char start_sec:6;
    unsigned char start_cyl_hi:2;
    unsigned char start_cyl;
    unsigned char part_type;
    unsigned char end_head;
    unsigned char end_sec:6;
    unsigned char end_cyl_hi:2;
    unsigned char end_cyl;
    unsigned long abs_start_sec;
    unsigned long sec_in_part;
} PartEntry;
 
typedef PartEntry PartTable[4];
 
static PartTable def_part_table =
{
    {
        boot_type: 0x00,
        start_head: 0x00,
        start_sec: 0x02,
        start_cyl: 0x00,
		start_cyl_hi: 0x00,
        part_type: 0x83,
        end_head: SEC_TO_HEADS(PART1_SIZE_SEC) % HEADS_PER_CYL - 1, // part1 = 60 * 1024
        end_sec: PART1_SIZE_SEC % SEC_PER_HEAD, // 15
		end_cyl_hi: 0x00,
		end_cyl: SEC_TO_CYL(PART1_SIZE_SEC) - 1, // 2
        abs_start_sec: 0x01,
        sec_in_part: PART1_SIZE_SEC - 1
    },
    {
        boot_type: 0x00,
        start_head: 0x00,
        start_sec: 0x01,
		start_cyl_hi: 0x00,
        start_cyl: SEC_TO_CYL(PART1_SIZE_SEC), // 3
        part_type: 0x05, // extended partition
        end_head: SEC_TO_HEADS(PART2_EX_SIZE_SEC) % HEADS_PER_CYL - 1,
        end_sec: PART2_EX_SIZE_SEC % SEC_PER_HEAD,
		end_cyl_hi: 0x00,
        end_cyl: SEC_TO_CYL(PART2_EX_SIZE_SEC) - 1,
        abs_start_sec: PART1_SIZE_SEC,
        sec_in_part: PART2_EX_SIZE_SEC
    },
    {},
    {}
};

static const unsigned int def_log_part_br_cyl[] = 
{
	SEC_TO_CYL(PART1_SIZE_SEC), 
	SEC_TO_CYL(PART1_SIZE_SEC) + SEC_TO_CYL(PART2_EX_1_SIZE_SEC)
};

static const PartTable def_log_part_table[] =
{
    {
        {
            boot_type: 0x00,
            start_head: 0x00,
            start_sec: 0x02,
			start_cyl_hi: 0x00,
            start_cyl: def_log_part_br_cyl[0],
            part_type: 0x83,
            end_head: SEC_TO_HEADS(PART2_EX_1_SIZE_SEC) % HEADS_PER_CYL - 1,
            end_sec: PART2_EX_1_SIZE_SEC % SEC_PER_HEAD,
			end_cyl_hi: 0x00,
            end_cyl: def_log_part_br_cyl[1] - 1,
            abs_start_sec: 0x01,
            sec_in_part: PART2_EX_1_SIZE_SEC - 1
        },
        {
            boot_type: 0x00,
            start_head: 0x00,
            start_sec: 0x01,
			start_cyl_hi: 0x00,
            start_cyl: def_log_part_br_cyl[1],
            part_type: 0x05,
            end_head: SEC_TO_HEADS(PART2_EX_2_SIZE_SEC) % HEADS_PER_CYL - 1,
            end_sec: PART2_EX_2_SIZE_SEC % SEC_PER_HEAD,
			end_cyl_hi: 0x00,
            end_cyl: def_log_part_br_cyl[1] + SEC_TO_CYL(PART2_EX_2_SIZE_SEC) - 1,
            abs_start_sec: PART2_EX_1_SIZE_SEC,
            sec_in_part: PART2_EX_2_SIZE_SEC
        },
    },
    {
        {
            boot_type: 0x00,
            start_head: 0x00,
            start_sec: 0x02,
			start_cyl_hi: 0x00,
            start_cyl: def_log_part_br_cyl[1],
            part_type: 0x83,
            end_head: SEC_TO_HEADS(PART2_EX_2_SIZE_SEC) % HEADS_PER_CYL - 1,
            end_sec: PART2_EX_2_SIZE_SEC % SEC_PER_HEAD,
			end_cyl_hi: 0x00,
            end_cyl: def_log_part_br_cyl[1] + SEC_TO_CYL(PART2_EX_2_SIZE_SEC) - 1,
            abs_start_sec: 0x01,
            sec_in_part: PART2_EX_2_SIZE_SEC - 1
        },
    }
};
 
static void copy_mbr(u8 *disk) {
    memset(disk, 0x0, MBR_SIZE);
    *(unsigned long *)(disk + MBR_DISK_SIGNATURE_OFFSET) = 0x36E5756D;
    memcpy(disk + PARTITION_TABLE_OFFSET, &def_part_table, PARTITION_TABLE_SIZE);
    *(unsigned short *)(disk + MBR_SIGNATURE_OFFSET) = MBR_SIGNATURE;
}

static void copy_br(u8 *disk, int start_cylinder, const PartTable *part_table) {
    disk += (start_cylinder * SEC_PER_CYL * RAM_DEV_SECTOR_SIZE);
    memset(disk, 0x0, BR_SIZE);
    memcpy(disk + PARTITION_TABLE_OFFSET, part_table,
        PARTITION_TABLE_SIZE);
    *(unsigned short *)(disk + BR_SIGNATURE_OFFSET) = BR_SIGNATURE;
}

void copy_mbr_n_br(u8 *disk) {
    int i;
	pr_info("%d\n", def_part_table[0].end_sec);
 
    copy_mbr(disk);
    for (i = 0; i < ARRAY_SIZE(def_log_part_table); i++) {
        copy_br(disk, def_log_part_br_cyl[i], &def_log_part_table[i]);
    }
}