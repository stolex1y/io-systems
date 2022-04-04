#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h> 
#include <linux/uaccess.h> 
#include <linux/types.h>
#include <linux/slab.h>

#include "vector.h"
#include "numbers_parser.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0) 
	#define HAVE_PROC_OPS 
#endif 

#define MY_MODULE "chdev driver: "
#define DEV_NAME "chdev"
#define DEV_COUNT 1
#define PROC_FILE_NAME "var3"
#define USER_STR_LEN 1024

static dev_t dev_num;
static struct cdev c_dev; 
static struct class *dev_class;
static struct proc_dir_entry *proc_file; 

static struct vector *numbers_vector;
static char *numbers_buffer;
static size_t numbers_buf_size = 1024;
static size_t numbers_buf_len = 0;
static char *user_str;

static void update_numbers_str(void) {
	if (vector_count(numbers_vector) * VECTOR_VALUE_STR_LEN + 2 > numbers_buf_size) {
		kfree(numbers_buffer);
		numbers_buf_size = vector_count(numbers_vector) * VECTOR_VALUE_STR_LEN * 2;
		numbers_buffer = kmalloc(numbers_buf_size, GFP_KERNEL);
	}
	vector_print(numbers_buffer, numbers_buf_size, numbers_vector);
	numbers_buf_len = strlen(numbers_buffer);
}

static ssize_t proc_file_read(struct file *f, char __user *buf, size_t len, loff_t *off) {
	len = min(len, numbers_buf_len);
    if (*off >= numbers_buf_len) {
		return 0;
	}
	if (copy_to_user(buf, numbers_buffer, len)) {
		return -EFAULT;
	}
	
	*off += len; 
	return len; 
} 

#ifdef HAVE_PROC_OPS 
	static const struct proc_ops proc_file_fops = { 
		.proc_read = proc_file_read
	}; 
#else 
	static const struct file_operations proc_file_fops = {
		.read = proc_file_read
	};
#endif

static ssize_t dev_read(struct file *f, char __user *buf, size_t len, loff_t *off) {	
	if (vector_count(numbers_vector) == 0 || *off >= numbers_buf_len) {
		return 0;
	}
	
	len = min(len, numbers_buf_len);
	
	/*if (copy_to_user(buf, numbers_buffer, len)) {
		return -EFAULT;
	}*/
	
	pr_info(MY_MODULE "%s\n", numbers_buffer);
	*off += len;
	return len;
}

static ssize_t dev_write(struct file *f, const char __user *buf, size_t len, loff_t *off) {
	
	len = min(len, (size_t) USER_STR_LEN);
	if (len == 0) {
		return 0;
	}
	
	if (copy_from_user(user_str, buf, len)) {
		return -EFAULT;
	}
	
	vector_add(numbers_vector, parse_string(user_str, len));
	update_numbers_str();
	
	return len;
}

static struct file_operations dev_fops = {
	.owner = THIS_MODULE,
	.read = dev_read,
	.write = dev_write
};

static void delete_device(void) {
	if (dev_class) {
		device_destroy(dev_class, dev_num);
		class_destroy(dev_class);
		unregister_chrdev_region(dev_num, DEV_COUNT);
	}
}

static int __init ch_dev_init(void) {
    pr_info(MY_MODULE "init\n");
	
	proc_file = proc_create(PROC_FILE_NAME, 0444, NULL, &proc_file_fops);
	if (!proc_file) {
		pr_err(MY_MODULE "proc file create error\n");
		return -1;
	}
	
    if (alloc_chrdev_region(&dev_num, 0, DEV_COUNT, DEV_NAME) < 0) {
		pr_err(MY_MODULE "alloc error\n");
		return -1;
	}
	dev_class = class_create(THIS_MODULE, DEV_NAME);
    if (!dev_class) {
		pr_err(MY_MODULE "class create error\n");
		unregister_chrdev_region(dev_num, DEV_COUNT);
		return -1;
	}
    if (!device_create(dev_class, NULL, dev_num, NULL, DEV_NAME)) {
		pr_err(MY_MODULE "device create error\n");
		class_destroy(dev_class);
		unregister_chrdev_region(dev_num, DEV_COUNT);
		return -1;
	}
    cdev_init(&c_dev, &dev_fops);
    if (cdev_add(&c_dev, dev_num, 1) < 0) {
		pr_err(MY_MODULE "device add error\n");
		delete_device();
		return -1;
	}
	
	
	numbers_vector = vector_new(10);
	numbers_buffer = kmalloc(numbers_buf_size, GFP_KERNEL);
	user_str = kmalloc(USER_STR_LEN + 1, GFP_KERNEL);
	if (!numbers_vector || !numbers_buffer || !user_str) {
		pr_err(MY_MODULE "couldn't allocate buffer\n");
		delete_device();
		return -ENOMEM;
	}
	numbers_buffer[numbers_buf_size] = 0;
	user_str[USER_STR_LEN] = 0;
	
    return 0;
}
 
static void __exit ch_dev_exit(void) {
    cdev_del(&c_dev);
    device_destroy(dev_class, dev_num);
    class_destroy(dev_class);
    unregister_chrdev_region(dev_num, 1);
	
	proc_remove(proc_file);
	
	vector_destroy(&numbers_vector);
	kfree(numbers_buffer);
	kfree(user_str);
    pr_info(MY_MODULE "exit\n");
}
 
module_init(ch_dev_init);
module_exit(ch_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alexey Filimonov");