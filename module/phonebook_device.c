#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "kernel_phonebook.h"
#include "phonebook_store.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roman Ponasenko");
MODULE_VERSION("0.1");

struct cdev cdev;
static dev_t device;
static const char *device_name = "phonebook";

static char answer[256];
static uint8_t answer_len = 0;
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

	if (copy_to_user(buf, &answer_len, sizeof(answer_len))) {
		return -EFAULT;	
	}	
	buf += sizeof(answer_len);

	if (copy_to_user(buf, &answer, answer_len)) {
		return -EFAULT;	
	}

	return sizeof(result) + sizeof(answer_len) + answer_len;
}

static ssize_t phonebook_write(struct file *file, const char __user *buf, size_t count,
	loff_t *f_pos) {
	uint8_t request;
	ssize_t read_bytes = 0;

	printk(KERN_INFO "phonebook: write\n");

	answer_len = 0;
	last_failed = 0;

	if (copy_from_user(&request, buf, sizeof(request))) {
		printk(KERN_INFO "phonebook: write: cant get request type\n");

		last_failed = 1;
		return -EINVAL;
	}

	read_bytes = sizeof(request);

	switch (request) {
		case PB_ADD: {
			struct pb_user_data data;

			printk(KERN_INFO "phonebook: write: add\n");

			if (copy_from_user(&data, buf + read_bytes, sizeof(struct pb_user_data))) {
				printk(KERN_INFO "phonebook: write: add: cant get user data\n");

				last_failed = 1;
				return -EINVAL;
			}

			read_bytes += sizeof(struct pb_user_data);

			if (pb_add(&data)) {
				printk(KERN_INFO "phonebook: write: add: cant add user\n");
				
				return -EINVAL;
			}

			break;
		}

		case PB_DEL: {
			uint8_t surname_len = 0;
			char* surname = NULL;

			printk(KERN_INFO "phonebook: write: del\n");

			if (copy_from_user(&surname_len, buf + read_bytes, sizeof(uint8_t))) {
				printk(KERN_INFO "phonebook: write: del: cant get surname length\n");
				
				last_failed = 1;
				return -EINVAL;
			}

			read_bytes += sizeof(uint8_t);

			surname = kmalloc(surname_len + 1, GFP_KERNEL);
			if (surname == NULL) {
				printk(KERN_INFO "phonebook: write: del: cant allocate surname buffer\n");
				
				last_failed = 1;
				return -EINVAL;
			}

			if (copy_from_user(surname, buf + read_bytes, surname_len)) {
				printk(KERN_INFO "phonebook: write: del: cant get surname\n");
				
				kfree(surname);
				last_failed = 1;
				return -EINVAL;
			}
			
			read_bytes += surname_len;

			surname[surname_len] = '\0';

			if (pb_del(surname, strlen(surname))) {
				printk(KERN_INFO "phonebook: write: del: cant del user\n");
				
				kfree(surname);
				last_failed = 1;
				return -EINVAL;
			}

			kfree(surname);

			break;
		}

		case PB_GET: {
			uint8_t surname_len = 0;
			char* surname = NULL;
			struct pb_user_data data;
			
			printk(KERN_INFO "phonebook: write: get\n");

			if (copy_from_user(&surname_len, buf + read_bytes, sizeof(uint8_t))) {
				printk(KERN_INFO "phonebook: write: get: cant get surname length\n");
				
				last_failed = 1;
				return -EINVAL;
			}

			read_bytes += sizeof(uint8_t);

			surname = kmalloc(surname_len + 1, GFP_KERNEL);
			if (surname == NULL) {
				printk(KERN_INFO "phonebook: write: get: cant allocate surname buffer\n");
				
				last_failed = 1;
				return -EINVAL;
			}

			if (copy_from_user(surname, buf + read_bytes, surname_len)) {
				printk(KERN_INFO "phonebook: write: get: cant get surname\n");
				
				kfree(surname);
				last_failed = 1;
				return -EINVAL;
			}
			
			read_bytes += surname_len;

			surname[surname_len] = '\0';

			if (pb_get(surname, strlen(surname), &data)) {
				printk(KERN_INFO "phonebook: write: get: cant get user\n");
				
				kfree(surname);
				last_failed = 1;
				return -EINVAL;
			}

			answer_len = sizeof(struct pb_user_data);
			memcpy(answer, &data, sizeof(struct pb_user_data));

			kfree(surname);
			
			break;
		}

		default: {
			printk(KERN_INFO "phonebook: write: default\n");
			last_failed = 1;
			
			return -EINVAL;
		
			break;
		}

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

