#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "kernel_phonebook.h"

MODULE_LICENSE("GPL");

struct cdev cdev;
static dev_t device;
static const char *device_name = "phonebook";

static char answer[128];
static uint32_t answer_len = 0;
static uint8_t last_failed = 0;

static int phonebook_open(struct inode *inode, struct file *file) {
	printk(KERN_INFO "phonebook: open\n");
	return 0;
}

static int phonebook_close(struct inode *inode, struct file *file) {
	printk(KERN_INFO "phonebook: close\n");
	return 0;
}

static ssize_t phonebook_read(struct file *file, char __user *buf, size_t count,
	loff_t *f_pos) {

	uint8_t result;

	printk(KERN_INFO "phonebook: read\n");

	result = (last_failed == 0) ? PB_OK : PB_FAIL;

	if (result == PB_FAIL) {
		if (copy_to_user(buf, &result, sizeof(result))) {
			return -EFAULT;
		}
		return sizeof(result);
	}

	// result == PB_OK
	if (copy_to_user(buf, &result, sizeof(result))) {
		return -EFAULT;	
	}	
	buf += sizeof(result);

	if (copy_to_user(buf, &answer, answer_len)) {
		return -EFAULT;	
	}

	return sizeof(result) + answer_len;
}

// todo: implement
static ssize_t phonebook_write(struct file *file, const char __user *buf, size_t count,
	loff_t *f_pos) {
	uint8_t request;
	ssize_t read_bytes;

	printk(KERN_INFO "phonebook: write\n");

	answer_len = 0;
	last_failed = 0;

	if (copy_from_user(&request, buf, sizeof(request))) {
		last_failed = 1;
		return -EINVAL;
	}

	read_bytes = sizeof(request);

	switch (request) {
		case PB_ADD:
			printk(KERN_INFO "phonebook: write: add\n");
			memcpy(answer, "add\0", 4);
			answer_len = 4;
			break;
		case PB_DEL:
			printk(KERN_INFO "phonebook: write: del\n");
			memcpy(answer, "del\0", 4);
			answer_len = 4;
			break;
		case PB_GET:
			printk(KERN_INFO "phonebook: write: get\n");
			memcpy(answer, "get\0", 4);
			answer_len = 4;
			break;
		default:
			printk(KERN_INFO "phonebook: write: default\n");
			last_failed = 1;
			break;
	}

	return read_bytes;
}

static struct file_operations phonebook_fops = {
	.owner = THIS_MODULE,
	.open = phonebook_open,
	.release = phonebook_close,
	.read = phonebook_read,
	.write = phonebook_write,
};

static int __init phonebook_init(void) {
	printk(KERN_INFO "phonebook: init\n");

	alloc_chrdev_region(&device, 0, 1, device_name);

	cdev_init(&cdev, &phonebook_fops);
	cdev.owner = THIS_MODULE;
	cdev.ops = &phonebook_fops;

	cdev_add(&cdev, device, 1);

	return 0;
}

static void __exit phonebook_exit(void) {
	printk(KERN_INFO "phonebook: exit\n");
	cdev_del(&cdev);
	unregister_chrdev_region(device, 1);
}

module_init(phonebook_init);
module_exit(phonebook_exit);

