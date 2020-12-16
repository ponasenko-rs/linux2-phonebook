#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/syscalls.h>
#include <linux/types.h>

#define PHONEBOOK_NAME_MAX_LENGTH 32
#define PHONEBOOK_SURNAME_MAX_LENGTH 32
#define PHONEBOOK_PHONE_MAX_LENGTH 32
#define PHONEBOOK_EMAIL_MAX_LENGTH 32

#define PB_ADD 0
#define PB_DEL 1
#define PB_GET 2

#define PB_OK 0
#define PB_FAIL 1

static struct file* file_open(const char* path, int flags, int rights) {
	struct file* filp = NULL;
	mm_segment_t oldfs;
	int err = 0;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	filp = filp_open(path, flags, rights);

	set_fs(oldfs);

	if (IS_ERR(filp)) {
		err = PTR_ERR(filp);
		return NULL;
	}
	return filp;
}

static void file_close(struct file* file) {
	filp_close(file, NULL);
}

static int file_read(struct file* file, unsigned long long offset,
	unsigned char* data, unsigned int size)  {

	mm_segment_t oldfs;
	int ret;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	ret = vfs_read(file, data, size, &offset);

	set_fs(oldfs);

	return ret;
}

static int file_write(struct file* file, unsigned long long offset,
	unsigned char* data, unsigned int size) {

	mm_segment_t oldfs;
	int ret;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	ret = vfs_write(file, data, size, &offset);
	set_fs(oldfs);

	return ret;
}


static DEFINE_MUTEX(lock);

struct user_data {
        char name[PHONEBOOK_NAME_MAX_LENGTH];
        char surname[PHONEBOOK_SURNAME_MAX_LENGTH];
        uint64_t age;
        char phone[PHONEBOOK_PHONE_MAX_LENGTH];
        char email[PHONEBOOK_EMAIL_MAX_LENGTH];
};

static const char* device_path = "/dev/phonebook";

SYSCALL_DEFINE3(get_user, const char*, surname, unsigned int, len, struct user_data*, output_data) {
	struct file* filep; 
	uint8_t request_type = PB_GET;
	uint8_t request_len = len + 1;
	uint8_t response_type;
	uint8_t response_len;
	char* write_buffer = NULL;
	char read_buffer[sizeof(response_type) + sizeof(response_len) + sizeof(*output_data)];

	if (mutex_lock_interruptible(&lock) < 0) {
		return -EINTR;
	}

	printk(KERN_INFO "phonebook: sys_get_user\n");

	filep = file_open(device_path, O_RDWR, 0600);
	if (filep == NULL) {
		printk(KERN_INFO "phonebook: sys_get_user: cant open device\n");
		mutex_unlock(&lock);
		return -1;
	}

	write_buffer = kmalloc(sizeof(request_type) + sizeof(request_len) + len + 1, GFP_KERNEL);
	if (write_buffer == NULL) {
		printk(KERN_INFO "phonebook: sys_get_user: cant allocate buffer\n");
		file_close(filep);
		mutex_unlock(&lock);
		return -1;
	}

	memcpy(write_buffer, &request_type, sizeof(request_type));
	memcpy(write_buffer + sizeof(request_type), &request_len, sizeof(request_len));
	if (copy_from_user(write_buffer + sizeof(request_type) + sizeof(request_len),
		 surname, len)) {

		printk(KERN_INFO "phonebook: sys_get_user: cant get surname\n");
		kfree(write_buffer);
		file_close(filep);
		mutex_unlock(&lock);
		return -1;
	}

	(write_buffer + sizeof(request_type) + sizeof(request_len))[len] = '\0';

	if (file_write(filep, 0, write_buffer, request_type + len + 1) < 0) {
		printk(KERN_INFO "phonebook: sys_get_user: write failed\n");
		kfree(write_buffer);
		file_close(filep);
		mutex_unlock(&lock);
		return -1;
	}

	if (file_read(filep, 0, read_buffer, sizeof(read_buffer)) < 0) {
		printk(KERN_INFO "phonebook: sys_get_user: read failed\n");
		kfree(write_buffer);
		file_close(filep);
		mutex_unlock(&lock);
		return -1;
	} 

	memcpy(&response_type, read_buffer, sizeof(response_type));
	memcpy(&response_len, read_buffer + sizeof(response_type), sizeof(response_len));

	if (response_type == PB_FAIL) {
		printk(KERN_INFO "phonebook: sys_get_user: request failed\n");
		kfree(write_buffer);
		file_close(filep);
		mutex_unlock(&lock);
		return -1;
	}

	if (response_len != sizeof(*output_data)) {
		printk(KERN_INFO "phonebook: sys_get_user: wrong response len\n");
		kfree(write_buffer);
		file_close(filep);
		mutex_unlock(&lock);
		return -1;
	}

	if (copy_to_user(output_data, read_buffer + sizeof(response_type)
		+ sizeof(response_len), sizeof(*output_data))) {

		printk(KERN_INFO "phonebook: sys_get_user: cant copy output\n");
		kfree(write_buffer);
		file_close(filep);
		mutex_unlock(&lock);
		return -1;
	}

	kfree(write_buffer);
	file_close(filep);
	mutex_unlock(&lock);

	return 0;
}

SYSCALL_DEFINE1(add_user, struct user_data*, input_data) {
	struct file* filep; 
	uint8_t request_type = PB_ADD;
	uint8_t response_type;
	uint8_t response_len;
	char* write_buffer = NULL;
	char read_buffer[sizeof(response_type) + sizeof(response_len)];
	

	if (mutex_lock_interruptible(&lock) < 0) {
		return -EINTR;
	}

	printk("phonebook: sys_add_user\n");

	filep = file_open(device_path, O_RDWR, 0600);
	if (filep == NULL) {
		printk(KERN_INFO "phonebook: sys_add_user: cant open device\n");
		mutex_unlock(&lock);
		return -1;
	}

	write_buffer = kmalloc(sizeof(request_type) + sizeof(*input_data), GFP_KERNEL);
	if (write_buffer == NULL) {
		printk(KERN_INFO "phonebook: sys_add_user: cant allocate buffer\n");
		file_close(filep);
		mutex_unlock(&lock);
		return -1;
	}

	memcpy(write_buffer, &request_type, sizeof(request_type));
	if (copy_from_user(write_buffer + sizeof(request_type), input_data, sizeof(*input_data))) {
		printk(KERN_INFO "phonebook: sys_add_user: cant get input_data\n");
		kfree(write_buffer);
		file_close(filep);
		mutex_unlock(&lock);
		return -1;
	}

	if (file_write(filep, 0, write_buffer, request_type + sizeof(*input_data)) < 0) {
		printk(KERN_INFO "phonebook: sys_add_user: write failed\n");
		kfree(write_buffer);
		file_close(filep);
		mutex_unlock(&lock);
		return -1;
	}

	if (file_read(filep, 0, read_buffer, sizeof(read_buffer)) < 0) {
		printk(KERN_INFO "phonebook: sys_add_user: read failed\n");
		kfree(write_buffer);
		file_close(filep);
		mutex_unlock(&lock);
		return -1;
	} 

	memcpy(&response_type, read_buffer, sizeof(response_type));
	memcpy(&response_len, read_buffer + sizeof(response_type), sizeof(response_len));

	if (response_type == PB_FAIL) {
		printk(KERN_INFO "phonebook: sys_add_user: request failed\n");
		kfree(write_buffer);
		file_close(filep);
		mutex_unlock(&lock);
		return -1;
	}

	if (response_len != 0) {
		printk(KERN_INFO "phonebook: sys_add_user: wrong response len\n");
		kfree(write_buffer);
		file_close(filep);
		mutex_unlock(&lock);
		return -1;
	}
	
	kfree(write_buffer);
	file_close(filep);
	mutex_unlock(&lock);

	return 0;
}


SYSCALL_DEFINE2(del_user, const char*, surname, unsigned int, len) {
	struct file* filep; 
	uint8_t request_type = PB_DEL;
	uint8_t request_len = len + 1;
	uint8_t response_type;
	uint8_t response_len;
	char *write_buffer = NULL;
	char read_buffer[sizeof(response_type) + sizeof(response_len)];

	if (mutex_lock_interruptible(&lock) < 0) {
		return -EINTR;
	}

	printk(KERN_INFO "phonebook: sys_del_user\n");

	filep = file_open(device_path, O_RDWR, 0600);
	if (filep == NULL) {
		printk(KERN_INFO "phonebook: sys_del_user: cant open device\n");
		mutex_unlock(&lock);
		return -1;
	}

	write_buffer = kmalloc(sizeof(request_type) + sizeof(request_len) + len + 1, GFP_KERNEL);
	if (write_buffer == NULL) {
		printk(KERN_INFO "phonebook: sys_del_user: cant allocate buffer\n");
		file_close(filep);
		mutex_unlock(&lock);
		return -1;
	}

	memcpy(write_buffer, &request_type, sizeof(request_type));
	memcpy(write_buffer + sizeof(request_type), &request_len, sizeof(request_len));
	if (copy_from_user(write_buffer + sizeof(request_type) + sizeof(request_len),
		 surname, len)) {

		printk(KERN_INFO "phonebook: sys_del_user: cant get surname\n");
		kfree(write_buffer);
		file_close(filep);
		mutex_unlock(&lock);
		return -1;
	}

	(write_buffer + sizeof(request_type) + sizeof(request_len))[len] = '\0';

	if (file_write(filep, 0, write_buffer, request_type + len + 1) < 0) {
		printk(KERN_INFO "phonebook: sys_del_user: write failed\n");
		kfree(write_buffer);
		file_close(filep);
		mutex_unlock(&lock);
		return -1;
	}

	if (file_read(filep, 0, read_buffer, sizeof(read_buffer)) < 0) {
		printk(KERN_INFO "phonebook: sys_del_user: read failed\n");
		kfree(write_buffer);
		file_close(filep);
		mutex_unlock(&lock);
		return -1;
	} 

	memcpy(&response_type, read_buffer, sizeof(response_type));
	memcpy(&response_len, read_buffer + sizeof(response_type), sizeof(response_len));

	if (response_type == PB_FAIL) {
		printk(KERN_INFO "phonebook: sys_del_user: request failed\n");
		kfree(write_buffer);
		file_close(filep);
		mutex_unlock(&lock);
		return -1;
	}

	kfree(write_buffer);
	file_close(filep);
	mutex_unlock(&lock);

	return 0;
}
