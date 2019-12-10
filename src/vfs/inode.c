#include "DirInode.h"
#include "FileInode.h"
#include "LinkInode.h"
#include "Log.h"
#include "PagePool.h"
#include "common.h"
#include <linux/fs.h>
#include <linux/pagemap.h>

static const struct address_space_operations nvmfsAddrSpaceOps = {
    .readpage = simple_readpage,
    .write_begin = simple_write_begin,
    .write_end = simple_write_end,
};

struct inode * nvmfs_alloc_inode(struct super_block * pSuperBlock, const struct inode * pDirNode, umode_t mode,
                                 dev_t dev)
{
    // allocate a new inode in memory. if we have set the alloc_inode callback in super_operations, it will be called.
    // otherwise, it will just call kmem_cache_alloc and do some initialize work(set i_nlink, i_sb, i_blkbits,
    // i_dev...).
    struct inode * pInode = new_inode(pSuperBlock);

    if (pInode)
    {
        pInode->i_ino = get_next_ino();

        // init uid,gid,mode for new inode
        inode_init_owner(pInode, pDirNode, mode);

        pInode->i_mapping->a_ops = &nvmfsAddrSpaceOps;
        mapping_set_gfp_mask(pInode->i_mapping, GFP_HIGHUSER);
        mapping_set_unevictable(pInode->i_mapping);

        pInode->i_atime = pInode->i_mtime = pInode->i_ctime = current_time(pInode);

        switch (mode & S_IFMT)
        {
        case S_IFREG:
            pInode->i_op = &nvmfsFileInodeOps;
            pInode->i_fop = &nvmfsFileOps;
            break;
        case S_IFDIR:
            pInode->i_op = &nvmfsDirInodeOps;
            pInode->i_fop = &simple_dir_operations;
            break;
        case S_IFLNK:
            pInode->i_op = &page_symlink_inode_operations;
            break;
        default:
            init_special_inode(pInode, mode, dev);
            break;
        }
    }

    return pInode;
}

int nvmfs_mknod(struct inode * pDirNode, struct dentry * pDentry, umode_t mode, dev_t dev)
{
    struct inode * pInode = nvmfs_alloc_inode(pDirNode->i_sb, pDirNode, mode, dev);

    if (pInode)
    {
        // associates a dentry with an inode. must use d_instantiate instead of d_add here.
        d_instantiate(pDentry, pInode);
        // increase the reference count
        dget(pDentry);

        pDirNode->i_mtime = pDirNode->i_ctime = current_time(pDirNode);
        return 0;
    }

    return -ENOSPC;
}

struct inode * NvmfsAllocInode(struct super_block * sb)
{
    struct kmem_cache * inodeSlab;
    struct NvmfsInode * inode;

    inodeSlab = sb->s_fs_info->inodeSlab;
    inode = kmem_cache_alloc(inodeSlab, GFP_KERNEL);
    inode->addr = invalid_nvm_addr;

    return &inode->vfsInode;
}

void NvmfsDestroyInode(struct inode * vfsInode)
{
    struct NvmfsInode * inode;

    inode = container_of(vfsInode, struct NvmfsInode, vfsInode);
    kmem_cache_free(vfsInode->i_sb->s_fs_info->inodeSlab, inode);
}

void InodeFormat(struct NvmfsInode * inode, struct NvmInode * nvmInode, logic_addr_t addr)
{
}

static inline int NvmInodeFormat(struct NvmInode * nvmInode, struct NvmfsInfo * info)
{
    nvmInode->atime = nvmInode->mtime = nvmInode->ctime = current_time(dirInode);
    nvmInode->linkCount = 1;
    nvmInode->flags.regularFile = 1;
    nvmInode->ino = InodeTableGetIno(&info->inodeTable);
    if (nvmInode->ino == invalid_nvmfs_ino)
        return -ENOSPC;
    return 0;
}

int NvmfsInodeCreate(struct inode * dirInode, struct dentry * vfsDentry, umode_t mode, bool excl)
{
    struct NvmInode nvmInode;
    struct NvmfsInode * inode;
    struct vfsInode * vfsInode;
    struct super_block * sb;
    int err;

    sb = dirInode->i_sb;

    err = NvmInodeFormat(&nvmInode, sb->s_fs_info);
    if (err)
        return err;
    vfsInode = NvmfsAllocInode(sb);
    inode = container_of(vfsInode, struct NvmfsInode, vfsInode);
    inode->addr = PagePoolAlloc(&sb->inodePool);
    LogFormat(&inode->log, inode->addr, &sb->acc, &nvmInode, sizeof(struct NvmInode));
    InodeTableUpdateInodeAddr(&sb->s_fs_info->inodeTable, inode->ino, inode->addr);
    // append entry to directory
}

struct inode_operations
{
    int (*create)(struct inode *, struct dentry *, umode_t, bool);
    struct dentry * (*lookup)(struct inode *, struct dentry *, unsigned int);
    // int (*link)(struct dentry *, struct inode *, struct dentry *);
    int (*unlink)(struct inode *, struct dentry *);
    // int (*symlink)(struct inode *, struct dentry *, const char *);
    int (*mkdir)(struct inode *, struct dentry *, umode_t);
    int (*rmdir)(struct inode *, struct dentry *);
    // int (*mknod)(struct inode *, struct dentry *, umode_t, dev_t);
    int (*rename)(struct inode *, struct dentry *, struct inode *, struct dentry *, unsigned int);
    // int (*readlink)(struct dentry *, char __user *, int);
    // const char * (*get_link)(struct dentry *, struct inode *, struct delayed_call *);
    // int (*setattr) (struct dentry *, struct iattr *);
    // int (*getattr) (const struct path *, struct kstat *, u32, unsigned int);
};