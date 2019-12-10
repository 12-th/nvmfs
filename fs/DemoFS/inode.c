#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/parser.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/time.h>

#define DEMOFS_MAGIC 0x3289480a

extern const struct inode_operations demofsFileInodeOps;
extern const struct file_operations demofsFileOps;

static const struct address_space_operations demofsAddrSpaceOps = {
    .readpage = simple_readpage,
    .write_begin = simple_write_begin,
    .write_end = simple_write_end,
};

static const struct inode_operations demofsDirInodeOps;

struct inode * demofs_alloc_inode(struct super_block * pSuperBlock, const struct inode * pDirNode, umode_t mode,
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

        pInode->i_mapping->a_ops = &demofsAddrSpaceOps;
        mapping_set_gfp_mask(pInode->i_mapping, GFP_HIGHUSER);
        mapping_set_unevictable(pInode->i_mapping);

        pInode->i_atime = pInode->i_mtime = pInode->i_ctime = current_time(pInode);

        switch (mode & S_IFMT)
        {
        case S_IFREG:
            pInode->i_op = &demofsFileInodeOps;
            pInode->i_fop = &demofsFileOps;
            break;
        case S_IFDIR:
            pInode->i_op = &demofsDirInodeOps;
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

static int demofs_mknod(struct inode * pDirNode, struct dentry * pDentry, umode_t mode, dev_t dev)
{
    struct inode * pInode = demofs_alloc_inode(pDirNode->i_sb, pDirNode, mode, dev);

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

static int demofs_create(struct inode * pDirNode, struct dentry * pDentry, umode_t mode, bool excl)
{
    return demofs_mknod(pDirNode, pDentry, mode | S_IFREG, 0);
}

static int demofs_mkdir(struct inode * pDirNode, struct dentry * pDentry, umode_t mode)
{
    int res = demofs_mknod(pDirNode, pDentry, mode | S_IFDIR, 0);

    // increase parent dir inode's reference count
    if (!res)
        inc_nlink(pDirNode);

    return res;
}

static int demofs_symlink(struct inode * pNodeDir, struct dentry * pDentry, const char * symname)
{
    int err;
    struct inode * pInode = demofs_alloc_inode(pNodeDir->i_sb, pNodeDir, S_IFLNK | S_IRWXUGO, 0);
    if (pInode)
    {
        // copy the symname to pagecache
        err = page_symlink(pInode, symname, strlen(symname) + 1);
        if (!err)
        {
            d_instantiate(pDentry, pInode);
            dget(pDentry);

            pNodeDir->i_mtime = pNodeDir->i_ctime = current_time(pNodeDir);
        }
        else
            iput(pInode);

        return err;
    }

    return -ENOSPC;
}

static const struct inode_operations demofsDirInodeOps = {.create = demofs_create,
                                                          .lookup = simple_lookup,
                                                          .link = simple_link,
                                                          .unlink = simple_unlink,
                                                          .symlink = demofs_symlink,
                                                          .mkdir = demofs_mkdir,
                                                          .rmdir = simple_rmdir,
                                                          .mknod = demofs_mknod,
                                                          .rename = simple_rename};

static const struct super_operations demofsSuperOps = {.statfs = simple_statfs, .drop_inode = generic_delete_inode};

struct STDemoFSMountOpt
{
    umode_t m_mode;
};

enum
{
    OptMode,
    OptErr
};

static const match_table_t tokens = {{OptMode, "mode=%o"}, {OptErr, NULL}};

struct STDemoFSInfo
{
    struct STDemoFSMountOpt m_mountOpt;
};

// parse mount options
int demofs_parse_options(char * options, struct STDemoFSMountOpt * pMountOpt)
{
    substring_t args[MAX_OPT_ARGS];
    int option;
    int token;
    char * curPos;

    while ((curPos = strsep(&options, ",")) != NULL)
    {
        if (!*curPos)
            continue;

        // find valid option defined in tokens, return the option's type and put arguments in args
        token = match_token(curPos, tokens, args);
        switch (token)
        {
        case OptMode:
            // parse the argument
            if (match_octal(&args[0], &option))
                return -EINVAL;
            pMountOpt->m_mode = option & S_IALLUGO;
        default:
            break;
        }
    }

    return 0;
}

// initialize superblock
int demofs_fill_sb(struct super_block * pSuperBlock, void * options, int silent)
{
    struct STDemoFSInfo * pFSInfo;
    struct inode * pInode;
    int err;

    pFSInfo = kzalloc(sizeof(struct STDemoFSInfo), GFP_KERNEL);
    if (!pFSInfo)
        return -ENOMEM;
    err = demofs_parse_options(options, &pFSInfo->m_mountOpt);
    if (err)
        return err;

    pSuperBlock->s_fs_info = pFSInfo;

    pSuperBlock->s_maxbytes = MAX_LFS_FILESIZE;
    pSuperBlock->s_blocksize = PAGE_SIZE;
    pSuperBlock->s_blocksize_bits = PAGE_SHIFT;
    pSuperBlock->s_magic = DEMOFS_MAGIC;
    pSuperBlock->s_op = &demofsSuperOps;
    pSuperBlock->s_time_gran = 1;

    pInode = demofs_alloc_inode(pSuperBlock, NULL, S_IFDIR | pFSInfo->m_mountOpt.m_mode, 0);
    if (!pInode)
        return -ENOMEM;

    // allocates the root dentry
    pSuperBlock->s_root = d_make_root(pInode);
    if (!(pSuperBlock->s_root))
    {
        iput(pInode);
        return -ENOMEM;
    }

    return 0;
}

// mount superblock, will finally call demofs_fill_sb
struct dentry * demofs_mount(struct file_system_type * pFSType, int flags, const char * devName, void * options)
{
    // use mount_nodev because our file system is not on physical block device
    return mount_nodev(pFSType, flags, options, demofs_fill_sb);
}

// unmount superblock
static void demofs_kill_sb(struct super_block * pSuperBlock)
{
    kfree(pSuperBlock->s_fs_info);
    kill_litter_super(pSuperBlock);
}

struct file_system_type demofsType = {
    .owner = THIS_MODULE, .name = "demofs", .mount = demofs_mount, .kill_sb = demofs_kill_sb};

// register filesystem
static int __init init_demofs(void)
{
    return register_filesystem(&demofsType);
}

static void __exit clean_demofs(void)
{
    unregister_filesystem(&demofsType);
}

MODULE_LICENSE("GPL");

module_init(init_demofs);
module_exit(clean_demofs);
