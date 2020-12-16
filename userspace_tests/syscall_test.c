#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "phonebook_constants.h"

struct user_data {
	char name[PHONEBOOK_NAME_MAX_LENGTH];
	char surname[PHONEBOOK_SURNAME_MAX_LENGTH];
	uint64_t age;
	char phone[PHONEBOOK_PHONE_MAX_LENGTH];
	char email[PHONEBOOK_EMAIL_MAX_LENGTH];
};

int user_data_equal(const struct user_data *lhs,
	const struct user_data *rhs) {

	return strcmp(&lhs->name, &rhs->name) == 0
		&& strcmp(&lhs->surname, &rhs->surname) == 0
		&& lhs->age == rhs->age
		&& strcmp(&lhs->phone, &rhs->phone) == 0
		&& strcmp(&lhs->email, &rhs->email) == 0;
}

long get_user(const char* surname, unsigned int len, struct user_data* output_data) {
	return syscall(548, surname, len, output_data);
}

long add_user(struct user_data* input_data) {
	return syscall(549, input_data);
}

long del_user(const char* surname, unsigned int len) {
	return syscall(550, surname, len);
}


int main() {
	struct user_data data;
	strcpy(data.name, "Ada");
	strcpy(data.surname, "Lovelace");
	data.age = 20;
	strcpy(data.phone, "111111111");
	strcpy(data.name, "a_l@g.c");
	
	struct user_data got_data;

	int res = get_user("Lovelace", 8, &got_data);
	if (res == 0) {
		assert(user_data_equal(&data, &got_data));
	} else {
		res = add_user(&data);
		assert(res == 0);

		res = get_user("Lovelace", 8, &got_data);
		assert(res == 0);
		assert(user_data_equal(&data, &got_data));

		res = del_user("Lovelace", 8);
		assert(res == 0);
		assert(get_user("Lovelace", 8, &got_data) != 0);
	}

	return 0;	
}
