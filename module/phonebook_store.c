#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "phonebook_store.h"

struct pb_entry {
	struct pb_user_data data;

	struct list_head list_node;
};

static LIST_HEAD(pb_entries);

int pb_get(const char *surname, unsigned int len, struct pb_user_data *data) {
	struct list_head *pos;

	if (len >= PHONEBOOK_SURNAME_MAX_LENGTH) {
		return -1;
	}


	list_for_each(pos, &pb_entries) {
		struct pb_entry* entry = NULL;
		entry = list_entry(pos, struct pb_entry, list_node);

		if (memcmp(&entry->data.surname, surname, len + 1) == 0) {
			if (data != NULL) {
				memcpy(data, &entry->data, sizeof(struct pb_user_data));
			}

			return 0;
		}
	}

	return -1;
}

int pb_add(const struct pb_user_data *data) {
	struct pb_entry* new_entry = kmalloc(sizeof(struct pb_entry), GFP_KERNEL);
	if (new_entry == NULL) {
		return -1;
	}

	memcpy(&new_entry->data, data, sizeof(struct pb_user_data));

	// cleaning
	new_entry->data.name[PHONEBOOK_NAME_MAX_LENGTH - 1] = '\0';
	new_entry->data.surname[PHONEBOOK_SURNAME_MAX_LENGTH - 1] = '\0';
	new_entry->data.phone[PHONEBOOK_PHONE_MAX_LENGTH - 1] = '\0';
	new_entry->data.email[PHONEBOOK_EMAIL_MAX_LENGTH - 1] = '\0';

	if (pb_get((const char *)&new_entry->data.surname,
		strlen((const char *)&new_entry->data.surname), NULL) == 0) {

		kfree(new_entry);
		return -1;
	}

	list_add_tail(&new_entry->list_node, &pb_entries);
	return 0;
}

// todo: implement
int pb_del(const char *surname, unsigned int len) {
	return -1;
}

