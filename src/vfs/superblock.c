#include "FsInfo.h"
#include "VfsInode.h"
#include "common.h"
#include "nvmfs.h"
#include <linux/init.h>
#include <linux/parser.h>
#include <linux/slab.h>

#define NVMFS_MAGIC 0xbadc0de

const struct super_operations nvmfsSuperOps;

void NvmfsKillSuperBlock(struct super_block * sb)
{
    DEBUG_PRINT("umount ! ! !")
    NvmfsInfoUninit(sb->s_fs_info);
    kfree(sb->s_fs_info);
    kill_litter_super(sb);
}

int NvmfsFillSuperBlock(struct super_block * sb, void * options, int silent)
{
    struct NvmfsInfo * info;
    struct inode * pInode;
    struct dentry * pDentry;
    int err;

    DEBUG_PRINT("------------mount-------------")

    sb->s_maxbytes = MAX_LFS_FILESIZE;
    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    sb->s_magic = NVMFS_MAGIC;
    sb->s_op = &nvmfsSuperOps;
    sb->s_time_gran = 1;
    sb->s_type = &nvmfs;

    info = kzalloc(sizeof(struct NvmfsInfo), GFP_KERNEL);
    if (!info)
        return -ENOMEM;
    NvmfsInfoFormat(info, S_IFDIR);
    sb->s_fs_info = info;
    err = VfsRootInodeFormat(sb, &pInode, &pDentry);
    if (err)
    {
        sb->s_fs_info = NULL;
        kfree(info);
        iput(pInode);
        return -ENOMEM;
    }
    sb->s_root = pDentry;

    return 0;
}

const struct super_operations nvmfsSuperOps = {.drop_inode = generic_delete_inode, .statfs = simple_statfs};