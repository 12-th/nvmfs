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
    case 6:
        return entry->unmapPage6;
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
    case 6:
        entry->unmapPage6 = value;
        break;
    }
}

void PageUnmapTableFormat(struct PageUnmapTable * pTable, struct BlockUnmapTable * pBlockUnmapTable, nvm_addr_t addr)
{
    pTable->addr = addr;
    pTable->pBlockUnmapTable = pBlockUnmapTable;
}

void PageUnmapTableInit(struct PageUnmapTable * pTable, struct BlockUnmapTable * pBlockUnmapTable, nvm_addr_t addr)
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

logical_page_t PageUnmapTableGet(struct PageUnmapTable * pTable, physical_page_t pageSeq)
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
             (pageRelativeIndex / 7) * sizeof(struct NVMPageUnmapTableEntry);
    offsetInsideEntry = pageRelativeIndex % 7;
    addr = pTable->addr + offset;

    NVMRead(addr, sizeof(struct NVMPageUnmapTableEntry), &entry);
    return RelativeIndexToPage(logicBlock, GetUnmapPageRelativeIndexFromEntry(&entry, offsetInsideEntry));
}

void PageUnmapTableSet(struct PageUnmapTable * pTable, physical_page_t pageSeq, logical_page_t logicPage)
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
             (pageRelativeIndex / 7) * sizeof(struct NVMPageUnmapTableEntry);
    offsetInsideEntry = pageRelativeIndex % 7;
    addr = pTable->addr + offset;

    NVMRead(addr, sizeof(struct NVMPageUnmapTableEntry), &entry);
    SetUnmapPageRelativeIndexOfEntry(&entry, offsetInsideEntry, RelativeIndexOfPage(logicPage));
    NVMWrite(addr, sizeof(struct NVMPageUnmapTableEntry), &entry);
}

void PageUnmapTableBatchFormat(struct PageUnmapTable * pTable, physical_block_t block)
{
    struct NVMPageUnmapTableEntry entry;
    int i, j;
    nvm_addr_t addr;

    addr = pTable->addr + block * sizeof(struct NVMPageUnmapTableEntryGroup);
    for (i = 0, j = 0; i < 512; ++i)
    {
        SetUnmapPageRelativeIndexOfEntry(&entry, j, i);
        ++j;
        if (j > 6)
        {
            j = 0;
            NVMWrite(addr, sizeof(struct NVMPageUnmapTableEntry), &entry);
            addr += sizeof(struct NVMPageUnmapTableEntry);
        }
    }
    NVMWrite(addr, sizeof(struct NVMPageUnmapTableEntry), &entry);
}