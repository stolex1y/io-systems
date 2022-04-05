#include <linux/blk-mq.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/errno.h>
 
#include "ram_device.h"
#include "partition.h"
 
#define DEV_FIRST_MINOR 0
#define DEV_MINOR_CNT 8
#define DEV_NAME "ramdev"

#define MY_MODULE DEV_NAME " driver: "
 
static u_int dev_major = 0;
struct ram_device *dev;
 
static int dev_open(struct block_device *bdev, fmode_t mode) {
    pr_info(MY_MODULE "Device is opened\n");	
    return 0;
}
 
static void dev_close(struct gendisk *gd, fmode_t mode) {
    pr_info(MY_MODULE "Device is closed\n");
}
 
/*
 * Actual Data transfer
 */
static void dev_transfer(struct ram_device *dev, unsigned long start_sector, 
							unsigned long sector_cnt, char *buffer, int write) { 
	if ((start_sector + sector_cnt) > dev->size) {
		pr_notice(MY_MODULE "Beyond-end write (%ld %ld)\n", start_sector, sector_cnt);
		return;
	}
	if (write)
		ram_device_write(dev, start_sector, buffer, sector_cnt);
	else
		ram_device_read(dev, start_sector, buffer, sector_cnt);

} 


 
/*static int dev_request(struct ram_device *dev, unsigned long start_sector, 
							unsigned long sector_cnt, char *buffer, int write) { 
    //int dir = rq_data_dir(req);
    //sector_t start_sector = blk_rq_pos(req);
    //unsigned int sector_cnt = blk_rq_sectors(req);
 
    struct bio_vec *bv;
    struct req_iterator iter;
 
    sector_t sector_offset;
    unsigned int sectors;
    u8 *buffer;
 
    int ret = 0;
 
    //printk(KERN_DEBUG MY_MODULE "Dir:%d; Sec:%lld; Cnt:%d\n", dir, start_sector, sector_cnt);
 
    sector_offset = 0;
    rq_for_each_segment(bv, req, iter)
    {
        buffer = page_address(bv->bv_page) + bv->bv_offset;
        if (bv->bv_len % dev_SECTOR_SIZE != 0)
        {
            pr_err(MY_MODULE "Should never happen: "
                "bio size (%d) is not a multiple of dev_SECTOR_SIZE (%d).\n"
                "This may lead to data truncation.\n",
                bv->bv_len, dev_SECTOR_SIZE);
            ret = -EIO;
        }
        sectors = bv->bv_len / dev_SECTOR_SIZE;
        printk(KERN_DEBUG MY_MODULE "Sector Offset: %lld; Buffer: %p; Length: %d sectors\n",
            sector_offset, buffer, sectors);
        if (dir == WRITE) // Write to the device
        {
            ramdevice_write(start_sector + sector_offset, buffer, sectors);
        }
        else // Read from the device
        {
            ramdevice_read(start_sector + sector_offset, buffer, sectors);
        }
        sector_offset += sectors;
    }
    if (sector_offset != sector_cnt)
    {
        pr_err(MY_MODULE "bio info doesn't match with the request info");
        ret = -EIO;
    }
 
    return ret;
}*/
 
/*
 * Represents a block I/O request for us to execute
 */
/*static void dev_request(struct request_queue *q) {
    struct request *req;
 
    // Gets the current request from the dispatch queue 
    while ((req = blk_fetch_request(q)) != NULL) {
		struct ram_device *dev = req->rq_disk->private_data;
        dev_transfer(dev, req->sector, req->nr_sectors, req->buffer, rq_data_dir(req));
    }
}
*/ 
static blk_status_t dev_request(struct blk_mq_hw_ctx *hctx, const struct blk_mq_queue_data* bd)   /* For blk-mq */
{
	struct request *req = bd->rq;
	struct ram_device *dev = req->rq_disk->private_data;
	struct bio_vec bvec;
	struct req_iterator iter;
	sector_t pos_sector = blk_rq_pos(req);
	void *buffer;
	blk_status_t ret;

	blk_mq_start_request (req);

	if (blk_rq_is_passthrough(req)) {
		printk (KERN_NOTICE "Skip non-fs request\n");
		ret = BLK_STS_IOERR;  //-EIO
		blk_mq_end_request(req, ret);
		return ret;
	}
	
	rq_for_each_segment(bvec, req, iter) {
		size_t num_sector = blk_rq_cur_sectors(req);
		buffer = page_address(bvec.bv_page) + bvec.bv_offset;
		dev_transfer(dev, pos_sector, num_sector,
				buffer, rq_data_dir(req) == WRITE);
		pos_sector += num_sector;
	}
	ret = BLK_STS_OK;
	blk_mq_end_request (req, ret);
	return ret;
}

/*
 * These are the file operations that performed on the ram block device
 */
static struct block_device_operations dev_ops =
{
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_close,
};

static struct blk_mq_ops mq_ops = {
    .queue_rq = dev_request,
};
 
/*
 * This is the registration and initialization section of the ram block device
 * driver
 */
static int __init dev_init(void)
{
	int ret; 
	if ((ret = ram_device_init(&dev)) < 0) {
		pr_err(MY_MODULE "ram_device_init failure\n");
		return ret;
	};
 
    /* Get Registered */
    dev_major = register_blkdev(0, DEV_NAME);
    if (dev_major <= 0)
    {
        pr_err(MY_MODULE "Unable to get Major Number\n");
        ram_device_cleanup(dev);
        return -EBUSY;
    }
 
    /* Get a request queue (here queue is created) */
    spin_lock_init(&dev->lock);
	//dev->queue = blk_init_queue(dev_request, &dev->lock);
	dev->queue = blk_mq_init_sq_queue(&dev->tag_set, &mq_ops, 128, BLK_MQ_F_SHOULD_MERGE);
    if (dev->queue == NULL) {
		pr_err(MY_MODULE "Failed init queue\n");
		unregister_blkdev(dev_major, DEV_NAME);
		ram_device_cleanup(dev);
		return -ENOMEM;
	}
	dev->queue->queuedata = dev;
 
    /*
     * Add the gendisk structure
     * By using this memory allocation is involved,
     * the minor number we need to pass bcz the device
     * will support this much partitions
     */
    dev->gd = alloc_disk(DEV_MINOR_CNT);
    if (!dev->gd)
    {
        pr_err(MY_MODULE "alloc_disk failure\n");
        blk_cleanup_queue(dev->queue);
        unregister_blkdev(dev_major, DEV_NAME);
        ram_device_cleanup(dev);
        return -ENOMEM;
    }
	
	dev->gd->major = dev_major;
	dev->gd->first_minor = 0;
	dev->gd->fops = &dev_ops;
	dev->gd->queue = dev->queue;
	dev->gd->private_data = dev;
    sprintf(dev->gd->disk_name, DEV_NAME);
    set_capacity(dev->gd, dev->size);
 
    /* Adding the disk to the system */
    add_disk(dev->gd);
    pr_info(MY_MODULE "Ram Block driver initialised (%d sectors; %d bytes)\n",
        dev->size, dev->size * RAM_DEV_SECTOR_SIZE);
 
    return 0;
}
/*
 * This is the unregistration and uninitialization section of the ram block
 * device driver
 */
static void __exit dev_cleanup(void) {
	if (dev->gd) {
		del_gendisk(dev->gd);
		put_disk(dev->gd);
	}
	if (dev->queue) {
		blk_cleanup_queue(dev->queue);
	}
    unregister_blkdev(dev_major, "rb");
    ram_device_cleanup(dev);
}
 
module_init(dev_init);
module_exit(dev_cleanup);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alexey Filimonov");
MODULE_DESCRIPTION("Ram Block Device Driver");