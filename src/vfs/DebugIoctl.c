#include "DirInodeInfo.h"
#include "File.h"
#include "FileInodeInfo.h"
#include "FsInfo.h"
#include "InodeType.h"
#include "common.h"
#include <linux/slab.h>

static void PrintLogicAddr(struct file * file, unsigned long addr)
{
    int i = 0;
    UINT64 * buffer;
    struct NvmfsInfo * fsInfo;

    fsInfo = file->f_inode->i_sb->s_fs_info;
    buffer = kmalloc(PAGE_SIZE, GFP_KERNEL);
    NVMAccesserRead(&fsInfo->acc, addr, PAGE_SIZE, buffer);
    DEBUG_PRINT("data from 0x%lx:\n", addr);
    for (i = 0; i < PAGE_SIZE / sizeof(UINT64); i += 4)
    {
        DEBUG_PRINT("0x%lx 0x%lx 0x%lx 0x%lx", buffer[i], buffer[i + 1], buffer[i + 2], buffer[i + 3]);
    }
    kfree(buffer);
}

static void PrintPhysAddr(struct file * file, unsigned long offset)
{
    int i = 0;
    UINT64 * buffer;
    struct NvmfsInfo * fsInfo;
    UINT64 dataStart, addr;

    fsInfo = file->f_inode->i_sb->s_fs_info;
    dataStart = DataStartAddrQuery(&fsInfo->wl.layouter);
    addr = dataStart + offset;
    buffer = kmalloc(PAGE_SIZE, GFP_KERNEL);
    NVMAccesserRead(&fsInfo->acc, addr, PAGE_SIZE, buffer);
    DEBUG_PRINT("data from 0x%lx:\n", addr);
    for (i = 0; i < PAGE_SIZE / sizeof(UINT64); i += 8)
    {
        DEBUG_PRINT("0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx", buffer[i], buffer[i + 1], buffer[i + 2],
                    buffer[i + 3], buffer[i + 4], buffer[i + 5], buffer[i + 6], buffer[i + 7]);
    }
    kfree(buffer);
}

long NvmfsUnlockedIoctl(struct file * file, unsigned int cmd, unsigned long arg)
{
    if (cmd == 0xf001)
    {
        PrintLogicAddr(file, arg);
        return 0;
    }
    if (cmd == 0xf002)
    {
        PrintPhysAddr(file, arg);
    }
    return 0;
}