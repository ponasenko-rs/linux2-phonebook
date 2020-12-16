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

#endif // KERNEL_PHONEBOOK_H

