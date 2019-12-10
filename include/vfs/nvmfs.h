#ifndef NVM_FS_H
#define NVM_FS_H

#include <linux/fs.h>

extern struct file_system_type nvmfs;

struct dentry * nvmfs_mount(struct file_system_type * pFSType, int flags, const char * devName, void * options);
static void nvmfs_kill_sb(struct super_block * pSuperBlock);

#endif