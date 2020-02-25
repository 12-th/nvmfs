#include "DirInodeInfo.h"
#include "FsConfig.h"
#include "FsConstructor.h"
#include "FsInfo.h"
#include "NVMOperations.h"
#include "common.h"
#include <linux/slab.h>

int NvmfsInfoInit(struct NvmfsInfo * info)
{
    struct FsConstructor ctor;
    struct NvmfsSuperBlock nvmsb;
    NVMInit(FS_NVM_SIZE);
    NVMRead(0, sizeof(nvmsb), &nvmsb);
    FsConstructorInit(&ctor, &info->bpool, &info->ppool, &info->wl, &info->inodeTable, nvmsb.inodeTableAddr,
                      nvmsb.inodeTableSize, nvmsb.totalBlockNum, FS_NVM_SIZE_BIT, FS_WEARLEVELER_RESERVE_SIZE);
    FsConstructorBegin(&ctor);
    FsConstructorRun(&ctor, 0);
    FsConstructorEnd(&ctor);
    return 0;
}

static void FsInfoFormat(struct NvmfsInfo * info)
{
    struct ExtentContainer container;

    NVMInit(FS_NVM_SIZE);
    WearLevelerFormat(&info->wl, FS_NVM_SIZE_BIT, FS_WEARLEVELER_RESERVE_SIZE);
    NVMAccesserInit(&info->acc, &info->wl);
    InodeTableFormat(&info->inodeTable, WearLevelerReserveDataAddrQuery(&info->wl), FS_INODE_TABLE_SIZE);
    BlockPoolInit(&info->bpool, &info->acc);
    ExtentContainerInit(&container, GFP_KERNEL);
    ExtentContainerAppend(&container, 0, WearLevelerLogicBlockNumQuery(&info->wl), GFP_KERNEL);
    BlockPoolExtentPut(&info->bpool, &container);
    ExtentContainerUninit(&container);
    PagePoolInit(&info->ppool, &info->bpool, info->acc);
}
int NvmfsInfoFormat(struct NvmfsInfo * info, mode_t rootMode)
{
    FsInfoFormat(info);
    return 0;
}

int NvmfsInfoUninit(struct NvmfsInfo * info)
{
    InodeTableUninit(&info->inodeTable);
    PagePoolUninit(&info->ppool);
    BlockPoolUninit(&info->bpool);
    NVMAccesserUninit(&info->acc);
    WearLevelerUninit(&info->wl);
    NVMUninit();
    return 0;
}
