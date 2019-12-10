#include "BlockPool.h"
#include "FileInode.h"
#include "FsConstructor.h"
#include "Inode.h"
#include "PagePool.h"
#include <linux/slab.h>

int FileInodeInfoFormat(struct FileInodeInfo * info, struct PagePool * ppool, struct BlockPool * bpool,
                        struct NvmInode * inodeData, struct NVMAccesser * acc)
{
    int err;

    err = LogFormat(&info->log, ppool, sizeof(struct NvmInode), acc);
    if (err)
        return err;
    FileDataManagerInit(&info->manager, bpool, ppool);
    return 0;
}

void FileInodeInfoUninit(struct FileInodeInfo * info)
{
    LogUninit(&info->log);
    FileDataManagerUninit(&info->manager);
}

void FileInodeInfoDestroy(struct FileInodeInfo * info, struct NVMAccesser * acc)
{
    LogDestroy(&info->log, acc, info->manager.ppool);
    FileDataManagerDestroy(&info->manager);
}

static void MergeFileInodeData(struct FileInodeInfo * info, struct NVMAccesser * acc)
{
}

void FileInodeInfoReadData(struct FileInodeInfo * info, void * buffer, UINT64 size, UINT64 fileStart,
                           struct NVMAccesser * acc)
{
    MergeFileInodeData(info, acc);
    FileDataManagerReadData(&info->manager, buffer, size, fileStart, acc);
}

void FileInodeInfoWriteData(struct FileInodeInfo * info, void * buffer, UINT64 size, UINT64 fileStart,
                            struct NVMAccesser * acc)
{
    MergeFileInodeData(info, acc);
    FileDataManagerWriteData(&info->manager, buffer, size, fileStart, &info->log, acc);
}

//-------------- recovery-------------------

struct FileInodeRecoveryContex
{
    struct FsConstructor * ctor;
};

static void * FileInodeRecoveryAddNewSpacePrepare(UINT32 * size, void * data)
{
    static struct InodeLogAddNewSpaceEntry entry;
    (void)(data);
    return &entry;
}

static void FileInodeRecoveryAddNewSpaceCleanup(UINT8 type, UINT32 size, void * buffer, logic_addr_t entryReadStartAddr,
                                                logic_addr_t entryReadEndAddr, void * data)
{
    struct InodeLogAddNewSpaceEntry * entry;
    struct FileInodeRecoveryContex * contex;
    contex = data;
    entry = buffer;
    if (entry->size == SIZE_4K)
        FsConstructorNotifyPageBusy(contex->ctor, entry->addr);
    else
        FsConstructorNotifyBlockBusy(contex->ctor, entry->addr, entry->size / SIZE_2M);
}

struct LogCleanupOps FileInodeRecoveryCleanupOps[] = {
    {NULL, NULL}, {FileInodeRecoveryAddNewSpacePrepare, FileInodeRecoveryAddNewSpaceCleanup}, {NULL, NULL}};

void FileInodeRecovery(logic_addr_t inodeAddr, struct FsConstructor * ctor, struct CircularBuffer * cb,
                       struct NVMAccesser * acc)
{
    struct FileInodeRecoveryContex contex = {ctor};
    struct Log log;

    LogRecoveryInit(&log, inodeAddr);
    LogRecovery(&log, ctor, FileInodeRecoveryCleanupOps, &contex, acc);
    LogRecoveryUninit(&log);
}

// int FileInodeInfoAllocateData(struct FileInodeInfo * info)
// {
// }

//---------------------rebuild----------------

struct FileInodeRebuildContex
{
    struct InodeLogWriteDataEntry extentEntry;
    struct InodeLogAddNewSpaceEntry spaceEntry;
    struct FileInodeInfo * info;
};

static void * FileInodeRebuildWriteDataPrepare(UINT32 * size, void * data)
{
    struct FileInodeRebuildContex * context;

    context = data;
    return &context->extentEntry;
}

static void FileInodeRebuildWriteDataCleanup(UINT8 type, UINT32 size, void * buffer, logic_addr_t entryReadStartAddr,
                                             logic_addr_t entryReadEndAddr, void * data)
{
    struct FileInodeRebuildContex * context;
    struct InodeLogWriteDataEntry * entry;

    context = data;
    entry = buffer;
    FileDataManagerRebuildAddExtent(&context->info->manager, entry->addr, entry->size, entry->fileStart);
}

static void * FileInodeRebuildAddNewSpacePrepare(UINT32 * size, void * data)
{
    struct FileInodeRebuildContex * context;

    context = data;
    return &context->spaceEntry;
}

static void FileInodeRebuildAddNewSpaceCleanup(UINT8 type, UINT32 size, void * buffer, logic_addr_t entryReadStartAddr,
                                               logic_addr_t entryReadEndAddr, void * data)
{
    struct FileInodeRebuildContex * context;
    struct InodeLogAddNewSpaceEntry * entry;

    context = data;
    entry = buffer;
    FileDataManagerRebuildAddSpace(&context->info->manager, entry->addr, entry->size);
}

struct LogCleanupOps FileInodeRebuildCleanupOps[] = {
    {NULL, NULL},
    {FileInodeRebuildAddNewSpacePrepare, FileInodeRebuildAddNewSpaceCleanup},
    {FileInodeRebuildWriteDataPrepare, FileInodeRebuildWriteDataCleanup}};

void FileInodeRebuild(struct FileInodeInfo * info, logic_addr_t inodeAddr, struct PagePool * ppool,
                      struct BlockPool * bpool, struct NVMAccesser * acc)
{
    struct FileInodeRebuildContex context = {.info = info};
    info->addr = inodeAddr;
    FileDataManagerRebuildBegin(&info->manager, ppool, bpool);
    LogRebuildBegin(&info->log, inodeAddr, sizeof(struct NvmInode));
    LogRebuild(&info->log, FileInodeRebuildCleanupOps, &context, acc);
    FileDataManagerRebuildEnd(&info->manager);
    LogRebuildEnd(&info->log, acc);
}