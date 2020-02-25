#include "BlockColdnessManager.h"
#include "BlockManager.h"
#include "BlockSwapLog.h"
#include "BlockUnmapTable.h"
#include "BlockWearTable.h"
#include "Layouter.h"
#include "MapTable.h"
#include "NVMOperations.h"
#include "WearLeveler.h"
#include <linux/sort.h>

struct BlockSwapLockHandle
{
    logic_addr_t addr[3];
};

void BlockManagerInit(struct BlockManager * manager, struct WearLeveler * wl)
{
    struct Layouter * l = &wl->layouter;
    manager->blockNum = BlockNumQuery(l);
    manager->dataStartOffset = l->dataStart;
    manager->coldness = wl->blockColdnessManager;
    manager->unmapTable = &wl->blockUnmapTable;
    manager->pageUnmapTable = &wl->pageUnmapTable;
    manager->wearTable = &wl->blockWearTable;
    manager->pageWearTable = &wl->pageWearTable;
    manager->log = &wl->blockSwapLog;
    manager->mapTable = &wl->mapTable;
    manager->threshold = STEP_WEAR_COUNT;
    manager->thresholdAddr = l->metaDataOfThreshold;
}

void BlockManagerRead(struct BlockManager * manager, logic_addr_t addr, UINT64 size, void * buffer)
{
    struct MapTableLockHandle handle;
    do
    {
        nvm_addr_t physAddr;
        handle = MapTableLockBlockForRead(manager->mapTable, addr);
        physAddr = MapTableQuery(manager->mapTable, addr);
        NVMRead(physAddr, size, buffer);
    } while (MapTableUnlockBlockForRead(manager->mapTable, addr, handle));
}

static logic_addr_t LockOneBlock(struct BlockManager * manager, physical_block_t block)
{
    nvm_addr_t addr = block_to_nvm_addr(block, manager->dataStartOffset);
    struct BlockInfo info;
    logic_addr_t logicAddr;

    do
    {
        BlockUnmapTableGet(manager->unmapTable, block, &info);
        logicAddr = logical_block_to_addr(info.unmapBlock);
    } while (MapTableLockBlockForSwap(manager->mapTable, logicAddr, addr));

    return logicAddr;
}

static struct BlockSwapLockHandle BlockSwapFastLock(struct BlockManager * manager, physical_block_t blockA,
                                                    physical_block_t blockB)
{
    struct BlockSwapLockHandle handle;
    if (blockA < blockB)
    {
        handle.addr[0] = LockOneBlock(manager, blockA);
        handle.addr[1] = LockOneBlock(manager, blockB);
    }
    else
    {
        handle.addr[0] = LockOneBlock(manager, blockB);
        handle.addr[1] = LockOneBlock(manager, blockA);
    }

    return handle;
}

static void BlockSwapFastUnlock(struct BlockManager * manager, struct BlockSwapLockHandle handle)
{
    MapTableUnlockBlockForSwap(manager->mapTable, handle.addr[0]);
    MapTableUnlockBlockForSwap(manager->mapTable, handle.addr[1]);
}

static void SortBlocks(physical_block_t * blocks)
{
    int i, j;
    for (i = 0; i < 2; i++)
        for (j = 0; j < 2 - i; j++)
            if (blocks[j] > blocks[j + 1])
            {
                physical_block_t tmp;
                tmp = blocks[j];
                blocks[j] = blocks[j + 1];
                blocks[j + 1] = tmp;
            }
}

static struct BlockSwapLockHandle BlockSwapSlowLock(struct BlockManager * manager, physical_block_t blockA,
                                                    physical_block_t blockB, physical_block_t blockC)
{
    physical_block_t blocks[3] = {blockA, blockB, blockC};
    int i;
    struct BlockSwapLockHandle handle;

    SortBlocks(blocks);
    for (i = 0; i < 3; ++i)
    {
        handle.addr[i] = LockOneBlock(manager, blocks[i]);
    }
    return handle;
}

static void BlockSwapSlowUnlock(struct BlockManager * manager, struct BlockSwapLockHandle handle)
{
    MapTableUnlockBlockForSwap(manager->mapTable, handle.addr[0]);
    MapTableUnlockBlockForSwap(manager->mapTable, handle.addr[1]);
    MapTableUnlockBlockForSwap(manager->mapTable, handle.addr[2]);
}

static void BlockSwapFast(struct BlockManager * manager, physical_block_t blockA, physical_block_t blockB, UINT32 wcA,
                          UINT64 dataStartOffset)
{
    UINT32 wcB;
    struct BlockInfo infoA, infoB, infoC;
    nvm_addr_t addr;
    struct BlockSwapLockHandle handle;

    handle = BlockSwapFastLock(manager, blockA, blockB);

    infoC.unmapBlock = invalid_block;
    BlockUnmapTableGet(manager->unmapTable, blockA, &infoA);
    BlockUnmapTableGet(manager->unmapTable, blockB, &infoB);
    addr = LogBlockSwapBegin(manager->log, blockA, blockB, invalid_block, infoA, infoB, infoC, 1);
    NVMemcpy(block_to_nvm_addr(blockB, dataStartOffset), block_to_nvm_addr(blockA, dataStartOffset), SIZE_2M);
    LogBlockSwapCommitStepx(addr, 2);
    BlockUnmapTableSet(manager->unmapTable, blockA, &infoB);
    BlockUnmapTableSet(manager->unmapTable, blockB, &infoA);
    MapTableBlockSwapFast(manager->mapTable, infoA.unmapBlock, infoB.unmapBlock);
    LogBlockSwapEnd(addr);

    wcB = BlockWearTableGet(manager->wearTable, blockB);
    BlockWearTableSet(manager->wearTable, blockA, wcA);
    BlockWearTableSet(manager->wearTable, blockB, wcB + 1);
    BlockColdnessManagerPut(manager->coldness, blockA, wcA / STEP_WEAR_COUNT, 0);
    BlockColdnessManagerPut(manager->coldness, blockB, (wcB + 1) / STEP_WEAR_COUNT, 1);

    BlockSwapFastUnlock(manager, handle);
}

static void BlockSwapSlow(struct BlockManager * manager, physical_block_t blockA, physical_block_t blockB,
                          physical_block_t blockC, UINT32 wcA, UINT64 dataStartOffset)
{
    struct BlockInfo infoA, infoB, infoC;
    UINT32 wcB, wcC;
    nvm_addr_t addr;
    struct BlockSwapLockHandle handle;

    handle = BlockSwapSlowLock(manager, blockA, blockB, blockC);

    BlockUnmapTableGet(manager->unmapTable, blockA, &infoA);
    BlockUnmapTableGet(manager->unmapTable, blockB, &infoB);
    BlockUnmapTableGet(manager->unmapTable, blockC, &infoC);
    addr = LogBlockSwapBegin(manager->log, blockA, blockB, blockC, infoA, infoB, infoC, 2);
    NVMemcpy(block_to_nvm_addr(blockC, dataStartOffset), block_to_nvm_addr(blockB, dataStartOffset), SIZE_2M);
    LogBlockSwapCommitStepx(addr, 2);
    NVMemcpy(block_to_nvm_addr(blockB, dataStartOffset), block_to_nvm_addr(blockA, dataStartOffset), SIZE_2M);
    LogBlockSwapCommitStepx(addr, 3);
    BlockUnmapTableSet(manager->unmapTable, blockA, &infoC);
    BlockUnmapTableSet(manager->unmapTable, blockB, &infoA);
    BlockUnmapTableSet(manager->unmapTable, blockC, &infoB);
    MapTableBlockSwapSlow(manager->mapTable, infoA.unmapBlock, infoB.unmapBlock, infoC.unmapBlock);
    LogBlockSwapEnd(addr);

    wcB = BlockWearTableGet(manager->wearTable, blockB);
    wcC = BlockWearTableGet(manager->wearTable, blockC);
    BlockWearTableSet(manager->wearTable, blockA, wcA);
    BlockWearTableSet(manager->wearTable, blockB, wcB + 1);
    BlockWearTableSet(manager->wearTable, blockC, wcC + 1);
    BlockColdnessManagerPut(manager->coldness, blockA, wcA / STEP_WEAR_COUNT, 0);
    BlockColdnessManagerPut(manager->coldness, blockB, (wcB + 1) / STEP_WEAR_COUNT, 1);
    BlockColdnessManagerPut(manager->coldness, blockC, (wcC + 1) / STEP_WEAR_COUNT, 1);

    BlockSwapSlowUnlock(manager, handle);
}

static int IncreaseBlockWearCount(struct BlockManager * manager, nvm_addr_t physAddr, int increaseWearCount,
                                  UINT32 threshold)
{
    physical_block_t blockA, blockB, blockC;
    UINT32 wcA;

    blockA = nvm_addr_to_block(physAddr, manager->dataStartOffset);
    wcA = BlockWearTableGet(manager->wearTable, blockA);
    if (wcA + increaseWearCount < threshold)
    {
        BlockWearTableSet(manager->wearTable, blockA, wcA + increaseWearCount);
        return 0;
    }

    BlockColdnessManagerRemove(manager->coldness, blockA);
    blockB = BlockColdnessManagerGetFreeBlock(manager->coldness, threshold / STEP_WEAR_COUNT - 1);
    if (blockB != invalid_block)
    {
        BlockSwapFast(manager, blockA, blockB, wcA + increaseWearCount, manager->dataStartOffset);
        return 0;
    }
    blockB = BlockColdnessManagerGetBusyBlock(manager->coldness, threshold / STEP_WEAR_COUNT - 1);
    if (blockB != invalid_block)
    {
        blockC = BlockColdnessManagerGetFreeBlock(manager->coldness, threshold / STEP_WEAR_COUNT);
        ASSERT(blockC != invalid_block);
        BlockSwapSlow(manager, blockA, blockB, blockC, wcA + increaseWearCount, manager->dataStartOffset);
        return 0;
    }
    BlockColdnessManagerPut(manager->coldness, blockA, wcA / STEP_WEAR_COUNT, 1);
    return 1;
}

static void SpecialBlockSwapType1(struct BlockManager * manager, physical_block_t blockA, physical_block_t blockB,
                                  UINT32 wcA, UINT64 dataStartOffset)
{
    UINT32 wcB;
    struct BlockInfo infoA, infoB, infoC;
    nvm_addr_t addr;
    struct BlockSwapLockHandle handle;

    handle = BlockSwapFastLock(manager, blockA, blockB);

    infoC.unmapBlock = invalid_block;
    BlockUnmapTableGet(manager->unmapTable, blockA, &infoA);
    BlockUnmapTableGet(manager->unmapTable, blockB, &infoB);
    addr = LogBlockSwapBegin(manager->log, blockA, blockB, invalid_block, infoA, infoB, infoC, 3);
    BlockUnmapTableSet(manager->unmapTable, blockB, &infoA);
    BlockUnmapTableSet(manager->unmapTable, blockA, &infoB);
    LogBlockSwapEnd(addr);

    wcB = BlockWearTableGet(manager->wearTable, blockB);
    BlockWearTableSet(manager->wearTable, blockA, wcA);
    BlockWearTableSet(manager->wearTable, blockB, wcB + 1);
    BlockColdnessManagerPut(manager->coldness, blockA, wcA / STEP_WEAR_COUNT, 0);

    BlockSwapFastUnlock(manager, handle);
}

static void SpecialBlockSwapType2(struct BlockManager * manager, physical_block_t blockA, physical_block_t blockB,
                                  UINT32 wcA, UINT64 dataStartOffset)
{
    UINT32 wcB;
    struct BlockInfo infoA, infoB, infoC;
    nvm_addr_t addr;
    struct BlockSwapLockHandle handle;

    handle = BlockSwapFastLock(manager, blockA, blockB);

    infoC.unmapBlock = invalid_block;
    BlockUnmapTableGet(manager->unmapTable, blockA, &infoA);
    BlockUnmapTableGet(manager->unmapTable, blockB, &infoB);
    addr = LogBlockSwapBegin(manager->log, blockA, blockB, invalid_block, infoA, infoB, infoC, 4);
    NVMemcpy(block_to_nvm_addr(blockB, manager->dataStartOffset), block_to_nvm_addr(blockA, dataStartOffset), SIZE_2M);
    LogBlockSwapCommitStepx(addr, 2);
    BlockUnmapTableSet(manager->unmapTable, blockB, &infoA);
    BlockUnmapTableSet(manager->unmapTable, blockA, &infoB);
    LogBlockSwapEnd(addr);

    wcB = BlockWearTableGet(manager->wearTable, blockB);
    BlockWearTableSet(manager->wearTable, blockA, wcA);
    BlockWearTableSet(manager->wearTable, blockB, wcB + 1);
    BlockColdnessManagerPut(manager->coldness, blockA, wcA / STEP_WEAR_COUNT, 1);

    BlockSwapFastUnlock(manager, handle);
}

static int SwapSpeicalBlock(struct BlockManager * manager, UINT32 threshold, nvm_addr_t addr, nvm_addr_t * newAddr)
{
    physical_block_t blockA, blockB;
    UINT32 wcA;

    *newAddr = invalid_nvm_addr;
    blockA = nvm_addr_to_block(addr, manager->dataStartOffset);
    wcA = BlockWearTableGet(manager->wearTable, blockA);
    if (wcA + 1 < threshold)
    {
        BlockWearTableSet(manager->wearTable, blockA, wcA + 1);
        return 0;
    }
    blockB = BlockColdnessManagerGetFreeBlock(manager->coldness, threshold / STEP_WEAR_COUNT - 1);
    if (blockB != invalid_block)
    {
        SpecialBlockSwapType1(manager, blockA, blockB, wcA + 1, manager->dataStartOffset);
        *newAddr = block_to_nvm_addr(blockB, manager->dataStartOffset);
        return 0;
    }
    blockB = BlockColdnessManagerGetBusyBlock(manager->coldness, threshold / STEP_WEAR_COUNT - 1);
    if (blockB != invalid_block)
    {
        SpecialBlockSwapType2(manager, blockA, blockB, wcA + 1, manager->dataStartOffset);
        *newAddr = block_to_nvm_addr(blockB, manager->dataStartOffset);
        return 0;
    }
    return 1;
}

static inline void IncreaseThreshold(struct BlockManager * manager, UINT32 threshold)
{
    UINT32 newThreshold = threshold + STEP_WEAR_COUNT;
    if (BOOL_COMPARE_AND_SWAP(&manager->threshold, threshold, newThreshold))
    {
        NVMWrite(manager->thresholdAddr, sizeof(UINT32), &newThreshold);
    }
}

void BlockManagerIncreaseSpecialBlockWearCount(struct BlockManager * manager, UINT32 threshold, nvm_addr_t physAddr,
                                               nvm_addr_t * newPhysAddr)
{
    UINT32 wc;
    physical_block_t block;

    *newPhysAddr = invalid_nvm_addr;
    block = nvm_addr_to_block(physAddr, manager->dataStartOffset);
    wc = BlockWearTableGet(manager->wearTable, block);
    if (wc + 1 >= manager->threshold)
    {
        if (SwapSpeicalBlock(manager, threshold, physAddr, newPhysAddr))
            IncreaseThreshold(manager, threshold);
    }
}

void BlockManagerIncreaseWearCount(struct BlockManager * manager, nvm_addr_t physAddr, UINT32 increaseWearCount)
{
    nvm_addr_t newPhysAddr;
    UINT32 threshold;
    int ret;

    if (!increaseWearCount)
        return;

    threshold = manager->threshold;
    ret = IncreaseBlockWearCount(manager, physAddr, increaseWearCount, threshold);
    if (ret)
        IncreaseThreshold(manager, threshold);

    if (!BlockSwapLogIsFull(manager->log))
        return;
    BlockManagerIncreaseSpecialBlockWearCount(manager, threshold, BlockSwapLogGetAreaAddr(manager->log), &newPhysAddr);
    if (newPhysAddr != invalid_nvm_addr)
    {
        BlockSwapLogSetAreaAddr(manager->log, newPhysAddr);
    }
}

void BlockManagerWrite(struct BlockManager * manager, logic_addr_t addr, UINT64 size, void * buffer,
                       int increaseWearCount)
{
    nvm_addr_t physAddr;

    MapTableLockBlockForWrite(manager->mapTable, addr);
    physAddr = MapTableQuery(manager->mapTable, addr);
    MapTableBlockBusy(manager->mapTable, addr);
    NVMWrite(physAddr, size, buffer);
    MapTableUnlockBlockForWrite(manager->mapTable, addr);

    BlockManagerIncreaseWearCount(manager, physAddr, increaseWearCount);
}

void BlockManagerMemset(struct BlockManager * manager, logic_addr_t addr, UINT64 size, int value, int increaseWearCount)
{
    nvm_addr_t physAddr;

    MapTableLockBlockForWrite(manager->mapTable, addr);
    physAddr = MapTableQuery(manager->mapTable, addr);
    MapTableBlockBusy(manager->mapTable, addr);
    NVMemset(addr, value, size);
    MapTableUnlockBlockForWrite(manager->mapTable, addr);

    BlockManagerIncreaseWearCount(manager, physAddr, increaseWearCount);
}

void BlockManagerTrim(struct BlockManager * manager, logic_addr_t addr)
{
    nvm_addr_t physAddr;
    physical_block_t block;
    UINT32 wc;

    MapTableLockBlockForWrite(manager->mapTable, addr);
    MapTableBlockTrim(manager->mapTable, addr);
    physAddr = MapTableQuery(manager->mapTable, addr);
    block = nvm_addr_to_block(physAddr, manager->dataStartOffset);
    wc = BlockWearTableGet(manager->wearTable, block);
    BlockColdnessManagerRemove(manager->coldness, block);
    BlockColdnessManagerPut(manager->coldness, block, wc / STEP_WEAR_COUNT, 0);
    MapTableUnlockBlockForWrite(manager->mapTable, addr);
}

void BlockManagerSplit(struct BlockManager * manager, logic_addr_t addr)
{
    struct BlockInfo info;
    nvm_addr_t physAddr;
    physical_block_t block;

    MapTableLockBlockForWrite(manager->mapTable, addr);
    MapTableSplit(manager->mapTable, addr);
    physAddr = MapTableQuery(manager->mapTable, addr);
    block = nvm_addr_to_block(physAddr, manager->dataStartOffset);
    BlockUnmapTableGet(manager->unmapTable, block, &info);
    info.fineGrain = 1;
    PageUnmapTableBatchFormat(manager->pageUnmapTable, block);
    PageWearTableBatchFormat(manager->pageWearTable, block_to_page(block));
    BlockUnmapTableSet(manager->unmapTable, block, &info);
    MapTableUnlockBlockForWrite(manager->mapTable, addr);
}

void BlockManagerMerge(struct BlockManager * manager, logic_addr_t addr)
{
    struct BlockInfo info;
    nvm_addr_t physAddr;
    physical_block_t block;
    UINT32 sum, wc;

    MapTableLockBlockForWrite(manager->mapTable, addr);
    MapTableMerge(manager->mapTable, addr);
    physAddr = MapTableQuery(manager->mapTable, addr);
    block = nvm_addr_to_block(physAddr, manager->dataStartOffset);
    BlockUnmapTableGet(manager->unmapTable, block, &info);
    info.fineGrain = 0;
    BlockUnmapTableSet(manager->unmapTable, block, &info);
    sum = PageWearTableSumGet(manager->pageWearTable, block_to_page(block));
    wc = BlockWearTableGet(manager->wearTable, block);
    BlockWearTableSet(manager->wearTable, block, wc + sum / 512);
    MapTableUnlockBlockForWrite(manager->mapTable, addr);
}