#ifndef VFS_INODE_H
#define VFS_INODE_H

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

extern struct inode_operations NvmfsDirInodeOps;
extern struct inode_operations NvmfsFileInodeOps;

int VfsInodeRebuild(struct super_block * sb, nvmfs_ino_t ino, struct inode * dirInode, struct inode ** inodePtr);

// struct inode * nvmfs_alloc_inode(struct super_block * pSuperBlock, const struct inode * pDirNode, umode_t mode,
//                                  dev_t dev);
// int nvmfs_mknod(struct inode * pDirNode, struct dentry * pDentry, umode_t mode, dev_t dev);

// struct inode * NvmfsAllocInode(struct super_block * sb);
// void NvmfsDestroyInode(struct inode * vfsInode);

#endif