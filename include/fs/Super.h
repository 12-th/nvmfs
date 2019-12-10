#ifndef NVMFS_SUPER_BLOCK_H
#define NVMFS_SUPER_BLOCK_H

struct super_operations
{
    struct inode * (*alloc_inode)(struct super_block * sb);
    void (*destroy_inode)(struct inode *);

    int (*drop_inode)(struct inode *);
    void (*evict_inode)(struct inode *);
    void (*put_super)(struct super_block *);

    int (*statfs)(struct dentry *, struct kstatfs *);
    void (*umount_begin)(struct super_block *);
};

#endif