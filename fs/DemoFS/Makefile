ifneq ($(KERNELRELEASE),)
		obj-m := demofs.o
		demofs-objs := inode.o file.o
EXTRA_CFLAGS := -ggdb -O0
else
		KERNELDIR ?= /lib/modules/$(shell uname -r)/build
		PWD := $(shell pwd)
default:
		$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
		rm *.order *.symvers *.mod.c *.o .*.o.cmd .*.cmd .tmp_versions -rf
endif