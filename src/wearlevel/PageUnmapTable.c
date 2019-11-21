#include "Align.h"
#include "BlockUnmapTable.h"
#include "NVMOperations.h"
#include "PageUnmapTable.h"

static inline UINT64 GetUnmapPageRelativeIndexFromEntry(struct NVMPageUnmapTableEntry * entry, UINT64 index)
{
    switch (index)
    {
    case 0:
        return entry->unmapPage0;
    case 1:
        return entry->unmapPage1;
    case 2:
        return entry->unmapPage2;
    case 3:
        return entry->unmapPage3;
    case 4:
        return entry->unmapPage4;
    case 5:
        return entry->unmapPage5;
    }
    return 0;
}

static inline UINT64 GetUnmapPageBusyFlagFromEntry(struct NVMPageUnmapTableEntry * entry, UINT64 index)
{
    switch (index)
    {
    case 0:
        return entry->unmapPage0Busy;
    case 1:
        return entry->unmapPage1Busy;
    case 2:
        return entry->unmapPage2Busy;
    case 3:
        return entry->unmapPage3Busy;
    case 4:
        return entry->unmapPage4Busy;
    case 5:
        return entry->unmapPage5Busy;
    }
    return 0;
}

static inline void SetUnmapPageRelativeIndexOfEntry(struct NVMPageUnmapTableEntry * entry, UINT64 index, UINT64 value)
{
    switch (index)
    {
    case 0:
        entry->unmapPage0 = value;
        break;
    case 1:
        entry->unmapPage1 = value;
        break;
    case 2:
        entry->unmapPage2 = value;
        break;
    case 3:
        entry->unmapPage3 = value;
        break;
    case 4:
        entry->unmapPage4 = value;
        break;
    case 5:
        entry->unmapPage5 = value;
        break;
    }
}

static inline void SetUnmapPageBusyFlagOfEntry(struct NVMPageUnmapTableEntry * entry, UINT64 index, UINT64 busy)
{
    switch (index)
    {
    case 0:
        entry->unmapPage0Busy = busy;
        break;
    case 1:
        entry->unmapPage1Busy = busy;
        break;
    case 2:
        entry->unmapPage2Busy = busy;
        break;
    case 3:
        entry->unmapPage3Busy = busy;
        break;
    case 4:
        entry->unmapPage4Busy = busy;
        break;
    case 5:
        entry->unmapPage5Busy = busy;
        break;
    }
}

void PageUnmapTableFormat(struct PageUnmapTable * pTable, struct BlockUnmapTable * pBlockUnmapTable, nvm_addr_t addr,
                          UINT64 pageNum, UINT64 tableSize)
{
    NVMemset(addr, 0, tableSize);
    pTable->addr = addr;
    pTable->pBlockUnmapTable = pBlockUnmapTable;
}

void PageUnmapTableInit(struct PageUnmapTable * pTable, struct BlockUnmapTable * pBlockUnmapTable, nvm_addr_t addr,
                        UINT64 pageNum)
{
    pTable->addr = addr;
    pTable->pBlockUnmapTable = pBlockUnmapTable;
}

void PageUnmapTableUninit(struct PageUnmapTable * pTable)
{
}

static inline logical_page_t RelativeIndexToPage(logical_block_t block, UINT64 index)
{
    return (block << (BITS_2M - BITS_4K)) + index;
}

static inline UINT64 RelativeIndexOfPage(logical_page_t page)
{
    return page & ((1UL << (BITS_2M - BITS_4K)) - 1);
}

void PageUnmapTableGet(struct PageUnmapTable * pTable, physical_page_t pageSeq, struct PageInfo * info)
{
    physical_block_t physBlock;
    logical_block_t logicBlock;
    struct BlockInfo blockInfo;
    nvm_addr_t addr;
    UINT64 offset;
    UINT64 offsetInsideEntry;
    UINT64 pageRelativeIndex;
    struct NVMPageUnmapTableEntry entry;

    pageRelativeIndex = pageSeq & ((1UL << (BITS_2M - BITS_4K)) - 1);
    physBlock = pageSeq >> (BITS_2M - BITS_4K);
    BlockUnmapTableGet(pTable->pBlockUnmapTable, physBlock, &blockInfo);
    logicBlock = blockInfo.unmapBlock;
    offset = logicBlock * sizeof(struct NVMPageUnmapTableEntryGroup) +
             (pageRelativeIndex / 6) * sizeof(struct NVMPageUnmapTableEntry);
    offsetInsideEntry = pageRelativeIndex % 6;
    addr = pTable->addr + offset;

    NVMRead(addr, sizeof(struct NVMPageUnmapTableEntry), &entry);

    info->unmapPage = RelativeIndexToPage(logicBlock, GetUnmapPageRelativeIndexFromEntry(&entry, offsetInsideEntry));
    info->busy = GetUnmapPageBusyFlagFromEntry(&entry, offsetInsideEntry);
}

void PageUnmapTableSet(struct PageUnmapTable * pTable, physical_page_t pageSeq, struct PageInfo * info)
{
    physical_block_t physBlock;
    logical_block_t logicBlock;
    struct BlockInfo blockInfo;
    nvm_addr_t addr;
    UINT64 offset;
    UINT64 offsetInsideEntry;
    UINT64 pageRelativeIndex;
    struct NVMPageUnmapTableEntry entry;

    pageRelativeIndex = pageSeq & ((1UL << (BITS_2M - BITS_4K)) - 1);
    physBlock = pageSeq >> (BITS_2M - BITS_4K);
    BlockUnmapTableGet(pTable->pBlockUnmapTable, physBlock, &blockInfo);
    logicBlock = blockInfo.unmapBlock;
    offset = logicBlock * sizeof(struct NVMPageUnmapTableEntryGroup) +
             (pageRelativeIndex / 6) * sizeof(struct NVMPageUnmapTableEntry);
    offsetInsideEntry = pageRelativeIndex % 6;
    addr = pTable->addr + offset;

    NVMRead(addr, sizeof(struct NVMPageUnmapTableEntry), &entry);
    SetUnmapPageRelativeIndexOfEntry(&entry, offsetInsideEntry, RelativeIndexOfPage(info->unmapPage));
    SetUnmapPageBusyFlagOfEntry(&entry, offsetInsideEntry, info->busy);
    NVMWrite(addr, sizeof(struct NVMPageUnmapTableEntry), &entry);
}

void PageUnmapTableBatchGet(struct PageUnmapTable * pTable, physical_block_t block, struct PageInfo * infos)
{
    logical_block_t logicBlock;
    struct BlockInfo info;
    nvm_addr_t addr;
    struct NVMPageUnmapTableEntry entry;
    int i, j;

    BlockUnmapTableGet(pTable->pBlockUnmapTable, block, &info);
    logicBlock = info.unmapBlock;
    addr = pTable->addr + logicBlock * sizeof(struct NVMPageUnmapTableEntryGroup);
    for (i = 0, j = 0; i < 512; ++i)
    {
        if (j == 0)
            NVMRead(addr, sizeof(struct NVMPageUnmapTableEntry), &entry);
        infos[i].unmapPage = RelativeIndexToPage(logicBlock, GetUnmapPageRelativeIndexFromEntry(&entry, j));
        infos[i].busy = GetUnmapPageBusyFlagFromEntry(&entry, j);
        addr += sizeof(struct NVMPageUnmapTableEntry);
        ++j;
        if (j >= 6)
            j = 0;
    }
}

void PageUnmapTableBatchFormat(struct PageUnmapTable * pTable, physical_block_t block, UINT8 busy)
{
    struct NVMPageUnmapTableEntry entry;
    int i, j;
    nvm_addr_t addr;

    addr = pTable->addr + block * sizeof(struct NVMPageUnmapTableEntryGroup);
    for (i = 0, j = 0; i < 512; ++i)
    {
        SetUnmapPageRelativeIndexOfEntry(&entry, j, i);
        SetUnmapPageBusyFlagOfEntry(&entry, j, busy);
        ++j;
        if (j >= 6)
        {
            j = 0;
            NVMWrite(addr, sizeof(struct NVMPageUnmapTableEntry), &entry);
            addr += sizeof(struct NVMPageUnmapTableEntry);
        }
    }
}