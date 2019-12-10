#ifndef DIR_INODE_H
#define DIR_INODE_H

#include <linux/fs.h>

extern const struct inode_operations nvmfsDirInodeOps;

void NvmfsDirInodeAddNewFile(struct inode * vfsDirInode, struct dentry * vfsFileDentry, struct inode * vfsFileInode);

#endif