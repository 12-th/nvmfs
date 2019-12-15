#include "Align.h"
#include "BlockPool.h"
#include "FileDataManager.h"
#include "FileInodeInfo.h"
#include "Log.h"
#include "common.h"
#include <linux/slab.h>

struct FileDataSpace
{
    union {
        struct ExtentContainer container;
        logic_addr_t addr;
    };
    UINT64 size;
};

void FileDataManagerInit(struct FileDataManager * manager, struct BlockPool * bpool, struct PagePool * ppool)
{
    manager->bpool = bpool;
    manager->ppool = ppool;
    FileExtentTreeInit(&manager->tree);
    manager->curArea = manager->curWriteStart = invalid_nvm_addr;
    manager->curSize = 0;
    manager->lastUsedPage = invalid_page;
    manager->fileMaxLen = 0;
}

void FileDataManagerUninit(struct FileDataManager * manager)
{
    FileExtentTreeUninit(&manager->tree);
}

void FileDataManagerDestroy(struct FileDataManager * manager)
{
    struct FileSpaceNode * space;
    struct rb_node * node;
    struct ExtentContainer container;

    ExtentContainerInit(&container, GFP_KERNEL);
    for_each_space_in_file_space_tree(space, &manager->tree, node)
    {
        if (space->size == SIZE_4K)
        {
            PagePoolFree(manager->ppool, space->start);
        }
        else
        {
            ExtentContainerAppend(&container, space->start, space->start + space->size, GFP_KERNEL);
        }
    }
    BlockPoolExtentPut(manager->bpool, &container);
    ExtentContainerUninit(&container);
    FileExtentTreeUninit(&manager->tree);
}

static inline UINT64 RestSizeOfCurrentArea(struct FileDataManager * manager)
{
    return manager->curSize - (manager->curWriteStart - manager->curArea);
}

static inline void UpdateFileMaxLen(struct FileDataManager * manager, UINT64 fileStart, UINT64 size)
{
    if (manager->fileMaxLen < fileStart + size)
        manager->fileMaxLen = fileStart + size;
}

static void WriteDataToArea(struct FileDataManager * manager, void * buffer, UINT64 size, UINT64 fileStart,
                            struct Log * log, struct NVMAccesser * acc)
{
    UINT64 restSize = RestSizeOfCurrentArea(manager);
    struct InodeLogWriteDataEntry entry = {.addr = manager->curWriteStart, .size = size, .fileStart = fileStart};
    NVMAccesserWrite(acc, manager->curWriteStart, size, buffer, size >= restSize);
    LogWrite(log, sizeof(entry), &entry, INODE_LOG_WRITE_DATA, NULL, manager->ppool, acc);
    FileExtentTreeAddExtent(&manager->tree, manager->curWriteStart, fileStart, size, GFP_KERNEL);
    manager->curWriteStart += size;
    UpdateFileMaxLen(manager, fileStart, size);
}

static void FileDataManagerWriteDataWithoutNewSpace(struct FileDataManager * manager, void * buffer, UINT64 size,
                                                    UINT64 fileStart, struct Log * log, struct NVMAccesser * acc)
{
    WriteDataToArea(manager, buffer, size, fileStart, log, acc);
}

static int AddNewSpace(struct FileDataManager * manager, logic_addr_t addr, UINT64 size, struct Log * log,
                       struct NVMAccesser * acc)
{
    struct InodeLogAddNewSpaceEntry entry = {.addr = addr, .size = size};
    int err;

    err = LogWrite(log, sizeof(entry), &entry, INODE_LOG_ADD_SPACE, NULL, manager->ppool, acc);
    if (err)
        return err;
    FileExtentTreeAddSpace(&manager->tree, addr, size, GFP_KERNEL);
    manager->curWriteStart = addr;
    manager->curArea = addr;
    manager->curSize = size;
    if (size == SIZE_4K)
    {
        manager->lastUsedPage = addr;
    }
    return 0;
}

static int FileDataManagerWriteDataSmall(struct FileDataManager * manager, void * buffer, UINT64 size, UINT64 fileStart,
                                         struct Log * log, struct NVMAccesser * acc)
{
    logic_addr_t page;
    UINT64 restSize;

    page = PagePoolAllocWithHint(manager->ppool, manager->lastUsedPage);
    if (page == invalid_nvm_addr)
        return -ENOSPC;

    restSize = RestSizeOfCurrentArea(manager);
    if (restSize)
    {
        WriteDataToArea(manager, buffer, restSize, fileStart, log, acc);
        size -= restSize;
        fileStart += restSize;
        buffer += restSize;
    }
    AddNewSpace(manager, page, SIZE_4K, log, acc);
    WriteDataToArea(manager, buffer, size, fileStart, log, acc);
    return 0;
}

static int PrepareExtentSpace(struct FileDataManager * manager, UINT64 size, struct ExtentContainer * container)
{
    UINT64 blockNum;
    blockNum = AlignUp(size, SIZE_2M) / SIZE_2M;
    BlockPoolExtentGet(manager->bpool, blockNum, container);
    if (container->size < blockNum)
    {
        BlockPoolExtentPut(manager->bpool, container);
        return -ENOSPC;
    }
    return 0;
}

static int FileDataManagerWriteDataLarge(struct FileDataManager * manager, void * buffer, UINT64 size, UINT64 fileStart,
                                         struct Log * log, struct NVMAccesser * acc)
{
    struct ExtentContainer container;
    UINT64 restSize;
    struct Extent * extent;

    restSize = RestSizeOfCurrentArea(manager);
    ExtentContainerInit(&container, GFP_KERNEL);
    if (PrepareExtentSpace(manager, size - restSize, &container) == -ENOSPC)
    {
        ExtentContainerUninit(&container);
        return -ENOSPC;
    }

    if (restSize)
    {
        WriteDataToArea(manager, buffer, restSize, fileStart, log, acc);
        size -= restSize;
        fileStart += restSize;
        buffer += restSize;
    }

    for_each_extent_in_container(extent, &container)
    {
        logic_addr_t addr = logical_block_to_addr(extent->start);
        UINT64 extentSize = logical_block_to_addr(extent->end) - addr;
        UINT64 thisTimeWriteSize;

        thisTimeWriteSize = extentSize >= size ? size : extentSize;
        AddNewSpace(manager, addr, extentSize, log, acc);
        WriteDataToArea(manager, buffer, thisTimeWriteSize, fileStart, log, acc);
        size -= thisTimeWriteSize;
        fileStart += thisTimeWriteSize;
        buffer += thisTimeWriteSize;
    }

    ExtentContainerUninit(&container);
    return 0;
}

int FileDataManagerWriteData(struct FileDataManager * manager, void * buffer, UINT64 size, UINT64 fileStart,
                             struct Log * log, struct NVMAccesser * acc)
{
    UINT64 restSize;

    restSize = RestSizeOfCurrentArea(manager);
    if (size <= restSize)
    {
        FileDataManagerWriteDataWithoutNewSpace(manager, buffer, size, fileStart, log, acc);
        return 0;
    }

    if (size - restSize <= SIZE_4K)
    {
        return FileDataManagerWriteDataSmall(manager, buffer, size, fileStart, log, acc);
    }

    return FileDataManagerWriteDataLarge(manager, buffer, size, fileStart, log, acc);
}

static void CopyDataToArea(struct FileDataManager * manager, logic_addr_t srcAddr, UINT64 size, UINT64 fileStart,
                           struct Log * log, struct NVMAccesser * acc)
{
    struct InodeLogWriteDataEntry entry = {.addr = manager->curWriteStart, .size = size, .fileStart = fileStart};
    NVMAccesserMemcpy(acc, srcAddr, manager->curWriteStart, size, 1);
    LogWrite(log, sizeof(entry), &entry, INODE_LOG_WRITE_DATA, NULL, manager->ppool, acc);
    FileExtentTreeAddExtent(&manager->tree, manager->curWriteStart, fileStart, size, GFP_KERNEL);
    manager->curWriteStart += size;
}

static int MergeDataSmall(struct FileDataManager * manager, struct Log * log, struct NVMAccesser * acc,
                          struct FileDataManager * newManager)
{
    logic_addr_t addr;
    struct FileExtentNode * fileExtent;
    struct rb_node * node;

    addr = PagePoolAllocWithHint(manager->ppool, manager->lastUsedPage);
    if (addr == invalid_nvm_addr)
        return -ENOSPC;
    AddNewSpace(newManager, addr, SIZE_4K, log, acc);

    for_each_extent_in_file_extent_tree(fileExtent, &manager->tree, node)
    {
        CopyDataToArea(newManager, fileExtent->start, fileExtent->size, fileExtent->fileStart, log, acc);
    }

    return 0;
}

static int MergeDataLarge(struct FileDataManager * manager, struct Log * log, struct NVMAccesser * acc,
                          struct FileDataManager * newManager, UINT64 effectiveSize)
{
    struct ExtentContainer container;
    int index = 0;
    struct rb_node * node;
    struct FileExtentNode * fileExtent;

    ExtentContainerInit(&container, GFP_KERNEL);
    if (PrepareExtentSpace(manager, effectiveSize, &container) == ENOSPC)
    {
        ExtentContainerUninit(&container);
        return -ENOSPC;
    }

    for_each_extent_in_file_extent_tree(fileExtent, &manager->tree, node)
    {
        UINT64 extentSize = fileExtent->size;
        logic_addr_t readStart = fileExtent->start;
        UINT64 fileStart = fileExtent->fileStart;

        while (extentSize)
        {
            UINT64 thisTimeCopySize;
            UINT64 restSize = RestSizeOfCurrentArea(newManager);
            if (restSize == 0)
            {
                struct Extent * extent;
                extent = ExtentContainerGetByIndex(&container, index);
                index++;
                AddNewSpace(newManager, logical_block_to_addr(extent->start),
                            logical_block_to_addr(extent->end - extent->start), log, acc);
                restSize = RestSizeOfCurrentArea(newManager);
            }
            thisTimeCopySize = extentSize > restSize ? restSize : extentSize;
            CopyDataToArea(newManager, readStart, thisTimeCopySize, fileStart, log, acc);
            readStart += thisTimeCopySize;
            fileStart += thisTimeCopySize;
            extentSize -= thisTimeCopySize;
        }
    }
    ExtentContainerUninit(&container);
    return 0;
}

int FileDataManagerMergeData(struct FileDataManager * manager, struct Log * log, struct NVMAccesser * acc)
{
    struct FileDataManager newManager;
    UINT64 effectiveSize;
    int err;

    FileDataManagerInit(&newManager, manager->bpool, manager->ppool);
    effectiveSize = FileExtentTreeGetEffectiveSize(&manager->tree);
    if (effectiveSize <= SIZE_4K)
    {
        err = MergeDataSmall(manager, log, acc, &newManager);
    }
    else
    {
        err = MergeDataLarge(manager, log, acc, &newManager, effectiveSize);
    }

    if (!err)
    {
        FileDataManagerUninit(manager);
        *manager = newManager;
    }
    else
    {
        FileDataManagerDestroy(&newManager);
    }

    return err;
}

INT64 FileDataManagerReadData(struct FileDataManager * manager, void * buffer, UINT64 size, UINT64 fileStart,
                              struct NVMAccesser * acc)
{
    UINT64 end;
    if (fileStart > manager->fileMaxLen)
        return -1;
    end = fileStart + size;
    if (end > manager->fileMaxLen)
        end = manager->fileMaxLen;
    FileExtentTreeRead(&manager->tree, fileStart, end, buffer, acc);
    return end - fileStart;
}

void FileDataManagerRebuildBegin(struct FileDataManager * manager, struct PagePool * ppool, struct BlockPool * bpool)
{
    manager->ppool = ppool;
    manager->bpool = bpool;
    FileExtentTreeInit(&manager->tree);
    manager->lastUsedPage = invalid_nvm_addr;
    manager->fileMaxLen = 0;
    manager->curArea = invalid_nvm_addr;
    manager->curWriteStart = invalid_nvm_addr;
    manager->curSize = 0;
}

void FileDataManagerRebuildAddSpace(struct FileDataManager * manager, logic_addr_t addr, UINT64 size)
{
    FileExtentTreeAddSpace(&manager->tree, addr, size, GFP_KERNEL);
    manager->curArea = addr;
    manager->curSize = size;
    if (size == SIZE_4K)
    {
        manager->lastUsedPage = addr;
    }
}

void FileDataManagerRebuildAddExtent(struct FileDataManager * manager, logic_addr_t addr, UINT64 size, UINT64 fileStart)
{
    FileExtentTreeAddExtent(&manager->tree, addr, fileStart, size, GFP_KERNEL);
    manager->curWriteStart = addr + size;
    UpdateFileMaxLen(manager, fileStart, size);
}

void FileDataManagerRebuildEnd(struct FileDataManager * manager)
{
    if (!(manager->curWriteStart >= manager->curArea &&
          manager->curWriteStart <= (manager->curArea + manager->curSize)))
    {
        // add new space and crashed
        manager->curWriteStart = manager->curArea;
    }
}

int FileDataManagerIsSame(struct FileDataManager * manager1, struct FileDataManager * manager2)
{
    if (manager1->ppool != manager2->ppool)
        return 0;
    if (manager1->bpool != manager2->bpool)
        return 0;
    if (manager1->curArea != manager2->curArea)
        return 0;
    if (manager1->curWriteStart != manager2->curWriteStart)
        return 0;
    if (manager1->curSize != manager2->curSize)
        return 0;
    if (manager1->lastUsedPage != manager2->lastUsedPage)
        return 0;
    return FileExtentTreeIsSame(&manager1->tree, &manager2->tree);
}

void FileDataManagerPrintInfo(struct FileDataManager * manager)
{
    DEBUG_PRINT("file data manager curArea is 0x%lx, curWriteStart is 0x%lx, curSize is 0x%lx, last used page is 0x%lx",
                manager->curArea, manager->curWriteStart, manager->curSize, manager->lastUsedPage);
    FileExtentTreePrintInfo(&manager->tree);
}