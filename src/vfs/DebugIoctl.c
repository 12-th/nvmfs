#include "DirInodeInfo.h"
#include "File.h"
#include "FileInodeInfo.h"
#include "FsInfo.h"
#include "InodeType.h"
#include "WearLeveler.h"
#include "common.h"
#include <linux/slab.h>

static void PrintLogicAddr(struct file * file, unsigned long addr, char * buf)
{
    int i = 0;
    UINT64 * buffer;
    struct NvmfsInfo * fsInfo;

    fsInfo = file->f_inode->i_sb->s_fs_info;
    buffer = kmalloc(PAGE_SIZE, GFP_KERNEL);
    NVMAccesserRead(&fsInfo->acc, addr, PAGE_SIZE, buffer);
    buf += sprintf(buf, "data from 0x%lx:\n", addr);
    for (i = 0; i < PAGE_SIZE / sizeof(UINT64); i += 4)
    {
        buf += sprintf(buf, "0x%lx :  0x%lx 0x%lx 0x%lx 0x%lx\n", addr + i * sizeof(UINT64), buffer[i], buffer[i + 1],
                       buffer[i + 2], buffer[i + 3]);
    }
    kfree(buffer);
}

static void PrintPhysAddr(struct file * file, unsigned long offset, char * buf)
{
    // int i = 0;
    // UINT64 * buffer;
    // struct NvmfsInfo * fsInfo;
    // UINT64 dataStart, addr;

    // fsInfo = file->f_inode->i_sb->s_fs_info;
    // dataStart = WearLevelerDataStartAddrQuery(&fsInfo->wl);
    // addr = dataStart + offset;
    // buffer = kmalloc(PAGE_SIZE, GFP_KERNEL);

    // buf += sprintf(buf, "data from 0x%lx:\n", addr);
    // for (i = 0; i < PAGE_SIZE / sizeof(UINT64); i += 8)
    // {
    //     buf += sprintf(buf, "0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n", buffer[i], buffer[i + 1],
    //                    buffer[i + 2], buffer[i + 3], buffer[i + 4], buffer[i + 5], buffer[i + 6], buffer[i + 7]);
    // }
    // kfree(buffer);
}

static void PrintSpaceInfo(struct file * file, char * buf)
{
    struct NvmfsInfo * fsInfo;
    UINT64 blockNum, pageNum, treeBlockNum, cachedBlockNum;

    fsInfo = file->f_inode->i_sb->s_fs_info;
    pageNum = fsInfo->ppool.pageNum;
    blockNum = fsInfo->bpool.blockNum;
    treeBlockNum = BlockPoolQueryInTreeBlockNum(&fsInfo->bpool);
    cachedBlockNum = fsInfo->bpool.cachedBlockNum;
    sprintf((char *)buf,
            "the rest space is %lx, block num should be %ld, page num should be %ld\n"
            "calculate block num is %ld, in tree block num is %ld, cached in array block num is %ld\n",
            (blockNum << BITS_2M) + (pageNum << BITS_4K), blockNum, pageNum, treeBlockNum + cachedBlockNum,
            treeBlockNum, cachedBlockNum);
}

static void PrintFileFirstSpace(struct file * file, char * buf)
{
    struct BaseInodeInfo * inodeInfo;

    inodeInfo = file->f_inode->i_private;
    if (inodeInfo == NULL)
    {
        sprintf(buf, "inode Info is null\n");
        return;
    }
    if (inodeInfo->type == INODE_TYPE_REGULAR_FILE)
    {
        logic_addr_t logStartAddr;
        UINT64 areaNum;
        logStartAddr = ((struct FileInodeInfo *)(inodeInfo))->log.cs.firstArea;
        areaNum = ((struct FileInodeInfo *)(inodeInfo))->log.cs.areaNum;
        sprintf(buf, "regular file, log start is 0x%lx, log area num is %ld\n", logStartAddr, areaNum);
        return;
    }
    if (inodeInfo->type == INODE_TYPE_DIR_FILE)
    {
        logic_addr_t logStartAddr;
        UINT64 areaNum;
        logStartAddr = ((struct DirInodeInfo *)(inodeInfo))->log.cs.firstArea;
        areaNum = ((struct DirInodeInfo *)(inodeInfo))->log.cs.areaNum;
        sprintf(buf, "regular file, log start is 0x%lx, log area num is %ld\n", logStartAddr, areaNum);
        return;
    }
}

long NvmfsUnlockedIoctl(struct file * file, unsigned int cmd, unsigned long arg)
{
    switch (cmd)
    {
    case 0xf001:
        PrintLogicAddr(file, ((unsigned long *)arg)[0], (char *)(((unsigned long *)arg)[1]));
        break;
    case 0xf002:
        PrintPhysAddr(file, ((unsigned long *)arg)[0], (char *)(((unsigned long *)arg)[1]));
        break;
    case 0xf003:
        PrintSpaceInfo(file, (char *)arg);
        break;
    case 0xf004:
        PrintFileFirstSpace(file, (char *)arg);
        break;
    }
    return 0;
}