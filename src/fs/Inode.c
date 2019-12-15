#include "DirInodeInfo.h"
#include "FileInodeInfo.h"
#include "FsInfo.h"
#include "Inode.h"
#include "InodeTable.h"
#include "InodeType.h"
#include "Log.h"
#include "common.h"
#include <linux/slab.h>

struct BaseInodeInfo * InodeInfoAlloc(UINT8 type)
{
    struct BaseInodeInfo * info = NULL;
    switch (type)
    {
    case INODE_TYPE_REGULAR_FILE:
        info = kmalloc(sizeof(struct FileInodeInfo), GFP_KERNEL);
        break;
    case INODE_TYPE_DIR_FILE:
        info = kmalloc(sizeof(struct DirInodeInfo), GFP_KERNEL);
        break;
    case INODE_TYPE_LINK_FILE:
        break;
    }
    info->type = type;
    return info;
}

void InodeInfoFree(struct BaseInodeInfo * info)
{
    kfree(info);
}

static int InodeFormatImpl(struct BaseInodeInfo * info, struct NvmfsInfo * fsInfo, logic_addr_t * firstArea)
{
    int err = 0;

    switch (info->type)
    {
    case INODE_TYPE_REGULAR_FILE:
        err =
            FileInodeInfoFormat((struct FileInodeInfo *)info, &fsInfo->ppool, &fsInfo->bpool, firstArea, &fsInfo->acc);
        break;
    case INODE_TYPE_DIR_FILE:
        err = DirInodeInfoFormat((struct DirInodeInfo *)info, &fsInfo->ppool, firstArea, &fsInfo->acc);
        break;
    case INODE_TYPE_LINK_FILE:
        break;
    }
    return err;
}

int RootInodeFormat(struct BaseInodeInfo * info, struct NvmfsInfo * fsInfo)
{
    int err;
    nvmfs_ino_t ino = 0;
    logic_addr_t firstArea;

    info->thisIno = ino;
    err = InodeFormatImpl(info, fsInfo, &firstArea);
    if (err)
        return err;

    InodeTableUpdateInodeAddr(&fsInfo->inodeTable, ino, firstArea);
    return 0;
}

int InodeFormat(struct BaseInodeInfo * info, struct NvmfsInfo * fsInfo)
{
    int err;
    nvmfs_ino_t ino;
    logic_addr_t firstArea;

    ino = InodeTableGetIno(&fsInfo->inodeTable);
    if (ino == invalid_nvmfs_ino)
        return -ENOSPC;

    info->thisIno = ino;
    err = InodeFormatImpl(info, fsInfo, &firstArea);
    if (err)
    {
        InodeTablePutIno(&fsInfo->inodeTable, ino);
        return err;
    }

    InodeTableUpdateInodeAddr(&fsInfo->inodeTable, ino, firstArea);
    return 0;
}

void InodeUninit(struct BaseInodeInfo * info, struct NvmfsInfo * fsInfo)
{
    UINT8 type = info->type;

    switch (type)
    {
    case INODE_TYPE_REGULAR_FILE:
        FileInodeInfoUninit((struct FileInodeInfo *)info);
        break;
    case INODE_TYPE_DIR_FILE:
        DirInodeInfoUninit((struct DirInodeInfo *)info);
        break;
    case INODE_TYPE_LINK_FILE:
        break;
    }
}

void InodeDestroy(struct BaseInodeInfo * info, struct NvmfsInfo * fsInfo)
{
    UINT8 type = info->type;

    InodeTablePutIno(&fsInfo->inodeTable, info->thisIno);
    switch (type)
    {
    case INODE_TYPE_REGULAR_FILE:
        FileInodeInfoDestroy((struct FileInodeInfo *)info, &fsInfo->acc);
        break;
    case INODE_TYPE_DIR_FILE:
        DirInodeInfoDestroy((struct DirInodeInfo *)info, &fsInfo->acc);
        break;
    case INODE_TYPE_LINK_FILE:
        break;
    }
}

void InodeRecovery(logic_addr_t inodeAddr, struct FsConstructor * ctor, struct CircularBuffer * cb,
                   struct NVMAccesser * acc)
{
    UINT8 type;
    LogRecoveryReadReserveData(inodeAddr, &type, sizeof(type), offsetof(struct BaseInodeInfo, type), acc);
    switch (type)
    {
    case INODE_TYPE_REGULAR_FILE:
        FileInodeInfoRecovery(inodeAddr, ctor, cb, acc);
        break;
    case INODE_TYPE_DIR_FILE:
        DirInodeInfoRecovery(inodeAddr, ctor, cb, acc);
        break;
    case INODE_TYPE_LINK_FILE:
        break;
    }
}

int InodeRebuild(struct BaseInodeInfo ** infoPtr, struct InodeTable * pTable, nvmfs_ino_t ino, struct PagePool * ppool,
                 struct BlockPool * bpool, struct NVMAccesser * acc)
{
    UINT8 type;
    logic_addr_t inodeAddr;
    struct BaseInodeInfo * info;

    inodeAddr = InodeTableGetInodeAddr(pTable, ino);
    if (inodeAddr == invalid_nvm_addr)
        return -ENOENT;
    LogRebuildReadReserveData(inodeAddr, &type, sizeof(type), offsetof(struct BaseInodeInfo, type), acc);
    info = InodeInfoAlloc(type);
    if (!info)
        return -ENOMEM;
    switch (type)
    {
    case INODE_TYPE_REGULAR_FILE:
        FileInodeInfoRebuild((struct FileInodeInfo *)info, inodeAddr, ppool, bpool, acc);
        break;
    case INODE_TYPE_DIR_FILE:
        DirInodeInfoRebuild((struct DirInodeInfo *)info, inodeAddr, ppool, acc);
        break;
    case INODE_TYPE_LINK_FILE:
        break;
    }

    *infoPtr = info;
    return 0;
}

UINT64 InodeInfoGetPageNum(struct BaseInodeInfo * info)
{
    switch (info->type)
    {
    case INODE_TYPE_REGULAR_FILE:
        return FileInodeInfoGetPageNum((struct FileInodeInfo *)info);
        break;
    case INODE_TYPE_DIR_FILE:
        return DirInodeInfoGetPageNum((struct DirInodeInfo *)info);
        break;
    case INODE_TYPE_LINK_FILE:
        break;
    }
    return 0;
}

UINT64 InodeInfoGetSize(struct BaseInodeInfo * info)
{
    switch (info->type)
    {
    case INODE_TYPE_REGULAR_FILE:
        return FileInodeInfoGetMaxLen((struct FileInodeInfo *)info);
        break;
    case INODE_TYPE_DIR_FILE:
        return DirInodeInfoGetPageNum((struct DirInodeInfo *)info) * PAGE_SIZE;
        break;
    case INODE_TYPE_LINK_FILE:
        break;
    }
    return 0;
}

void BaseInodeInfoPrint(struct BaseInodeInfo * info)
{
    DEBUG_PRINT("ino is %ld, parent ino is %ld, type is %d", info->thisIno, info->parentIno, info->type);
}