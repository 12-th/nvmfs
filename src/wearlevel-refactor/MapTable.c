#include "AtomicFunctions.h"
#include "Layouter.h"
#include "MapTable.h"
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/string.h>

void MapTableFormat(struct MapTable * pTable, UINT64 blockNum, UINT64 dataStartOffset)
{
    int i;

    pTable->blockNum = blockNum;
    pTable->dataStartOffset = dataStartOffset;
    pTable->entries = kvmalloc(sizeof(struct BlockEntry) * blockNum, GFP_KERNEL);
    memset(pTable->entries, 0, sizeof(struct BlockEntry) * blockNum);
    for (i = 0; i < blockNum; ++i)
    {
        pTable->entries[i].physBlockIndex = i;
    }
}

void MapTableUninit(struct MapTable * pTable)
{
    kvfree(pTable->entries);
}

static inline UINT64 IndexOfBlock(logic_addr_t addr)
{
    return addr >> BITS_2M;
}

static inline UINT64 RelativeIndexOfPage(logic_addr_t addr)
{
    return (addr >> BITS_4K) & ((1UL << 9) - 1);
}

static inline UINT64 PageOffsetOfAddr(logic_addr_t addr)
{
    return addr & ((1UL << BITS_4K) - 1);
}

static inline UINT64 BlockOffsetOfAddr(logic_addr_t addr)
{
    return addr & ((1UL << BITS_2M) - 1);
}

static physical_block_t MapBlockQuery(struct MapTable * pTable, logic_addr_t addr)
{
    return pTable->entries[IndexOfBlock(addr)].physBlockIndex;
}

nvm_addr_t MapTableQuery(struct MapTable * pTable, logic_addr_t addr)
{
    struct BlockEntry * be;

    be = &pTable->entries[IndexOfBlock(addr)];
    if (be->fineGrain)
    {
        struct PageEntry * pe;
        pe = &be->ptr->entries[RelativeIndexOfPage(addr)];
        return pTable->dataStartOffset + (be->physBlockIndex << BITS_2M) + (pe->relativeIndex << BITS_4K) +
               PageOffsetOfAddr(addr);
    }
    return pTable->dataStartOffset + (be->physBlockIndex << BITS_2M) + BlockOffsetOfAddr(addr);
}

void MapTableBlockSet(struct MapTable * pTable, logic_addr_t logicAddr, physical_block_t block)
{
    pTable->entries[IndexOfBlock(logicAddr)].physBlockIndex = block;
}

void MapTablePageSet(struct MapTable * pTable, logic_addr_t logicAddr, UINT64 relativeIndex)
{
    struct BlockEntry * be;
    be = &pTable->entries[IndexOfBlock(logicAddr)];
    be->ptr->entries[RelativeIndexOfPage(logicAddr)].relativeIndex = relativeIndex;
}

int MapTableIsBlock(struct MapTable * pTable, logic_addr_t addr)
{
    return !(pTable->entries[IndexOfBlock(addr)].fineGrain);
}

int MapTableIsBlockBusy(struct MapTable * pTable, logic_addr_t addr)
{
    return pTable->entries[IndexOfBlock(addr)].busy;
}

int MapTableIsPageBusy(struct MapTable * pTable, logic_addr_t addr)
{
    return pTable->entries[IndexOfBlock(addr)].ptr->entries[RelativeIndexOfPage(addr)].busy;
}

void MapTableBlockTrim(struct MapTable * pTable, logic_addr_t addr)
{
    pTable->entries[IndexOfBlock(addr)].busy = 0;
}

void MapTablePageTrim(struct MapTable * pTable, logic_addr_t addr)
{
    pTable->entries[IndexOfBlock(addr)].ptr->entries[RelativeIndexOfPage(addr)].busy = 0;
}

void MapTableBlockBusy(struct MapTable * pTable, logic_addr_t addr)
{
    pTable->entries[IndexOfBlock(addr)].busy = 1;
}

void MapTablePageBusy(struct MapTable * pTable, logic_addr_t addr)
{
    struct BlockEntry * be;
    be = &pTable->entries[IndexOfBlock(addr)];
    be->busy = 1;
    be->ptr->entries[RelativeIndexOfPage(addr)].busy = 1;
}

void MapTableMerge(struct MapTable * pTable, logic_addr_t addr)
{
    struct BlockEntry * be;

    be = &pTable->entries[IndexOfBlock(addr)];
    be->fineGrain = 0;
    kfree(be->ptr);
    be->ptr = NULL;
}

void MapTableSplit(struct MapTable * pTable, logic_addr_t addr)
{
    struct BlockEntry * be;
    struct PageEntryTable * pet;
    int i;

    pet = kmalloc(sizeof(struct PageEntryTable), GFP_KERNEL);
    for (i = 0; i < 512; ++i)
    {
        pet->entries[i].value = 0;
        pet->entries[i].relativeIndex = i;
    }
    be = &pTable->entries[IndexOfBlock(addr)];
    be->fineGrain = 1;
    be->ptr = pet;
}

static inline UINT64 MapTableBlockReadLock(struct MapTable * pTable, logic_addr_t addr)
{
    UINT64 value;
    UINT64 blockIndex;

    blockIndex = IndexOfBlock(addr);
    do
    {
        value = pTable->entries[blockIndex].lock;
    } while (value & 1);
    return value;
}

static inline int MapTableBlockReadUnlock(struct MapTable * pTable, logic_addr_t addr, UINT64 oldValue)
{
    return oldValue != pTable->entries[IndexOfBlock(addr)].lock;
}

static inline void MapTableBlockWaitUntilUnlock(struct MapTable * pTable, logic_addr_t addr)
{
    UINT64 value;
    UINT64 blockIndex;

    blockIndex = IndexOfBlock(addr);
    do
    {
        value = pTable->entries[blockIndex].lock;
    } while (value & 1);
}

static inline void MapTableBlockWriteLock(struct MapTable * pTable, logic_addr_t addr)
{
    UINT64 blockIndex;
    struct BlockEntry entry, newEntry;

    blockIndex = IndexOfBlock(addr);
    do
    {
    retry:
        entry = pTable->entries[blockIndex];
        if (entry.lock & 1)
            goto retry;
        newEntry = entry;
        newEntry.lock++;
    } while (!BOOL_COMPARE_AND_SWAP((UINT64 *)(&pTable->entries[blockIndex]), entry.value, newEntry.value));
}

static inline void MapTableBlockWriteUnlock(struct MapTable * pTable, logic_addr_t addr)
{
    struct BlockEntry * be;

    be = &pTable->entries[IndexOfBlock(addr)];
    be->lock++;
}

static inline UINT64 MapTablePageReadLock(struct MapTable * pTable, logic_addr_t addr)
{
    struct PageEntry * pe;
    UINT64 value;

    pe = &pTable->entries[IndexOfBlock(addr)].ptr->entries[RelativeIndexOfPage(addr)];
    do
    {
        value = pe->lock;
    } while (value & 1);
    return value;
}

static inline int MapTablePageReadUnlock(struct MapTable * pTable, logic_addr_t addr, UINT64 oldValue)
{
    return pTable->entries[IndexOfBlock(addr)].ptr->entries[RelativeIndexOfPage(addr)].lock != oldValue;
}

static inline void MapTablePageWaitUntilUnlock(struct MapTable * pTable, struct PageEntry * pe)
{
    UINT64 value;
    do
    {
        value = pe->lock;
    } while (value & 1);
}

static inline void MapTablePageWriteLock(struct MapTable * pTable, logic_addr_t addr)
{
    struct PageEntry * pe;
    struct PageEntry oldEntry, newEntry;

    pe = &pTable->entries[IndexOfBlock(addr)].ptr->entries[RelativeIndexOfPage(addr)];
    do
    {
    retry:
        oldEntry = *pe;
        if (oldEntry.lock & 1)
            goto retry;
        newEntry = oldEntry;
        newEntry.lock++;
    } while (!BOOL_COMPARE_AND_SWAP(&pe->value, oldEntry.value, newEntry.value));
}

static inline void MapTablePageWriteUnlock(struct MapTable * pTable, logic_addr_t addr)
{
    struct PageEntry * pe;

    pe = &pTable->entries[IndexOfBlock(addr)].ptr->entries[RelativeIndexOfPage(addr)];
    pe->lock++;
}

static inline int LockBlockForSwap(struct MapTable * pTable, logic_addr_t logicAddr, nvm_addr_t physAddr)
{
    MapTableBlockWriteLock(pTable, logicAddr);
    if (MapBlockQuery(pTable, logicAddr) != nvm_addr_to_block(physAddr, pTable->dataStartOffset))
    {
        MapTableBlockWriteUnlock(pTable, logicAddr);
        return 1;
    }
    return 0;
}

static inline void WaitForAllPagesUnlock(struct MapTable * pTable, logic_addr_t addr)
{
    struct BlockEntry * be;

    be = &pTable->entries[IndexOfBlock(addr)];
    if (be->fineGrain)
    {
        int i;
        struct PageEntry * pe;

        pe = be->ptr->entries;
        for (i = 0; i < 512; ++i)
        {
            MapTablePageWaitUntilUnlock(pTable, pe);
            pe++;
        }
    }
}

void MapTableBlockSwapFast(struct MapTable * pTable, logical_block_t blockA, logical_block_t blockB)
{
    UINT64 indexA, indexB;
    indexA = pTable->entries[blockA].physBlockIndex;
    indexB = pTable->entries[blockB].physBlockIndex;
    pTable->entries[blockA].physBlockIndex = indexB;
    pTable->entries[blockB].physBlockIndex = indexA;
}

void MapTableBlockSwapSlow(struct MapTable * pTable, logical_block_t blockA, logical_block_t blockB,
                           logical_block_t blockC)
{
    UINT64 indexA, indexB, indexC;

    indexA = pTable->entries[blockA].physBlockIndex;
    indexB = pTable->entries[blockB].physBlockIndex;
    indexC = pTable->entries[blockC].physBlockIndex;
    pTable->entries[blockA].physBlockIndex = indexB;
    pTable->entries[blockB].physBlockIndex = indexC;
    pTable->entries[blockC].physBlockIndex = indexA;
}

int MapTableLockBlockForSwap(struct MapTable * pTable, logic_addr_t logicAddr, nvm_addr_t physAddr)
{
    if (LockBlockForSwap(pTable, logicAddr, physAddr))
        return 1;
    WaitForAllPagesUnlock(pTable, logicAddr);
    return 0;
}

void MapTableUnlockBlockForSwap(struct MapTable * pTable, logic_addr_t logicAddr)
{
    MapTableBlockWriteUnlock(pTable, logicAddr);
}

void MapTablePageSwap(struct MapTable * pTable, logical_page_t pageA, logical_page_t pageB)
{
    UINT64 mapIndexA, mapIndexB;
    UINT64 relativeIndexA, relativeIndexB, block;
    relativeIndexA = pageA & 511;
    relativeIndexB = pageB & 511;
    block = logical_page_to_block(pageA);
    mapIndexA = pTable->entries[block].ptr->entries[relativeIndexA].relativeIndex;
    mapIndexB = pTable->entries[block].ptr->entries[relativeIndexB].relativeIndex;
    pTable->entries[block].ptr->entries[relativeIndexA].relativeIndex = mapIndexB;
    pTable->entries[block].ptr->entries[relativeIndexB].relativeIndex = mapIndexA;
}

int MapTableLockBlockForPageSwap(struct MapTable * pTable, logic_addr_t logicAddr, nvm_addr_t physAddr)
{
    return LockBlockForSwap(pTable, logicAddr, physAddr);
}

void MapTableUnlockBlockForPageSwap(struct MapTable * pTable, logic_addr_t logicAddr)
{
    MapTableBlockWriteUnlock(pTable, logicAddr);
}

int MapTableLockPageForSwap(struct MapTable * pTable, logic_addr_t logicAddr, nvm_addr_t physAddr)
{
    MapTableLockPageForWrite(pTable, logicAddr);
    if (MapTableQuery(pTable, logicAddr) != physAddr)
    {
        MapTableUnlockPageForWrite(pTable, logicAddr);
        return 1;
    }
    return 0;
}

void MapTableUnlockPageForSwap(struct MapTable * pTable, logic_addr_t logicAddr)
{
    MapTableUnlockPageForWrite(pTable, logicAddr);
}

struct MapTableLockHandle MapTableLockBlockForRead(struct MapTable * pTable, logic_addr_t addr)
{
    struct MapTableLockHandle handle;

    handle.value1 = MapTableBlockReadLock(pTable, addr);
    return handle;
}

int MapTableUnlockBlockForRead(struct MapTable * pTable, logic_addr_t addr, struct MapTableLockHandle handle)
{
    return MapTableBlockReadUnlock(pTable, addr, handle.value1);
}

void MapTableLockBlockForWrite(struct MapTable * pTable, logic_addr_t addr)
{
    MapTableBlockWriteLock(pTable, addr);
}

void MapTableUnlockBlockForWrite(struct MapTable * pTable, logic_addr_t addr)
{
    MapTableBlockWriteUnlock(pTable, addr);
}

struct MapTableLockHandle MapTableLockPageForRead(struct MapTable * pTable, logic_addr_t addr)
{
    struct MapTableLockHandle handle;

    handle.value1 = MapTableBlockReadLock(pTable, addr);
    handle.value2 = MapTablePageReadLock(pTable, addr);
    return handle;
}

int MapTableUnlockPageForRead(struct MapTable * pTable, logic_addr_t addr, struct MapTableLockHandle handle)
{
    return MapTableBlockReadUnlock(pTable, addr, handle.value1) || MapTablePageReadUnlock(pTable, addr, handle.value2);
}

void MapTableLockPageForWrite(struct MapTable * pTable, logic_addr_t addr)
{
    MapTablePageWriteLock(pTable, addr);
}

void MapTableUnlockPageForWrite(struct MapTable * pTable, logic_addr_t addr)
{
    MapTablePageWriteUnlock(pTable, addr);
}