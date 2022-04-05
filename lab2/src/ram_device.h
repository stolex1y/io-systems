#ifndef RAM_DEVICE_H
#define RAM_DEVICE_H

#include <linux/blk-mq.h>
#include <linux/types.h>

struct ram_device {
    /* Size is the size of the device (in sectors) */
    unsigned int size;
	/* Array where the disk stores its data */
	u8 *data;
    /* For exclusive access to our request queue */
    spinlock_t lock;
    /* Our request queue */
    struct request_queue *queue;
    /* This is kernel's representation of an individual disk device */
    struct gendisk *gd;
	struct blk_mq_tag_set tag_set;
};

int ram_device_init(struct ram_device **dev);
void ram_device_cleanup(struct ram_device *dev);
void ram_device_write(struct ram_device *dev, sector_t sector_off, u8 *buffer, unsigned int sectors);
void ram_device_read(struct ram_device *dev, sector_t sector_off, u8 *buffer, unsigned int sectors);

#endif