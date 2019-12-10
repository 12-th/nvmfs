#include "DirInode.h"
#include "Inode.h"

int DirInodeInfoFormat(struct DirInodeInfo * info, struct PagePool * ppool, struct NvmInode * inodeData,
                       struct NVMAccesser * acc)
{
    int err;
    info->pool = ppool;
    DirFileDentryCacheInit(&info->cache);
    err = LogFormat(&info->log, ppool, sizeof(struct NvmInode), acc);
    if (err)
    {
        DirFileDentryCacheUninit(&info->cache);
        return err;
    }
    LogWriteReserveData(&info->log, sizeof(struct NvmInode), 0, inodeData, acc);
    return 0;
}

void DirInodeInfoUninit(struct DirInodeInfo * info)
{
    DirFileDentryCacheUninit(&info->cache);
    LogUninit(&info->log);
}

void DirInodeInfoDestroy(struct DirInodeInfo * info, struct NVMAccesser * acc)
{
    DirFileDentryCacheUninit(&info->cache);
    LogDestroy(&info->log, acc, info->pool);
}

int DirInodeInfoLookupDentry(struct DirInodeInfo * info, nvmfs_ino_t ino)
{
    struct DentryNode * node;
    node = DirFileDentryCacheLookup(&info->cache, ino);
    if (!node)
        return -EINVAL;
    return 0;
}

int DirInodeAddDentry(struct DirInodeInfo * info, nvmfs_ino_t ino, char * name, UINT64 len, UINT8 type,
                      struct NVMAccesser * acc)
{
    struct DirInodeLogAddDentryEntry entry = {.ino = ino, .type = type, .nameLen = len};
    struct LogStepWriteHandle handle;
    logic_addr_t nameAddr;
    int err;

    err = LogStepWriteBegin(&info->log, &handle, INODE_LOG_ADD_DENTRY, sizeof(entry) + len, info->pool, acc);
    if (err)
        return err;
    LogStepWriteContinue(&handle, &entry, sizeof(entry), NULL, acc);
    LogStepWriteContinue(&handle, name, len, &nameAddr, acc);
    LogStepWriteEnd(&handle, acc);
    err = DirFileDentryCacheAppendDentry(&info->cache, ino, type, nameAddr, len);
    return err;
}

int DirInodeRemoveDentry(struct DirInodeInfo * info, nvmfs_ino_t ino, struct NVMAccesser * acc)
{
    struct DirInodeLogRemoveDentryEntry entry = {.ino = ino};
    int err;

    if (DirFileDentryCacheLookup(&info->cache, ino) == NULL)
        return -EINVAL;

    err = LogWrite(&info->log, sizeof(entry), &entry, INODE_LOG_REMOVE_DENTRY, NULL, info->pool, acc);
    if (err)
        return err;
    DirFileDentryCacheRemoveDentry(&info->cache, ino);
    return 0;
}

//----------------recovery------------------------

static void * DirInodeRecoveryAddDentryPrepare(UINT32 * size, void * data)
{
    static struct DirInodeLogAddDentryEntry entry;

    (void)(data);
    *size = sizeof(entry);
    return &entry;
}

static void DirInodeRecoveryAddDentryCleanup(UINT8 type, UINT32 size, void * buffer, logic_addr_t entryReadStartAddr,
                                             logic_addr_t entryReadEndAddr, void * data)
{
    struct DirFileDentryRecoveryCache * cache;
    struct DirInodeLogAddDentryEntry * entry;

    cache = data;
    entry = buffer;
    DirFileDentryRecoveryCacheAdd(cache, entry->ino);
}

static void * DirInodeRecoveryRemoveDentryPrepare(UINT32 * size, void * data)
{
    static struct DirInodeLogRemoveDentryEntry entry;

    return &entry;
}

static void DirInodeRecoveryRemoveDentryCleanup(UINT8 type, UINT32 size, void * buffer, logic_addr_t entryReadStartAddr,
                                                logic_addr_t entryReadEndAddr, void * data)
{
    struct DirFileDentryRecoveryCache * cache;
    struct DirInodeLogAddDentryEntry * entry;

    cache = data;
    entry = buffer;
    DirFileDentryRecoveryCacheRemove(cache, entry->ino);
}

static struct LogCleanupOps DirInodeRecoveryCleanupOps[] = {
    {NULL, NULL},
    {.prepare = DirInodeRecoveryAddDentryPrepare, .cleanup = DirInodeRecoveryAddDentryCleanup},
    {.prepare = DirInodeRecoveryRemoveDentryPrepare, .cleanup = DirInodeRecoveryRemoveDentryCleanup}};

void DirInodeRecovery(logic_addr_t inodeAddr, struct FsConstructor * ctor, struct CircularBuffer * cb,
                      struct NVMAccesser * acc)
{
    struct Log log;
    struct DirFileDentryRecoveryCache cache;

    LogRecoveryInit(&log, inodeAddr);
    DirFileDentryRecoveryCacheInit(&cache);
    LogRecovery(&log, ctor, DirInodeRecoveryCleanupOps, &cache, acc);
    DirFileDentryRecoveryCacheUninit(&cache, cb);
    LogRecoveryUninit(&log);
}

//--------------rebuild-----------------

struct DirInodeRebuildContex
{
    struct DirInodeLogAddDentryEntry addEntry;
    struct DirInodeLogRemoveDentryEntry removeEntry;
    struct DirInodeInfo * info;
};

static void * DirInodeRebuildAddDentryPrepare(UINT32 * size, void * data)
{
    struct DirInodeRebuildContex * context;

    context = data;
    *size = sizeof(struct DirInodeLogAddDentryEntry);
    return &context->addEntry;
}

static void DirInodeRebuildAddDentryCleanup(UINT8 type, UINT32 size, void * buffer, logic_addr_t entryReadStartAddr,
                                            logic_addr_t entryReadEndAddr, void * data)
{
    struct DirInodeLogAddDentryEntry * entry;
    struct DirInodeRebuildContex * context;

    entry = buffer;
    context = data;
    DirFileDentryCacheAppendDentry(&context->info->cache, entry->ino, entry->type, entryReadEndAddr, entry->nameLen);
}

static void * DirInodeRebuildRemoveDentryPrepare(UINT32 * size, void * data)
{
    struct DirInodeRebuildContex * context;

    context = data;
    *size = sizeof(struct DirInodeLogRemoveDentryEntry);
    return &context->removeEntry;
}

static void DirInodeRebuildRemoveDentryCleanup(UINT8 type, UINT32 size, void * buffer, logic_addr_t entryReadStartAddr,
                                               logic_addr_t entryReadEndAddr, void * data)
{
    struct DirInodeLogRemoveDentryEntry * entry;
    struct DirInodeRebuildContex * context;

    entry = buffer;
    context = data;
    DirFileDentryCacheRemoveDentry(&context->info->cache, entry->ino);
}

struct LogCleanupOps DirInodeRebuildCleanupOps[] = {
    {NULL, NULL},
    {DirInodeRebuildAddDentryPrepare, DirInodeRebuildAddDentryCleanup},
    {DirInodeRebuildRemoveDentryPrepare, DirInodeRebuildRemoveDentryCleanup}};

void DirInodeRebuild(struct DirInodeInfo * info, logic_addr_t addr, struct PagePool * pool, struct NVMAccesser * acc)
{
    struct DirInodeRebuildContex context = {.info = info};

    info->pool = pool;
    LogRebuildBegin(&info->log, addr, sizeof(struct NvmInode));
    DirFileDentryCacheInit(&info->cache);
    LogRebuild(&info->log, DirInodeRebuildCleanupOps, &context, acc);
    LogRebuildEnd(&info->log, acc);
}