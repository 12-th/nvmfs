#ifndef FS_INFO_H
#define FS_INFO_H

#include "BlockPool.h"
#include "InodeTable.h"
#include "NVMAccesser.h"
#include "PagePool.h"
#include "Types.h"
#include "WearLeveler.h"

struct NvmfsInfo
{
    // struct NvmfsMountOpt m_mountOpt;
    struct WearLeveler wl;
    struct PagePool ppool;
    struct BlockPool bpool;
    struct NVMAccesser acc;
    struct InodeTable inodeTable;
};

struct NvmfsSuperBlock
{
    UINT64 nvmSizeBits;
    UINT64 inodeTableAddr;
    UINT64 inodeTableSize;
    UINT64 totalBlockNum;
};

int NvmfsInfoFormat(struct NvmfsInfo * info, mode_t rootMode);
int NvmfsInfoInit(struct NvmfsInfo * info);
int NvmfsInfoUninit(struct NvmfsInfo * info);

#endif