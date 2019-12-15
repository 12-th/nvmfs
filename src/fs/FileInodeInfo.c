#include "BlockPool.h"
#include "FileInodeInfo.h"
#include "FsConstructor.h"
#include "Inode.h"
#include "PagePool.h"
#include "common.h"
#include <linux/slab.h>

int FileInodeInfoFormat(struct FileInodeInfo * info, struct PagePool * ppool, struct BlockPool * bpool,
                        logic_addr_t * firstArea, struct NVMAccesser * acc)
{
    int err;

    err = LogFormat(&info->log, ppool, sizeof(struct BaseInodeInfo), acc);
    if (err)
        return err;
    FileDataManagerInit(&info->manager, bpool, ppool);
    LogWriteReserveData(&info->log, sizeof(struct BaseInodeInfo), 0, &info->baseInfo, acc);
    *firstArea = LogFirstArea(&info->log);
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

INT64 FileInodeInfoReadData(struct FileInodeInfo * info, void * buffer, UINT64 size, UINT64 fileStart,
                            struct NVMAccesser * acc)
{
    MergeFileInodeData(info, acc);
    return FileDataManagerReadData(&info->manager, buffer, size, fileStart, acc);
}

INT64 FileInodeInfoWriteData(struct FileInodeInfo * info, void * buffer, UINT64 size, UINT64 fileStart,
                             struct NVMAccesser * acc)
{
    int err;
    MergeFileInodeData(info, acc);
    err = FileDataManagerWriteData(&info->manager, buffer, size, fileStart, &info->log, acc);
    if (err)
        return err;
    return size;
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

void FileInodeInfoRecovery(logic_addr_t inodeAddr, struct FsConstructor * ctor, struct CircularBuffer * cb,
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

void FileInodeInfoRebuild(struct FileInodeInfo * info, logic_addr_t inodeAddr, struct PagePool * ppool,
                          struct BlockPool * bpool, struct NVMAccesser * acc)
{
    struct FileInodeRebuildContex context = {.info = info};
    FileDataManagerRebuildBegin(&info->manager, ppool, bpool);
    LogRebuildReadReserveData(inodeAddr, &info->baseInfo, sizeof(struct BaseInodeInfo), 0, acc);
    LogRebuildBegin(&info->log, inodeAddr, sizeof(struct BaseInodeInfo));
    LogRebuild(&info->log, FileInodeRebuildCleanupOps, &context, acc);
    FileDataManagerRebuildEnd(&info->manager);
    LogRebuildEnd(&info->log, acc);
}

int FileInodeIsInfoSame(struct FileInodeInfo * info1, struct FileInodeInfo * info2)
{
    return LogIsSame(&info1->log, &info2->log) && FileDataManagerIsSame(&info1->manager, &info2->manager);
}

void FileInodePrintInfo(struct FileInodeInfo * info)
{
    BaseInodeInfoPrint(&info->baseInfo);
    LogPrintInfo(&info->log);
    FileDataManagerPrintInfo(&info->manager);
}