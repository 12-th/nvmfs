#ifndef FILE_DATA_MANAGER_H
#define FILE_DATA_MANAGER_H

#include "FileExtentTree.h"

struct PagePool;
struct BlockPool;
struct Log;
struct NVMAccesser;

struct FileDataManager
{
    struct FileExtentTree tree;
    struct PagePool * ppool;
    struct BlockPool * bpool;

    logic_addr_t curArea;
    logic_addr_t curWriteStart;
    UINT64 curSize;
    logical_page_t lastUsedPage;

    UINT64 fileMaxLen;
};

void FileDataManagerInit(struct FileDataManager * manager, struct BlockPool * bpool, struct PagePool * ppool);
void FileDataManagerUninit(struct FileDataManager * manager);
void FileDataManagerDestroy(struct FileDataManager * manager);
int FileDataManagerWriteData(struct FileDataManager * manager, void * buffer, UINT64 size, UINT64 fileStart,
                             struct Log * log, struct NVMAccesser * acc);
int FileDataManagerReadData(struct FileDataManager * manager, void * buffer, UINT64 size, UINT64 fileStart,
                            struct NVMAccesser * acc);
int FileDataManagerTruncate(struct FileDataManager * manager, struct Log * log, struct NVMAccesser * acc);
int FileDataManagerMergeData(struct FileDataManager * manager, struct Log * log, struct NVMAccesser * acc);

void FileDataManagerRebuildBegin(struct FileDataManager * manager, struct PagePool * ppool, struct BlockPool * bpool);
void FileDataManagerRebuildAddSpace(struct FileDataManager * manager, logic_addr_t addr, UINT64 size);
void FileDataManagerRebuildAddExtent(struct FileDataManager * manager, logic_addr_t addr, UINT64 size,
                                     UINT64 fileStart);
void FileDataMangerRebuildTruncate(struct FileDataManager * manager);
void FileDataManagerRebuildEnd(struct FileDataManager * manager);

static inline UINT64 FileDataManagerGetSpaceSize(struct FileDataManager * manager)
{
    return FileExtentTreeGetSpaceSize(&manager->tree);
}

int FileDataManagerIsSame(struct FileDataManager * manager1, struct FileDataManager * manager2);
void FileDataManagerPrintInfo(struct FileDataManager * manager);

#endif
