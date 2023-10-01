CC=gcc
KERNEL_PATH ?= /lib/modules/$(shell uname -r)/build
obj-m += hook.o

hook: hook.c
	make -C $(KERNEL_PATH) M=$(PWD) modules

check: check.c
	$(CC) check.c -o check
	./check

clean:
	make -C $(KERNEL_PATH) M=$(PWD) clean
	rm -f check
