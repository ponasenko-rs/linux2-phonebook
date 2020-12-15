#ifndef PHONEBOOK_STORE_H
#define PHONEBOOK_STORE_H

#include "kernel_phonebook.h"

int pb_get(const char *surname, unsigned int len, struct pb_user_data *data);
int pb_add(const struct pb_user_data *data);	
int pb_del(const char *surname, unsigned int len);

#endif // PHONEBOOK_STORE_H

