#ifndef NVMFS_INODE_H
#define NVMFS_INODE_H

#include "InodeTable.h"
#include "Log.h"
#include "Types.h"
#include <linux/fs.h>

struct NvmInode
{
    struct timespec atime;
    struct timespec mtime;
    struct timespec ctime;
    UINT32 linkCount;
    nvmfs_inode_type type;
    nvmfs_ino_t ino;
};

struct NvmfsInode
{
    struct NvmInode nvmInode;
    struct inode vfsInode;
    logic_addr_t addr;
    struct Log log;
};

struct inode * nvmfs_alloc_inode(struct super_block * pSuperBlock, const struct inode * pDirNode, umode_t mode,
                                 dev_t dev);
int nvmfs_mknod(struct inode * pDirNode, struct dentry * pDentry, umode_t mode, dev_t dev);

struct inode * NvmfsAllocInode(struct super_block * sb);
void NvmfsDestroyInode(struct inode * vfsInode);

// inode log的前部分保留数据，一个是现在的inode，一个是保存的副本inode

#endif