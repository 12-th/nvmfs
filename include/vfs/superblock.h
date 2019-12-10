#ifndef NVMFS_SUPER_BLOCK_H
#define NVMFS_SUPER_BLOCK_H

#include "BlockPool.h"
#include "InodeTable.h"
#include "NVMAccesser.h"
#include "PagePool.h"
#include "Types.h"
#include "WearLeveler.h"
#include <linux/fs.h>
#include <linux/slab.h>

struct NvmfsSuperBlock
{
    UINT64 nvmSize;
    logic_addr_t inodeTable;
    logic_addr_t root;
    logic_addr_t logArea;
};

struct NvmfsMountOpt
{
    umode_t m_mode;
};

enum
{
    OptMode,
    OptErr
};

struct NvmfsInfo
{
    struct NvmfsMountOpt m_mountOpt;
    struct WearLeveler wl;
    struct kmem_cache * inodeSlab;
    struct PagePool inodePool;
    struct BlockPool blockPool;
    struct NVMAccesser acc;
    struct InodeTable inodeTable;
};

extern const struct super_operations nvmfsSuperOps;

int NvmfsFillSuperBlock(struct super_block * sb, void * options, int silent);
int NvmfsKillSuperBlock(struct super_block * sb);

int nvmfs_fill_sb(struct super_block * pSuperBlock, void * options, int silent);
void nvmfs_kill_sb(struct super_block * pSuperBlock);

#endif