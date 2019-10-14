#ifndef PAGE_UNMAP_TABLE_H
#define PAGE_UNMAP_TABLE_H

#include "Types.h"

struct BlockUnmapTable;

struct NVMPageUnmapTableEntry
{
    UINT64 unmapPage0 : 9;
    UINT64 unmapPage1 : 9;
    UINT64 unmapPage2 : 9;
    UINT64 unmapPage3 : 9;
    UINT64 unmapPage4 : 9;
    UINT64 unmapPage5 : 9;
    UINT64 unmapPage6 : 9;
};

struct NVMPageUnmapTableEntryGroup
{
    struct NVMPageUnmapTableEntry entries[74];
};

struct NVMPageUnmapTable
{
    struct NVMPageUnmapTableEntryGroup groups[0];
};

struct PageUnmapTable
{
    nvm_addr_t addr;
    struct BlockUnmapTable * pBlockUnmapTable;
};

void PageUnmapTableFormat(struct PageUnmapTable * pTable, struct BlockUnmapTable * pBlockUnmapTable, nvm_addr_t addr,
                          UINT64 pageNum, UINT64 tableSize);
void PageUnmapTableInit(struct PageUnmapTable * pTable, struct BlockUnmapTable * pBlockUnmapTable, nvm_addr_t addr,
                        UINT64 pageNum);
void PageUnmapTableUninit(struct PageUnmapTable * pTable);
logical_page_t PageUnmapTableGet(struct PageUnmapTable * pTable, physical_page_t pageSeq);
void PageUnmapTableSet(struct PageUnmapTable * pTable, physical_page_t pageSeq, UINT64 relativeIndex);
void PageUnmapTableBatchGet(struct PageUnmapTable * pTable, physical_block_t block, UINT64 * relativeIndice);

#endif