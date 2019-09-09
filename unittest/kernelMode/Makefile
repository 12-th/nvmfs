#--------------src directory-------------------
KDIR = /lib/modules/`uname -r`/build
SRC_DIR = $(PWD)/../..
UNITTEST_DIR = $(PWD)/..
MODULE_DIR = $(PWD)

EXCLUDE_SRC = 

EXCLUDE_UNITTEST = 

INCLUDE_DIR = 	$(SRC_DIR)/include	\
				$(UNITTEST_DIR)/include	\
				$(MODULE_DIR)/

MODULE_OBJS := $(patsubst %.c, %.o, $(wildcard $(MODULE_DIR)/*.c))
SRC_OBJS := $(patsubst %.c, %.o, $(filter-out $(addprefix $(SRC_DIR)/src/, $(EXCLUDE_SRC)), $(wildcard $(SRC_DIR)/src/*.c)))
UNITTEST_OBJS := $(patsubst %.c, %.o, $(filter-out $(addprefix $(UNITTEST_DIR)/, $(EXCLUDE_UNITTEST)), $(wildcard $(UNITTEST_DIR)/*.c)))
OBJS += $(subst $(PWD)/, , $(MODULE_OBJS))
OBJS += $(subst $(PWD)/, , $(SRC_OBJS))
OBJS += $(subst $(PWD)/, , $(UNITTEST_OBJS))

#--------------compile flags----------------
MACRO_DEFINES = KUTEST_KERNEL_MODE	\
				DEBUG				\
				KERNEL_MODE			\


C_INCLUDE_DIRS = $(addprefix -I, $(INCLUDE_DIR))
C_MACRO_DEFINES = $(addprefix -D, $(MACRO_DEFINES))

ccflags-y := -Wall -g $(C_INCLUDE_DIRS) $(C_MACRO_DEFINES)
ldflags-y += -T$(PWD)/kutest_link.lds
clean-files := $(OBJS)

obj-m = unittest.o
unittest-y = $(OBJS)

#--------------compile implement------------------
kbuild:
	make -C $(KDIR) M=`pwd`

clean:
	make -C $(KDIR) M=`pwd` clean
	rm -f $(SRC_DIR)/.*.o.cmd
	rm -f $(UNITTEST_DIR)/.*.o.cmd

install:
	insmod unittest.ko

uninstall:
	rmmod unittest

