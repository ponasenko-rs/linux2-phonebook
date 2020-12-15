#ifndef KERNEL_PHONEBOOK_H
#define KERNEL_PHONEBOOK_H

#include <linux/types.h>

#include "phonebook_constants.h"

struct pb_user_data {
	char name[PHONEBOOK_NAME_MAX_LENGTH];
	char surname[PHONEBOOK_SURNAME_MAX_LENGTH];
	uint64_t age;
	char phone[PHONEBOOK_PHONE_MAX_LENGTH];
	char email[PHONEBOOK_EMAIL_MAX_LENGTH];
};

#define PB_ADD 0
#define PB_DEL 1
#define PB_GET 2

#define PB_OK 0
#define PB_FAIL 1

#endif // KERNEL_PHONEBOOK_H

