#ifndef LINK_INODE_H
#define LINK_INODE_H

#include <linux/fs.h>

int nvmfs_symlink(struct inode * pNodeDir, struct dentry * pDentry, const char * symname);

#endif