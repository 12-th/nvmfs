#ifndef VFS_INODE_H
#define VFS_INODE_H

#include "InodeTable.h"
#include "Log.h"
#include "Types.h"
#include <linux/fs.h>

extern struct inode_operations NvmfsDirInodeOps;
extern struct inode_operations NvmfsFileInodeOps;

int VfsRootInodeFormat(struct super_block * sb, struct inode ** rootInodePtr, struct dentry ** rootDentryPtr);

#endif