#include "DirInodeInfo.h"
#include "FileInodeInfo.h"
#include "FsInfo.h"
#include "Inode.h"
#include "VfsFile.h"
#include "VfsInode.h"
#include "common.h"
#include <linux/fs.h>
#include <linux/pagemap.h>

// static const struct address_space_operations nvmfsAddrSpaceOps = {
//     .readpage = simple_readpage,
//     .write_begin = simple_write_begin,
//     .write_end = simple_write_end,
// };

// static struct inode * NvmfsAllocInode(struct super_block * pSuperBlock)
// {
//     // allocate a new inode in memory. if we have set the alloc_inode callback in super_operations, it will be
//     // called.
//     // otherwise, it will just call kmem_cache_alloc and do some initialize work(set i_nlink, i_sb, i_blkbits,
//     // i_dev...).
//     struct inode * pInode = new_inode(pSuperBlock);

//     if (pInode)
//     {

//         // init uid,gid,mode for new inode
//         inode_init_owner(pInode, pDirNode, mode);

//         pInode->i_mapping->a_ops = &nvmfsAddrSpaceOps;
//         mapping_set_gfp_mask(pInode->i_mapping, GFP_HIGHUSER);
//         mapping_set_unevictable(pInode->i_mapping);

//         pInode->i_atime = pInode->i_mtime = pInode->i_ctime = current_time(pInode);

//         switch (mode & S_IFMT)
//         {
//         case S_IFREG:
//             pInode->i_op = &nvmfsFileInodeOps;
//             pInode->i_fop = &nvmfsFileOps;
//             break;
//         case S_IFDIR:
//             pInode->i_op = &nvmfsDirInodeOps;
//             pInode->i_fop = &simple_dir_operations;
//             break;
//         case S_IFLNK:
//             pInode->i_op = &page_symlink_inode_operations;
//             break;
//         default:
//             init_special_inode(pInode, mode, dev);
//             break;
//         }
//     }

//     return pInode;
// }

// int nvmfs_mknod(struct inode * pDirNode, struct dentry * pDentry, umode_t mode, dev_t dev)
// {
//     struct inode * pInode = nvmfs_alloc_inode(pDirNode->i_sb, pDirNode, mode, dev);

//     if (pInode)
//     {
//         // associates a dentry with an inode. must use d_instantiate instead of d_add here.
//         d_instantiate(pDentry, pInode);
//         // increase the reference count
//         dget(pDentry);

//         pDirNode->i_mtime = pDirNode->i_ctime = current_time(pDirNode);
//         return 0;
//     }

//     return -ENOSPC;
// }

// void InodeFormat(struct NvmfsInode * inode, struct NvmInode * nvmInode, logic_addr_t addr)
// {
// }

static struct inode * AllocAndInitVfsInode(struct super_block * sb, UINT8 type, mode_t mode, struct inode * dirInode)
{
    struct inode * pInode;

    pInode = new_inode(sb);
    if (pInode == NULL)
        return NULL;
    switch (type)
    {
    case INODE_TYPE_REGULAR_FILE:
        pInode->i_op = &NvmfsFileInodeOps;
        pInode->i_fop = &NvmfsRegFileOps;
        mode = (S_IFREG) | (mode & (~S_IFMT));
    case INODE_TYPE_DIR_FILE:
        pInode->i_op = &NvmfsDirInodeOps;
        pInode->i_fop = &NvmfsDirFileOps;
        mode = S_IFDIR | (mode & (~S_IFMT));
        break;
    case INODE_TYPE_LINK_FILE:
        break;
    }
    inode_init_owner(pInode, dirInode, mode);
    pInode->i_mapping->a_ops = NULL;
    pInode->i_atime = pInode->i_mtime = pInode->i_ctime = current_time(pInode);
    return pInode;
}

int VfsInodeRebuild(struct super_block * sb, nvmfs_ino_t ino, struct inode * dirInode, struct inode ** inodePtr)
{
    struct inode * pInode;
    struct NvmfsInfo * info;
    struct BaseInodeInfo * inodeInfo;
    int err;

    info = sb->s_fs_info;
    err = InodeRebuild(&inodeInfo, &info->inodeTable, ino, &info->ppool, &info->bpool, &info->acc);
    if (err)
        return err;

    pInode = AllocAndInitVfsInode(sb, inodeInfo->type, inodeInfo->mode, dirInode);
    if (pInode == NULL)
    {
        InodeUninit(inodeInfo, info);
        return -ENOMEM;
    }

    pInode->i_private = inodeInfo;
    *inodePtr = pInode;
    return 0;
}

static UINT8 ModeToType(umode_t mode)
{
    switch (mode & S_IFMT)
    {
    case S_IFREG:
        return INODE_TYPE_REGULAR_FILE;
        break;
    case S_IFDIR:
        return INODE_TYPE_DIR_FILE;
        break;
    case S_IFLNK:
        return INODE_TYPE_LINK_FILE;
        break;
    default:
        break;
    }
    return -1;
}

static int VfsInodeCreateImpl(struct inode * dirInode, struct dentry * vfsDentry, UINT8 type, umode_t mode)
{
    struct BaseInodeInfo * inodeInfo;
    struct NvmfsInfo * fsInfo;
    int err;
    struct inode * pInode;
    struct super_block * sb;
    nvmfs_ino_t parentIno;

    sb = dirInode->i_sb;
    fsInfo = sb->s_fs_info;
    parentIno = ((struct BaseInodeInfo *)(dirInode->i_private))->parentIno;
    pInode = AllocAndInitVfsInode(sb, type, mode, dirInode);
    err = InodeFormat(&inodeInfo, fsInfo, type, parentIno, mode);
    if (err)
    {
        iput(pInode);
        return err;
    }

    err = DirInodeInfoAddDentry(dirInode->i_private, inodeInfo->thisIno, (char *)vfsDentry->d_name.name,
                                vfsDentry->d_name.len + 1, type, &fsInfo->acc);
    if (err)
    {
        InodeDestroy(inodeInfo, fsInfo);
        iput(pInode);
        return err;
    }
    d_instantiate(vfsDentry, pInode);
    return 0;
}

static int VfsInodeCreate(struct inode * dirInode, struct dentry * vfsDentry, umode_t mode, bool excl)
{
    DEBUG_PRINT("inode create, name is %s, len is %ld\n", vfsDentry->d_name.name, (unsigned long)vfsDentry->d_name.len);
    return VfsInodeCreateImpl(dirInode, vfsDentry, ModeToType(mode), mode);
}

static struct dentry * VfsInodeLookup(struct inode * dirInode, struct dentry * dentry, unsigned int flags)
{
    struct DirInodeInfo * dirInfo;
    int err;
    nvmfs_ino_t ino;
    struct NvmfsInfo * fsInfo;
    struct inode * pInode;
    struct BaseInodeInfo * inodeInfo;

    dirInfo = dirInode->i_private;
    fsInfo = dirInode->i_sb->s_fs_info;
    DEBUG_PRINT("inode lookup, dirInode ino is %ld, name is %s, len is %ld", dirInfo->baseInfo.thisIno,
                dentry->d_name.name, (unsigned long)dentry->d_name.len);
    err = DirInodeInfoLookupDentryByName(dirInfo, &ino, (char *)dentry->d_name.name, dentry->d_name.len + 1,
                                         &fsInfo->acc);
    if (err)
        return NULL;
    err = InodeRebuild(&inodeInfo, &fsInfo->inodeTable, ino, &fsInfo->ppool, &fsInfo->bpool, &fsInfo->acc);
    if (err)
        return ERR_PTR(err);
    pInode = AllocAndInitVfsInode(dirInode->i_sb, inodeInfo->type, 0, dirInode);
    if (!pInode)
    {
        InodeUninit(inodeInfo, fsInfo);
        return ERR_PTR(-ENOMEM);
    }
    pInode->i_private = inodeInfo;

    d_add(dentry, pInode);
    return NULL;
}

static int VfsInodeRemoveImpl(struct inode * dirInode, struct dentry * dentry)
{
    struct DirInodeInfo * dirInfo;
    int err;
    nvmfs_ino_t ino;
    struct NvmfsInfo * fsInfo;

    dirInfo = dirInode->i_private;
    fsInfo = dirInode->i_sb->s_fs_info;
    err = DirInodeInfoLookupDentryByName(dirInfo, &ino, (char *)dentry->d_name.name, dentry->d_name.len + 1,
                                         &fsInfo->acc);
    if (err)
        return err;
    return DirInodeInfoRemoveDentry(dirInfo, ino, &fsInfo->acc);
}

static int VfsInodeUnlink(struct inode * dirInode, struct dentry * dentry)
{
    return VfsInodeRemoveImpl(dirInode, dentry);
}

static int VfsInodeMkdir(struct inode * dirInode, struct dentry * dentry, umode_t mode)
{
    return VfsInodeCreateImpl(dirInode, dentry, INODE_TYPE_DIR_FILE, mode);
}

static int VfsInodeRmdir(struct inode * dirInode, struct dentry * dentry)
{
    return VfsInodeRemoveImpl(dirInode, dentry);
}

struct inode_operations NvmfsDirInodeOps = {.create = VfsInodeCreate,
                                            .lookup = VfsInodeLookup,
                                            .unlink = VfsInodeUnlink,
                                            .mkdir = VfsInodeMkdir,
                                            .rmdir = VfsInodeRmdir};

struct inode_operations NvmfsFileInodeOps = {};

// struct inode_operations
// {
// int (*create)(struct inode *, struct dentry *, umode_t, bool);
// struct dentry * (*lookup)(struct inode *, struct dentry *, unsigned int);
// int (*link)(struct dentry *, struct inode *, struct dentry *);
// int (*unlink)(struct inode *, struct dentry *);
// int (*symlink)(struct inode *, struct dentry *, const char *);
// int (*mkdir)(struct inode *, struct dentry *, umode_t);
// int (*rmdir)(struct inode *, struct dentry *);
// int (*mknod)(struct inode *, struct dentry *, umode_t, dev_t);
// int (*rename)(struct inode *, struct dentry *, struct inode *, struct dentry *, unsigned int);
// int (*readlink)(struct dentry *, char __user *, int);
// const char * (*get_link)(struct dentry *, struct inode *, struct delayed_call *);
// int (*setattr) (struct dentry *, struct iattr *);
// int (*getattr) (const struct path *, struct kstat *, u32, unsigned int);
// };