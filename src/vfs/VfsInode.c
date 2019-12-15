#include "DirInodeInfo.h"
#include "FileInodeInfo.h"
#include "FsInfo.h"
#include "Inode.h"
#include "InodeType.h"
#include "VfsFile.h"
#include "VfsInode.h"
#include "common.h"
#include <linux/fs.h>
#include <linux/pagemap.h>

static struct inode * AllocAndInitVfsInode(struct super_block * sb, mode_t mode, struct inode * dirInode)
{
    struct inode * pInode;

    pInode = new_inode(sb);
    if (pInode == NULL)
        return NULL;
    switch (mode & S_IFMT)
    {
    case S_IFREG:
        pInode->i_op = &NvmfsFileInodeOps;
        pInode->i_fop = &NvmfsRegFileOps;
        break;
    case S_IFDIR:
        pInode->i_op = &NvmfsDirInodeOps;
        pInode->i_fop = &NvmfsDirFileOps;
        break;
    case S_IFLNK:
        break;
    }
    inode_init_owner(pInode, dirInode, mode);
    pInode->i_mapping->a_ops = NULL;
    pInode->i_atime = pInode->i_mtime = pInode->i_ctime = current_time(pInode);
    return pInode;
}

static inline void InitVfsInodeFromInodeInfo(struct inode * vfsInode, struct BaseInodeInfo * info)
{
    vfsInode->i_mode = info->mode;
    vfsInode->i_atime = info->atime;
    vfsInode->i_ctime = info->ctime;
    vfsInode->i_mtime = info->mtime;
    vfsInode->i_blkbits = BITS_4K;
    vfsInode->i_uid = info->uid;
    vfsInode->i_gid = info->gid;
}

static inline void BaseInodeInfoFormat(struct BaseInodeInfo * info, nvmfs_ino_t parentIno, struct inode * vfsInode)
{
    info->mode = vfsInode->i_mode;
    info->atime = vfsInode->i_atime;
    info->ctime = vfsInode->i_ctime;
    info->mtime = vfsInode->i_mtime;
    info->uid = vfsInode->i_uid;
    info->gid = vfsInode->i_gid;
    info->parentIno = parentIno;
    info->linkCount = 1;
}

static int VfsInodeRebuild(struct super_block * sb, nvmfs_ino_t ino, struct inode * dirInode, struct inode ** inodePtr)
{
    struct inode * pInode;
    struct NvmfsInfo * fsInfo;
    struct BaseInodeInfo * inodeInfo;
    int err;

    fsInfo = sb->s_fs_info;
    err = InodeRebuild(&inodeInfo, &fsInfo->inodeTable, ino, &fsInfo->ppool, &fsInfo->bpool, &fsInfo->acc);
    if (err)
        return err;

    pInode = AllocAndInitVfsInode(sb, inodeInfo->mode, dirInode);
    if (pInode == NULL)
    {
        InodeUninit(inodeInfo, fsInfo);
        return -ENOMEM;
    }

    InitVfsInodeFromInodeInfo(pInode, inodeInfo);

    pInode->i_ino = inodeInfo->thisIno;
    pInode->i_blocks = InodeInfoGetPageNum(inodeInfo);
    pInode->i_size = InodeInfoGetSize(inodeInfo);
    pInode->i_private = inodeInfo;
    *inodePtr = pInode;
    return 0;
}

int VfsRootInodeFormat(struct super_block * sb, struct inode ** rootInodePtr, struct dentry ** rootDentryPtr)
{
    struct BaseInodeInfo * inodeInfo;
    struct NvmfsInfo * fsInfo;
    struct inode * pInode;
    struct dentry * pDentry;

    fsInfo = sb->s_fs_info;
    pInode = AllocAndInitVfsInode(sb, S_IFDIR, NULL);
    inodeInfo = InodeInfoAlloc(INODE_TYPE_DIR_FILE);
    BaseInodeInfoFormat(inodeInfo, 0, pInode);
    RootInodeFormat(inodeInfo, fsInfo);

    pInode->i_ino = 0;
    pInode->i_private = inodeInfo;
    pInode->i_size = 0;
    pInode->i_blocks = InodeInfoGetPageNum(inodeInfo);

    pDentry = d_make_root(pInode);
    *rootInodePtr = pInode;
    *rootDentryPtr = pDentry;
    return 0;
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
    parentIno = ((struct BaseInodeInfo *)(dirInode->i_private))->thisIno;

    pInode = AllocAndInitVfsInode(sb, mode, dirInode);
    inodeInfo = InodeInfoAlloc(ModeToType(mode));
    BaseInodeInfoFormat(inodeInfo, parentIno, pInode);
    err = InodeFormat(inodeInfo, fsInfo);

    pInode->i_ino = inodeInfo->thisIno;
    pInode->i_private = inodeInfo;
    pInode->i_size = 0;
    pInode->i_blocks = InodeInfoGetPageNum(inodeInfo);

    err = DirInodeInfoAddDentry(dirInode->i_private, inodeInfo->thisIno, (char *)vfsDentry->d_name.name,
                                vfsDentry->d_name.len + 1, type, &fsInfo->acc);
    if (err)
    {
        InodeDestroy(inodeInfo, fsInfo);
        iput(pInode);
        return err;
    }
    DEBUG_PRINT("inode create, name is %s, len is %ld, thisIno is %ld, parentIno is %ld\n", vfsDentry->d_name.name,
                (unsigned long)vfsDentry->d_name.len, inodeInfo->thisIno, parentIno);
    d_instantiate(vfsDentry, pInode);
    return 0;
}

static int VfsInodeCreate(struct inode * dirInode, struct dentry * vfsDentry, umode_t mode, bool excl)
{
    return VfsInodeCreateImpl(dirInode, vfsDentry, ModeToType(mode), mode);
}

static struct dentry * VfsInodeLookup(struct inode * dirInode, struct dentry * dentry, unsigned int flags)
{
    struct DirInodeInfo * dirInfo;
    int err;
    nvmfs_ino_t ino;
    struct NvmfsInfo * fsInfo;
    struct inode * pInode = NULL;

    dirInfo = dirInode->i_private;
    fsInfo = dirInode->i_sb->s_fs_info;

    DEBUG_PRINT("inode lookup, name is %s, len is %ld", dentry->d_name.name, (unsigned long)dentry->d_name.len);
    err = DirInodeInfoLookupDentryByName(dirInfo, &ino, (char *)dentry->d_name.name, dentry->d_name.len + 1,
                                         &fsInfo->acc);
    if (err)
    {
        if (err != -ENOENT)
            return ERR_PTR(err);
        return NULL;
    }

    err = VfsInodeRebuild(dirInode->i_sb, ino, dirInode, &pInode);
    DEBUG_PRINT("inode lookup success, pInode->parentIno is %ld, pInode->thisIno is %ld, dirInode->thisIno is %ld",
                ((struct BaseInodeInfo *)pInode->i_private)->parentIno,
                ((struct BaseInodeInfo *)pInode->i_private)->thisIno,
                ((struct BaseInodeInfo *)dirInode->i_private)->thisIno);
    if (err)
        return ERR_PTR(err);

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
    DEBUG_PRINT("remove file, name is %s", dentry->d_name.name);
    return VfsInodeRemoveImpl(dirInode, dentry);
}

static int VfsInodeMkdir(struct inode * dirInode, struct dentry * dentry, umode_t mode)
{
    return VfsInodeCreateImpl(dirInode, dentry, INODE_TYPE_DIR_FILE, S_IFDIR | mode);
}

static int VfsInodeRmdir(struct inode * dirInode, struct dentry * dentry)
{
    DEBUG_PRINT("remove dir, name is %s", dentry->d_name.name);
    return VfsInodeRemoveImpl(dirInode, dentry);
}

static int VfsInodeGetattr(const struct path * path, struct kstat * stat, u32 request_mask, unsigned int query_flags)
{
    struct inode * inode = d_inode(path->dentry);

    stat->dev = inode->i_sb->s_dev;
    stat->ino = inode->i_ino;
    stat->mode = inode->i_mode;
    stat->nlink = inode->i_nlink;
    stat->uid = inode->i_uid;
    stat->gid = inode->i_gid;
    // stat->rdev = inode->i_rdev;
    stat->size = i_size_read(inode);
    stat->atime = inode->i_atime;
    stat->mtime = inode->i_mtime;
    stat->ctime = inode->i_ctime;
    stat->blksize = SIZE_4K;
    stat->blocks = inode->i_blocks;

    if (IS_NOATIME(inode))
        stat->result_mask &= ~STATX_ATIME;
    if (IS_AUTOMOUNT(inode))
        stat->attributes |= STATX_ATTR_AUTOMOUNT;

    DEBUG_PRINT("getattr called, inode size is %ld", (unsigned long)stat->size);
    return 0;
}

struct inode_operations NvmfsDirInodeOps = {.create = VfsInodeCreate,
                                            .lookup = VfsInodeLookup,
                                            .unlink = VfsInodeUnlink,
                                            .mkdir = VfsInodeMkdir,
                                            .rmdir = VfsInodeRmdir};

struct inode_operations NvmfsFileInodeOps = {.getattr = VfsInodeGetattr};

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