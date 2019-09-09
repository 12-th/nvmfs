#include "CharDeviceOperations.h"
#include "UnitTestModule.h"
#include "IOctlDefine.h"
#include "kutest.h"
#include <linux/cdev.h>

static int standard_cdev_open(struct inode * inode, struct file * file)
{
    struct StandardCharDevice * scdev = container_of(inode->i_cdev, struct StandardCharDevice, cdev);
    file->private_data = scdev;

    // printk(KERN_DEBUG "open invoked\n");
    return 0;
}

static ssize_t standard_cdev_read(struct file * file, char __user * user_buffer, size_t size, loff_t * offset)
{
    // printk(KERN_DEBUG "read invoked\n");
    return size;
}

static ssize_t standard_cdev_write(struct file * file, const char __user * user_buffer, size_t size, loff_t * offset)
{
    // printk(KERN_DEBUG "write invoked\n");
    return size;
}

static long standard_cdev_ioctl(struct file * file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct UnitTestArgs * args = (struct UnitTestArgs *)arg;

    // printk(KERN_DEBUG "ioctl invoked\n");

    switch(cmd)
    {
        case UNIT_TEST_GET_TESTS_COUNT:
            args->testCount = KutestGetUnitTestCount();
            break;
        case UNIT_TEST_GET_TEST_NAME:
            ret = KutestGetSpecifyUnitTestName(args->index, args->testCaseName, args->testName);
            break;
        case UNIT_TEST_RUN_TEST:
            ret = KutestRunSpecifyUnitTest(args->index);
            break;
        default:
        break;
    }

    return ret;
}

static int standard_cdev_release(struct inode * inode, struct file * file)
{
    // printk(KERN_DEBUG "close invoked\n");
    return 0;
}

struct file_operations StandardCharDeviceOps = 
{
    .open = standard_cdev_open,
    .read = standard_cdev_read,
    .write = standard_cdev_write,
    .unlocked_ioctl = standard_cdev_ioctl,
    .release = standard_cdev_release
};