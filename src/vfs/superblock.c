#include "inode.h"
#include "nvmfs.h"
#include "superblock.h"
#include <linux/init.h>
#include <linux/parser.h>
#include <linux/slab.h>

static const match_table_t tokens = {{OptMode, "mode=%o"}, {OptErr, NULL}};

#define NVMFS_MAGIC 0xbadcafe0

const struct super_operations nvmfsSuperOps;

static int nvmfs_parse_options(char * options, struct STDemoFSMountOpt * pMountOpt)
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

static int NvmfsInfoInit(struct NvmfsInfo * info)
{
    WearLevelerInit(&info->wl);
}

int NvmfsFillSuperBlock(struct super_block * sb, void * options, int silent)
{
    struct NvmfsInfo * info;
    struct inode * pInode;
    int err;

    info = kzalloc(sizeof(struct NvmfsInfo), GFP_KERNEL);
    NvmfsInfoInit(info);
    if (!info)
        return -ENOMEM;
    err = nvmfs_parse_options(options, &info->m_mountOpt);
    if (err)
        return err;

    sb->s_fs_info = info;
    sb->s_maxbytes = MAX_LFS_FILESIZE;
    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    sb->s_magic = NVMFS_MAGIC;
    sb->s_op = &nvmfsSuperOps;
    sb->s_time_gran = 1;
    sb->s_type = &nvmfs;

    pInode = nvmfs_alloc_inode(sb, NULL, S_IFDIR | info->m_mountOpt.m_mode, 0);
    if (!pInode)
        return -ENOMEM;

    // allocates the root dentry
    sb->s_root = d_make_root(pInode);
    if (!(sb->s_root))
    {
        iput(pInode);
        return -ENOMEM;
    }

    info->inodeSlab = kmem_cache_create("nvmfsInodeSlab", sizeof(struct NvmfsInode), 0, 0, NULL);
    WearLevelerInit(&info->wl);
    NvmAccesserInit(&info->acc, &info->wl);
    BlockPoolInit(&info->blockPool);
    // scan to find out free extents

    PagePoolGlobalInit();
    PagePoolInit(&info->inodePool, &info->blockPool, info->acc);

    return 0;
}

int NvmfsKillSuperBlock(struct super_block * sb)
{
    kmem_cache_destroy(sb->s_fs_info->inodeSlab);
    kfree(sb->s_fs_info);
    kill_litter_super(sb);
}

// const struct super_operations nvmfsSuperOps = {.alloc_inode =.statfs = simple_statfs,
//                                                .drop_inode = generic_delete_inode};
const struct super_operations nvmfsSuperOps = {.alloc_inode = NvmfsAllocInode,
                                               .destroy_inode = NvmfsDestroyInode,
                                               .drop_inode = generic_delete_inode,
                                               .statfs = simple_statfs};