obj-m += phonebook.o
phonebook-objs += phonebook_device.o phonebook_store.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

