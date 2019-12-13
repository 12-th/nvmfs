#include "DirInodeInfo.h"
#include "Inode.h"

static unsigned int DJBHash(char * str, int len)
{
    unsigned int hash = 5381;
    unsigned int i = 0;

    while (i < len)
    {
        hash += (hash << 5) + str[i];
        ++i;
    }

    return (hash & 0x7FFFFFFF);
}

int DirInodeInfoFormat(struct DirInodeInfo * info, struct PagePool * ppool, logic_addr_t * firstArea,
                       struct NVMAccesser * acc)
{
    int err;
    info->pool = ppool;
    DirFileDentryCacheInit(&info->cache);
    err = LogFormat(&info->log, ppool, sizeof(struct BaseInodeInfo), acc);
    if (err)
    {
        DirFileDentryCacheUninit(&info->cache);
        return err;
    }
    LogWriteReserveData(&info->log, sizeof(struct BaseInodeInfo), 0, &info->baseInfo, acc);
    *firstArea = LogFirstArea(&info->log);
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

int DirInodeInfoLookupDentryByIno(struct DirInodeInfo * info, nvmfs_ino_t ino)
{
    struct DentryNode * node;
    node = DirFileDentryCacheLookupByIno(&info->cache, ino);
    if (!node)
        return -EINVAL;
    return 0;
}

int DirInodeInfoLookupAndGetDentryName(struct DirInodeInfo * info, nvmfs_ino_t ino, char * buffer, UINT64 bufferSize,
                                       struct NVMAccesser * acc)
{
    struct DentryNode * node;

    node = DirFileDentryCacheLookupByIno(&info->cache, ino);
    if (!node)
        return -ENOENT;
    if (bufferSize < node->nameLen)
    {
        return -EINVAL;
    }
    LogRead(&info->log, node->nameAddr, node->nameLen, buffer, acc);
    return 0;
}

int DirInodeInfoLookupDentryByName(struct DirInodeInfo * info, nvmfs_ino_t * retIno, char * name, UINT64 nameLen,
                                   struct NVMAccesser * acc)
{
    UINT32 nameHash;
    struct DentryNode * node;

    if (strcmp(name, ".") == 0)
    {
        *retIno = info->thisIno;
        return 0;
    }
    if (strcmp(name, "..") == 0)
    {
        *retIno = info->parentIno;
        return 0;
    }
    nameHash = DJBHash(name, nameLen);
    node = DirFileDentryCacheLookupByName(&info->cache, name, nameHash, nameLen, &info->log, acc);
    if (node == NULL)
        return -ENOENT;
    *retIno = node->ino;
    return 0;
}

int DirInodeInfoAddDentry(struct DirInodeInfo * info, nvmfs_ino_t ino, char * name, UINT64 len, UINT8 type,
                          struct NVMAccesser * acc)
{
    struct DirInodeLogAddDentryEntry entry = {.ino = ino, .type = type, .nameLen = len};
    struct LogStepWriteHandle handle;
    logic_addr_t nameAddr;
    int err;
    UINT32 nameHash;

    nameHash = DJBHash(name, len);
    err = DirFileDentryCacheAppendDentryCheck(&info->cache, ino, name, nameHash, len, &info->log, acc);
    if (err)
        return err;
    err = LogStepWriteBegin(&info->log, &handle, INODE_LOG_ADD_DENTRY, sizeof(entry) + len, info->pool, acc);
    if (err)
    {
        DirFileDentryCacheRemoveDentry(&info->cache, ino);
        return err;
    }
    LogStepWriteContinue(&handle, &entry, sizeof(entry), NULL, acc);
    LogStepWriteContinue(&handle, name, len, &nameAddr, acc);
    LogStepWriteEnd(&handle, acc);
    DirFileDentryCacheJustAppendDentry(&info->cache, ino, type, nameAddr, nameHash, len);
    return err;
}

int DirInodeInfoRemoveDentry(struct DirInodeInfo * info, nvmfs_ino_t ino, struct NVMAccesser * acc)
{
    struct DirInodeLogRemoveDentryEntry entry = {.ino = ino};
    int err;

    if (DirFileDentryCacheLookupByIno(&info->cache, ino) == NULL)
        return -EINVAL;

    err = LogWrite(&info->log, sizeof(entry), &entry, INODE_LOG_REMOVE_DENTRY, NULL, info->pool, acc);
    if (err)
        return err;
    DirFileDentryCacheRemoveDentry(&info->cache, ino);
    return 0;
}

void DirInodeInfoIterateDentry(struct DirInodeInfo * info, UINT64 index,
                               int (*func)(void * data, UINT8 type, const char * name, UINT32 len, nvmfs_ino_t ino),
                               void * data, struct NVMAccesser * acc)
{
    switch (index)
    {
    case 0:
        if (func(data, info->baseInfo.type, ".", 2, info->baseInfo.thisIno))
            return;
        index++;
    case 1:
        if (func(data, INODE_TYPE_DIR_FILE, "..", 3, info->baseInfo.parentIno))
            return;
        index++;
    }

    DirFileDentryCacheIterate(&info->cache, index - 2, func, data, &info->log, acc);
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

void DirInodeInfoRecovery(logic_addr_t inodeAddr, struct FsConstructor * ctor, struct CircularBuffer * cb,
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
    DirFileDentryCacheJustAppendDentry(&context->info->cache, entry->ino, entry->type, entryReadEndAddr,
                                       entry->nameHash, entry->nameLen);
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

void DirInodeInfoRebuild(struct DirInodeInfo * info, logic_addr_t addr, struct PagePool * pool,
                         struct NVMAccesser * acc)
{
    struct DirInodeRebuildContex context = {.info = info};

    info->pool = pool;
    LogRebuildReadReserveData(addr, &info->baseInfo, sizeof(struct BaseInodeInfo), 0, acc);
    LogRebuildBegin(&info->log, addr, sizeof(struct BaseInodeInfo));
    DirFileDentryCacheInit(&info->cache);
    LogRebuild(&info->log, DirInodeRebuildCleanupOps, &context, acc);
    LogRebuildEnd(&info->log, acc);
}