#ifndef DEBUG_IOCTL_H
#define DEBUG_IOCTL_H

#include <linux/fs.h>

long NvmfsUnlockedIoctl(struct file * file, unsigned int cmd, unsigned long arg);

#endif