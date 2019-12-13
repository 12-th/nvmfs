#include "DirInodeInfo.h"
#include "FileInodeInfo.h"
#include "FsInfo.h"
#include "Inode.h"
#include "InodeTable.h"
#include "Log.h"
#include <linux/slab.h>

static void BaseInodeInfoFormat(struct BaseInodeInfo * baseInfo, UINT8 type, nvmfs_ino_t thisIno, nvmfs_ino_t parentIno)
{
    baseInfo->type = type;
    baseInfo->thisIno = thisIno;
    baseInfo->parentIno = parentIno;
    baseInfo->linkCount = 1;
    // #warning set time
}

int RootInodeFormat(struct NvmfsInfo * fsInfo)
{
    struct DirInodeInfo * info;
    logic_addr_t firstArea;
    int err;

    info = kmalloc(sizeof(struct DirInodeInfo), GFP_KERNEL);
    if (!info)
        return -ENOMEM;
    BaseInodeInfoFormat((struct BaseInodeInfo *)info, INODE_TYPE_DIR_FILE, 0, 0);
    err = DirInodeInfoFormat((struct DirInodeInfo *)info, &fsInfo->ppool, &firstArea, &fsInfo->acc);
    if (err)
    {
        kfree(info);
        return err;
    }
    InodeTableUpdateInodeAddr(&fsInfo->inodeTable, 0, firstArea);
    DirInodeInfoUninit(info);
    kfree(info);
    return 0;
}

int InodeFormat(struct BaseInodeInfo ** infoPtr, struct NvmfsInfo * fsInfo, UINT8 type, nvmfs_ino_t parentIno)
{
    struct BaseInodeInfo * info = NULL;
    int err;
    nvmfs_ino_t ino;
    logic_addr_t firstArea;

    ino = InodeTableGetIno(&fsInfo->inodeTable);
    if (ino == invalid_nvmfs_ino)
        return -ENOSPC;

    switch (type)
    {
    case INODE_TYPE_REGULAR_FILE:
        info = kmalloc(sizeof(struct FileInodeInfo), GFP_KERNEL);
        if (!info)
            return -ENOMEM;
        BaseInodeInfoFormat(info, type, ino, parentIno);
        err =
            FileInodeInfoFormat((struct FileInodeInfo *)info, &fsInfo->ppool, &fsInfo->bpool, &firstArea, &fsInfo->acc);
        if (err)
        {
            kfree(info);
            return err;
        }
        break;
    case INODE_TYPE_DIR_FILE:
        info = kmalloc(sizeof(struct DirInodeInfo), GFP_KERNEL);
        if (!info)
            return -ENOMEM;
        BaseInodeInfoFormat(info, type, ino, parentIno);
        err = DirInodeInfoFormat((struct DirInodeInfo *)info, &fsInfo->ppool, &firstArea, &fsInfo->acc);
        if (err)
        {
            kfree(info);
            return err;
        }
        break;
    case INODE_TYPE_LINK_FILE:
        break;
    }

    InodeTableUpdateInodeAddr(&fsInfo->inodeTable, ino, firstArea);
    *infoPtr = info;
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
    void * data = NULL;

    inodeAddr = InodeTableGetInodeAddr(pTable, ino);
    if (inodeAddr == invalid_nvm_addr)
        return -ENOENT;
    LogRebuildReadReserveData(inodeAddr, &type, sizeof(type), offsetof(struct BaseInodeInfo, type), acc);
    switch (type)
    {
    case INODE_TYPE_REGULAR_FILE:
        data = kmalloc(sizeof(struct FileInodeInfo), GFP_KERNEL);
        if (!data)
            return -ENOMEM;
        FileInodeInfoRebuild((struct FileInodeInfo *)data, inodeAddr, ppool, bpool, acc);
        break;
    case INODE_TYPE_DIR_FILE:
        data = kmalloc(sizeof(struct DirInodeInfo), GFP_KERNEL);
        if (!data)
            return -ENOMEM;
        DirInodeInfoRebuild((struct DirInodeInfo *)data, inodeAddr, ppool, acc);
        break;
    case INODE_TYPE_LINK_FILE:
        break;
    }
    *infoPtr = data;
    return 0;
}