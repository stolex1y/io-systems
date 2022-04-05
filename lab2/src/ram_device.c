#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/blk-mq.h>
 
#include "ram_device.h"
#include "partition.h" 
 
int ram_device_init(struct ram_device **dev) {
	*dev = vmalloc(sizeof(struct ram_device));
	if (*dev == NULL)
		return -ENOMEM;
	(*dev)->size = RAM_DEVICE_SIZE_SECTORS;
    (*dev)->data = vmalloc(RAM_DEVICE_SIZE_SECTORS * RAM_DEV_SECTOR_SIZE);
    if ((*dev)->data == NULL)
        return -ENOMEM;
    /* Setup its partition table */
    copy_mbr_n_br((*dev)->data);
    return RAM_DEVICE_SIZE_SECTORS;
}
 
void ram_device_cleanup(struct ram_device *dev) {
	if (dev) {
		vfree(dev->data);
		vfree(dev);
	}
}
 
void ram_device_write(struct ram_device *dev, sector_t sector_off, u8 *buffer, unsigned int sectors) {
	if (dev)
		memcpy(dev->data + sector_off * RAM_DEV_SECTOR_SIZE, buffer, sectors * RAM_DEV_SECTOR_SIZE);
}
void ram_device_read(struct ram_device *dev, sector_t sector_off, u8 *buffer, unsigned int sectors) {
	if (dev)
		memcpy(buffer, dev->data + sector_off * RAM_DEV_SECTOR_SIZE, sectors * RAM_DEV_SECTOR_SIZE);
}